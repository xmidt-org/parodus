/**
 * @file websocket_mgr.c
 *
 * @description This file is used to manage websocket connection, websocket messages and ping/pong (heartbeat) mechanism.
 *
 * Copyright (c) 2015  Comcast
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <cJSON.h>
#include <nopoll.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <math.h>
#include <errno.h>
#include "wss_mgr.h"

#include <netdb.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <wrp-c.h>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>

#include "ParodusInternal.h"
#include "time.h"
#include "connection.h"
#include "spin_thread.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/


/* WebPA default Config */
#define WEBPA_SERVER_URL                                "talaria-beta.webpa.comcast.net"
#define WEBPA_SERVER_PORT                               8080
#define WEBPA_RETRY_INTERVAL_SEC                        10
#define WEBPA_MAX_PING_WAIT_TIME_SEC                    180


#define METADATA_COUNT 					11			
#define WEBPA_MESSAGE_HANDLE_INTERVAL_MSEC          	250
#define HEARTBEAT_RETRY_SEC                         	30      /* Heartbeat (ping/pong) timeout in seconds */
#define KEEPALIVE_INTERVAL_SEC                         	30
#define PARODUS_UPSTREAM "tcp://127.0.0.1:6666"


#define IOT "iot"
#define HARVESTER "harvester"
#define GET_SET "get_set"


/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

static ParodusCfg parodusCfg;
static noPollConn *g_conn = NULL;
static int numOfClients = 0;

reg_list_item_t * head = NULL;

void *metadataPack;
size_t metaPackSize=-1;

volatile unsigned int heartBeatTimer = 0;
volatile bool terminated = false;
ParodusMsg *ParodusMsgQ = NULL;
UpStreamMsg *UpStreamMsgQ = NULL;

bool close_retry = false;
bool LastReasonStatus = false;
char *reconnect_reason = "webpa_process_starts";

void close_and_unref_connection(noPollConn *conn);
char parodus_url[32] ={'\0'};

pthread_mutex_t g_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t close_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t nano_mut=PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t g_cond=PTHREAD_COND_INITIALIZER;
pthread_cond_t close_con=PTHREAD_COND_INITIALIZER;
pthread_cond_t nano_con=PTHREAD_COND_INITIALIZER;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

static void __report_log (noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data);
static void *handle_upstream();
static void *handleUpStreamEvents();
static void *messageHandlerTask();
static void *serviceAliveTask();
static void getParodusUrl();
static void sendAuthStatus(reg_list_item_t *new_node);
static void addToList( wrp_msg_t **msg);
static int deleteFromList(char* service_name);
/*
 Export parodusCfg
 */
ParodusCfg *get_parodus_cfg(void) 
{
    return &parodusCfg;
}

noPollConn *get_global_conn(void)
{
    return g_conn;
}

void set_global_conn(noPollConn *conn)
{
    g_conn = conn;
}

/**
 * @brief __report_log Nopoll log handler 
 * Nopoll log handler for integrating nopoll logs 
 */
static void __report_log (noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data)
{
	
	UNUSED(ctx);
	UNUSED(user_data);
	if (level == NOPOLL_LEVEL_DEBUG) 
	{
  	    //PARODUS_DEBUG("%s\n", log_msg);
	}
	if (level == NOPOLL_LEVEL_INFO) 
	{
		PARODUS_INFO("%s\n", log_msg);
	}
	if (level == NOPOLL_LEVEL_WARNING) 
	{
  	     PARODUS_DEBUG("%s\n", log_msg);
	}
	if (level == NOPOLL_LEVEL_CRITICAL) 
	{
  	     PARODUS_ERROR("%s\n", log_msg );
	}
	return;
}

void close_and_unref_connection(noPollConn *conn)
{
    if (conn) {
        nopoll_conn_close(conn);
        if (0 < nopoll_conn_ref_count (conn)) {
            nopoll_conn_unref(conn);
        }
    }
}


static void getParodusUrl()
{
    const char *parodusIp = NULL;
    const char * envParodus = getenv ("PARODUS_SERVICE_URL");
    
    if( envParodus != NULL)
    {
      parodusIp = envParodus;
    }
    else
    {
      parodusIp = PARODUS_UPSTREAM ;
    }
    
    snprintf(parodus_url,sizeof(parodus_url),"%s", parodusIp);
    PARODUS_INFO("formatted parodus Url %s\n",parodus_url);
	
}



/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

void createSocketConnection(void *config_in, void (* initKeypress)())
{
	int intTimer=0;	
    	ParodusCfg *tmpCfg = (ParodusCfg*)config_in;
        noPollCtx *ctx;
        
   	loadParodusCfg(tmpCfg,get_parodus_cfg());
	PARODUS_DEBUG("Configure nopoll thread handlers in Parodus\n");
	nopoll_thread_handlers(&createMutex, &destroyMutex, &lockMutex, &unlockMutex);
	ctx = nopoll_ctx_new();
	if (!ctx) 
	{
		PARODUS_ERROR("\nError creating nopoll context\n");
	}

	#ifdef NOPOLL_LOGGER
  		nopoll_log_set_handler (ctx, __report_log, NULL);
	#endif
	
	createNopollConnection(ctx);
	getParodusUrl();
        UpStreamMsgQ = NULL;
        StartThread(handle_upstream);
        StartThread(handleUpStreamEvents);
        ParodusMsgQ = NULL;
        StartThread(messageHandlerTask);
        StartThread(serviceAliveTask);
	
	if (NULL != initKeypress) 
	{
  		(* initKeypress) ();
	}
	
	do
	{
		nopoll_loop_wait(ctx, 5000000);
		intTimer = intTimer + 5;
		
		if(heartBeatTimer >= get_parodus_cfg()->webpa_ping_timeout) 
		{
			if(!close_retry) 
			{
				PARODUS_ERROR("ping wait time > %d. Terminating the connection with WebPA server and retrying\n", get_parodus_cfg()->webpa_ping_timeout);
				reconnect_reason = "Ping_Miss";
				LastReasonStatus = true;
				pthread_mutex_lock (&close_mut);
				close_retry = true;
				pthread_mutex_unlock (&close_mut);
			}
			else
			{			
				PARODUS_DEBUG("heartBeatHandler - close_retry set to %d, hence resetting the heartBeatTimer\n",close_retry);
			}
			heartBeatTimer = 0;
		}
		else if(intTimer >= 30)
		{
			PARODUS_DEBUG("heartBeatTimer %d\n",heartBeatTimer);
			heartBeatTimer += HEARTBEAT_RETRY_SEC;	
			intTimer = 0;		
		}
		
		if(close_retry)
		{
			PARODUS_INFO("close_retry is %d, hence closing the connection and retrying\n", close_retry);
			close_and_unref_connection(g_conn);
			g_conn = NULL;
			createNopollConnection(ctx);
		}		
	} while(!close_retry);
	  	
	close_and_unref_connection(g_conn);
	nopoll_ctx_unref(ctx);
	nopoll_cleanup_library();
}

/**
 * @brief Interface to terminate WebSocket client connections and clean up resources.
 */

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

       
/*
 * @brief To handle UpStream messages which is received from nanomsg server socket
 */

static void *handle_upstream()
{
	UpStreamMsg *message;
	int sock, bind;
	int bytes =0;
	void *buf;
	
	PARODUS_DEBUG("******** Start of handle_upstream ********\n");
		
	sock = nn_socket( AF_SP, NN_PULL );
	if(sock >= 0)
	{
                bind = nn_bind(sock, parodus_url);
                if(bind < 0)
                {
                        PARODUS_ERROR("Unable to bind socket (errno=%d, %s)\n",errno, strerror(errno));
                }
                else
                {
                        while( 1 ) 
                        {
                                buf = NULL;
                                PARODUS_INFO("nanomsg server gone into the listening mode...\n");
                                bytes = nn_recv (sock, &buf, NN_MSG, 0);
                                PARODUS_INFO("Upstream message received from nanomsg client: \"%s\"\n", (char*)buf);
                                message = (UpStreamMsg *)malloc(sizeof(UpStreamMsg));

                                if(message)
                                {
                                        message->msg =buf;
                                        message->len =bytes;
                                        message->next=NULL;
                                        pthread_mutex_lock (&nano_mut);
                                        //Producer adds the nanoMsg into queue
                                        if(UpStreamMsgQ == NULL)
                                        {
                                                UpStreamMsgQ = message;

                                                PARODUS_DEBUG("Producer added message\n");
                                                pthread_cond_signal(&nano_con);
                                                pthread_mutex_unlock (&nano_mut);
                                                PARODUS_DEBUG("mutex unlock in producer thread\n");
                                        }
                                        else
                                        {
                                                UpStreamMsg *temp = UpStreamMsgQ;
                                                while(temp->next)
                                                {
                                                        temp = temp->next;
                                                }

                                                temp->next = message;
                                                pthread_mutex_unlock (&nano_mut);
                                        }
                                }
                                else
                                {
                                        PARODUS_ERROR("failure in allocation for message\n");
                                }
                        }
                }
	}
	else
	{
		    PARODUS_ERROR("Unable to create socket (errno=%d, %s)\n",errno, strerror(errno));
	}
	PARODUS_DEBUG ("End of handle_upstream\n");
	return 0;
}


static void *handleUpStreamEvents()
{		
        int rv=-1, rc = -1;	
        int msgType;
        wrp_msg_t *msg;	
        void *appendData;
        size_t encodedSize;
        reg_list_item_t *temp = NULL;
        int matchFlag = 0;

        while(1)
        {
                pthread_mutex_lock (&nano_mut);
                PARODUS_DEBUG("mutex lock in consumer thread\n");
                if(UpStreamMsgQ != NULL)
                {
                        UpStreamMsg *message = UpStreamMsgQ;
                        UpStreamMsgQ = UpStreamMsgQ->next;
                        pthread_mutex_unlock (&nano_mut);
                        PARODUS_DEBUG("mutex unlock in consumer thread\n");

                        if (!terminated) 
                        {
                                /*** Decoding Upstream Msg to check msgType ***/
                                /*** For MsgType 9 Perform Nanomsg client Registration else Send to server ***/	
                                PARODUS_DEBUG("---- Decoding Upstream Msg ----\n");

                                rv = wrp_to_struct( message->msg, message->len, WRP_BYTES, &msg );
                                if(rv > 0)
                                {
                                        msgType = msg->msg_type;				   
                                        if(msgType == 9)
                                        {
                                                PARODUS_INFO("\n Nanomsg client Registration for Upstream\n");
                                                //Extract serviceName and url & store it in a linked list for reg_clients
                                                if(numOfClients !=0)
                                                {
                                                        temp = head;
                                                        while(temp!=NULL)
                                                        {
                                                                if(strcmp(temp->service_name, msg->u.reg.service_name)==0)
                                                                {
                                                                        PARODUS_INFO("match found, client is already registered\n");
                                                                        strncpy(temp->url,msg->u.reg.url, strlen(msg->u.reg.url)+1);
                                                                        if(nn_shutdown(temp->sock, 0) < 0)
                                                                        {
                                                                                PARODUS_ERROR ("Failed to shutdown\n");
                                                                        }

                                                                        temp->sock = nn_socket(AF_SP,NN_PUSH );
                                                                        if(temp->sock >= 0)
                                                                        {					
                                                                                int t = NANOMSG_SOCKET_TIMEOUT_MSEC;
                                                                                rc = nn_setsockopt(temp->sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
                                                                                if(rc < 0)
                                                                                {
                                                                                        PARODUS_ERROR ("Unable to set socket timeout (errno=%d, %s)\n",errno, strerror(errno));
                                                                                }
                                                                                rc = nn_connect(temp->sock, msg->u.reg.url); 
                                                                                if(rc < 0)
                                                                                {
                                                                                        PARODUS_ERROR ("Unable to connect socket (errno=%d, %s)\n",errno, strerror(errno));
                                                                                }
                                                                                else
                                                                                {
                                                                                        PARODUS_INFO("Client registered before. Sending acknowledgement \n"); 
                                                                                        sendAuthStatus(temp);
                                                                                        matchFlag = 1;
                                                                                        break;
                                                                                }
                                                                        }
                                                                        else
                                                                        {
                                                                                PARODUS_ERROR("Unable to create socket (errno=%d, %s)\n",errno, strerror(errno));
                                                                        }
                                                                }
                                                                PARODUS_DEBUG("checking the next item in the list\n");
                                                                temp= temp->next;
                                                        }	
                                                }
                                                PARODUS_DEBUG("matchFlag is :%d\n", matchFlag);
                                                if((matchFlag == 0) || (numOfClients == 0))
                                                {
                                                        numOfClients = numOfClients + 1;
                                                        PARODUS_DEBUG("Adding nanomsg clients to list\n");
                                                        addToList(&msg);
                                                }
                                        }
                                        else
                                        {
                                                //Sending to server for msgTypes 3, 4, 5, 6, 7, 8.
                                                PARODUS_INFO(" Received upstream data with MsgType: %d\n", msgType);
                                                //Appending metadata with packed msg received from client
                                                if(metaPackSize > 0)
                                                {
                                                        PARODUS_DEBUG("Appending received msg with metadata\n");
                                                        encodedSize = appendEncodedData( &appendData, message->msg, message->len, metadataPack, metaPackSize );
                                                        PARODUS_DEBUG("encodedSize after appending :%zu\n", encodedSize);
                                                        PARODUS_DEBUG("metadata appended upstream msg %s\n", (char *)appendData);
                                                        PARODUS_INFO("Sending metadata appended upstream msg to server\n");
                                                        handleUpstreamMessage(g_conn,appendData, encodedSize);

                                                        free( appendData);
                                                        appendData =NULL;
                                                }
                                                else
                                                {		
                                                        PARODUS_ERROR("Failed to send upstream as metadata packing is not successful\n");
                                                }
                                        }
                                }
                                else
                                {
                                        PARODUS_ERROR("Error in msgpack decoding for upstream\n");
                                }
                                PARODUS_DEBUG("Free for upstream decoded msg\n");
                                wrp_free_struct(msg);
                                msg = NULL;
                        }

                        if(nn_freemsg (message->msg) < 0)
                        {
                                PARODUS_ERROR ("Failed to free msg\n");
                        }
                        free(message);
                        message = NULL;
                }
                else
                {
                        PARODUS_DEBUG("Before pthread cond wait in consumer thread\n");   
                        pthread_cond_wait(&nano_con, &nano_mut);
                        pthread_mutex_unlock (&nano_mut);
                        PARODUS_DEBUG("mutex unlock in consumer thread after cond wait\n");
                        if (terminated) 
                        {
                                break;
                        }
                }
        }
        return NULL;
}

/*
 * @brief To handle messages
 */
static void *messageHandlerTask()
{
	while(1)
	{
		pthread_mutex_lock (&g_mutex);
		PARODUS_DEBUG("mutex lock in consumer thread\n");
		if(ParodusMsgQ != NULL)
		{
			ParodusMsg *message = ParodusMsgQ;
			ParodusMsgQ = ParodusMsgQ->next;
			pthread_mutex_unlock (&g_mutex);
			PARODUS_DEBUG("mutex unlock in consumer thread\n");
			if (!terminated) 
			{
				listenerOnMessage(message->payload, message->len, &numOfClients, &head);
			}
								
			nopoll_msg_unref(message->msg);
			free(message);
			message = NULL;
		}
		else
		{
			PARODUS_DEBUG("Before pthread cond wait in consumer thread\n");   
			pthread_cond_wait(&g_cond, &g_mutex);
			pthread_mutex_unlock (&g_mutex);
			PARODUS_DEBUG("mutex unlock in consumer thread after cond wait\n");
			if (terminated) 
			{
				break;
			}
		}
	}
	PARODUS_DEBUG ("Ended messageHandlerTask\n");
	return 0;
}

/*
 * @brief To handle registered services to indicate that the service is still alive.
 */
static void *serviceAliveTask()
{
	void *svc_bytes;
	wrp_msg_t svc_alive_msg;
	int byte = 0;
	size_t size = 0;
	int ret = -1, nbytes = -1;
	reg_list_item_t *temp = NULL; 
	
	svc_alive_msg.msg_type = WRP_MSG_TYPE__SVC_ALIVE;	
	
	nbytes = wrp_struct_to( &svc_alive_msg, WRP_BYTES, &svc_bytes );
        if(nbytes < 0)
        {
                PARODUS_ERROR(" Failed to encode wrp struct returns %d\n", nbytes);
        }
        else
        {
	        while(1)
	        {
		        PARODUS_DEBUG("serviceAliveTask: numOfClients registered is %d\n", numOfClients);
		        if(numOfClients > 0)
		        {
			        //sending svc msg to all the clients every 30s
			        temp = head;
			        size = (size_t) nbytes;
			        while(NULL != temp)
			        {
				        byte = nn_send (temp->sock, svc_bytes, size, 0);
				
				        PARODUS_DEBUG("svc byte sent :%d\n", byte);
				        if(byte == nbytes)
				        {
					        PARODUS_DEBUG("service_name: %s is alive\n",temp->service_name);
				        }
				        else
				        {
					        PARODUS_INFO("Failed to send keep alive msg, service %s is dead\n", temp->service_name);
					        //need to delete this client service from list
					
					        ret = deleteFromList((char*)temp->service_name);
				        }
				        byte = 0;
				        if(ret == 0)
				        {
					        PARODUS_DEBUG("Deletion from list is success, doing resync with head\n");
					        temp= head;
					        ret = -1;
				        }
				        else
				        {
					        temp= temp->next;
				        }
			        }
		         	PARODUS_DEBUG("Waiting for 30s to send keep alive msg \n");
		         	sleep(KEEPALIVE_INTERVAL_SEC);
	            	}
	            	else
	            	{
	            		PARODUS_INFO("No clients are registered, waiting ..\n");
	            		sleep(70);
	            	}
	        }
	}
	return 0;
}


void parseCommandLine(int argc,char **argv,ParodusCfg * cfg)
{
    
     int c;
    while (1)
    {
      static struct option long_options[] = {
          {"hw-model",     required_argument,   0, 'm'},
          {"hw-serial-number",  required_argument,  0, 's'},
          {"hw-manufacturer",  required_argument, 0, 'f'},
          {"hw-mac",  required_argument, 0, 'd'},
          {"hw-last-reboot-reason",  required_argument, 0, 'r'},
          {"fw-name",  required_argument, 0, 'n'},
          {"boot-time",  required_argument, 0, 'b'},
          {"webpa-url",  required_argument, 0, 'u'},
          {"webpa-ping-timeout",    required_argument, 0, 'p'},
          {"webpa-backoff-max",  required_argument, 0, 'o'},
          {"webpa-inteface-used",    required_argument, 0, 'i'},
          {0, 0, 0, 0}
        };
      /* getopt_long stores the option index here. */
      int option_index = 0;
      c = getopt_long (argc, argv, "m:s:f:d:r:n:b:u:p:o:i:",long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
        break;

      switch (c)
        {
        case 'm':
          parStrncpy(cfg->hw_model, optarg,sizeof(cfg->hw_model));
          PARODUS_INFO("hw-model is %s\n",cfg->hw_model);
         break;
        
        case 's':
          parStrncpy(cfg->hw_serial_number,optarg,sizeof(cfg->hw_serial_number));
          PARODUS_INFO("hw_serial_number is %s\n",cfg->hw_serial_number);
          break;

        case 'f':
          parStrncpy(cfg->hw_manufacturer, optarg,sizeof(cfg->hw_manufacturer));
          PARODUS_INFO("hw_manufacturer is %s\n",cfg->hw_manufacturer);
          break;

        case 'd':
           parStrncpy(cfg->hw_mac, optarg,sizeof(cfg->hw_mac));
           PARODUS_INFO("hw_mac is %s\n",cfg->hw_mac);
          break;
        
        case 'r':
          parStrncpy(cfg->hw_last_reboot_reason, optarg,sizeof(cfg->hw_last_reboot_reason));
          PARODUS_INFO("hw_last_reboot_reason is %s\n",cfg->hw_last_reboot_reason);
          break;

        case 'n':
          parStrncpy(cfg->fw_name, optarg,sizeof(cfg->fw_name));
          PARODUS_INFO("fw_name is %s\n",cfg->fw_name);
          break;

        case 'b':
          cfg->boot_time = atoi(optarg);
          PARODUS_INFO("boot_time is %d\n",cfg->boot_time);
          break;
       
         case 'u':
          parStrncpy(cfg->webpa_url, optarg,sizeof(cfg->webpa_url));
          PARODUS_INFO("webpa_url is %s\n",cfg->webpa_url);
          break;
        
        case 'p':
          cfg->webpa_ping_timeout = atoi(optarg);
          PARODUS_INFO("webpa_ping_timeout is %d\n",cfg->webpa_ping_timeout);
          break;

        case 'o':
          cfg->webpa_backoff_max = atoi(optarg);
          PARODUS_INFO("webpa_backoff_max is %d\n",cfg->webpa_backoff_max);
          break;

        case 'i':
          parStrncpy(cfg->webpa_interface_used, optarg,sizeof(cfg->webpa_interface_used));
          PARODUS_INFO("webpa_inteface_used is %s\n",cfg->webpa_interface_used);
          break;

        case '?':
          /* getopt_long already printed an error message. */
          break;

        default:
           PARODUS_ERROR("Enter Valid commands..\n");
          abort ();
        }
    }
  
 PARODUS_DEBUG("argc is :%d\n", argc);
 PARODUS_DEBUG("optind is :%d\n", optind);

  /* Print any remaining command line arguments (not options). */
  if (optind < argc)
    {
      PARODUS_DEBUG ("non-option ARGV-elements: ");
      while (optind < argc)
        PARODUS_DEBUG ("%s ", argv[optind++]);
      putchar ('\n');
    }

}


void sendUpstreamMsgToServer(void **resp_bytes, int resp_size)
{
	void *appendData;
	size_t encodedSize;
	//appending response with metadata 			
	if(metaPackSize > 0)
	{
	   	encodedSize = appendEncodedData( &appendData, *resp_bytes, resp_size, metadataPack, metaPackSize );
	   	PARODUS_DEBUG("metadata appended upstream response %s\n", (char *)appendData);
	   	PARODUS_DEBUG("encodedSize after appending :%zu\n", encodedSize);
	   		   
		PARODUS_INFO("Sending response to server\n");
	   	handleUpstreamMessage(g_conn,appendData, encodedSize);
	   	
		free( appendData);
		appendData =NULL;
	}
	else
	{		
		PARODUS_ERROR("Failed to send upstream as metadata packing is not successful\n");
	}

}


static void addToList( wrp_msg_t **msg)
{   
    //new_node indicates the new clients which needs to be added to list
    int rc = -1;
    reg_list_item_t *new_node = NULL;
    new_node=(reg_list_item_t *)malloc(sizeof(reg_list_item_t));
    
    if(new_node)
    {
            memset( new_node, 0, sizeof( reg_list_item_t ) );
         
            new_node->sock = nn_socket( AF_SP, NN_PUSH );
            PARODUS_DEBUG("new_node->sock is %d\n", new_node->sock);
            if(new_node->sock >= 0)
            {
                    int t = NANOMSG_SOCKET_TIMEOUT_MSEC;
                    rc = nn_setsockopt(new_node->sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
                    if(rc < 0)
                    {
                        PARODUS_ERROR ("Unable to set socket timeout (errno=%d, %s)\n",errno, strerror(errno));
                    }
                    
                    rc = nn_connect(new_node->sock, (*msg)->u.reg.url);
                    if(rc < 0)
                    {
                        PARODUS_ERROR ("Unable to connect socket (errno=%d, %s)\n",errno, strerror(errno));
                    }
                    else
                    {
                        PARODUS_DEBUG("(*msg)->u.reg.service_name is %s\n", (*msg)->u.reg.service_name);
                        PARODUS_DEBUG("(*msg)->u.reg.url is %s\n", (*msg)->u.reg.url);

                        strncpy(new_node->service_name, (*msg)->u.reg.service_name, strlen((*msg)->u.reg.service_name)+1);
                        strncpy(new_node->url, (*msg)->u.reg.url, strlen((*msg)->u.reg.url)+1);

                        new_node->next=NULL;
                         
                        if (head== NULL) //adding first client
                        {
                                PARODUS_INFO("Adding first client to list\n");
                                head=new_node;
                        }
                        else   //client2 onwards           
                        {
                                reg_list_item_t *temp = NULL;
                                PARODUS_INFO("Adding clients to list\n");
                                temp=head;

                                while(temp->next !=NULL)
                                {
	                                temp=temp->next;
                                }

                                temp->next=new_node;
                        }

                        PARODUS_DEBUG("client is added to list\n");
                        PARODUS_INFO("client service %s is added to list with url: %s\n", new_node->service_name, new_node->url);
                        if((strcmp(new_node->service_name, (*msg)->u.reg.service_name)==0)&& (strcmp(new_node->url, (*msg)->u.reg.url)==0))
                        {
                                PARODUS_INFO("sending auth status to reg client\n");
                                sendAuthStatus(new_node);
                        }
                        else
                        {
                                PARODUS_ERROR("nanomsg client registration failed\n");
                        }
                    }
            }
            else
            {
                    PARODUS_ERROR("Unable to create socket (errno=%d, %s)\n",errno, strerror(errno));
            }
    }
}


static void sendAuthStatus(reg_list_item_t *new_node)
{
	int byte = 0, nbytes = -1;	
	size_t size=0;
	void *auth_bytes;
	wrp_msg_t auth_msg_var;
	
	auth_msg_var.msg_type = WRP_MSG_TYPE__AUTH;
	auth_msg_var.u.auth.status = 200;
	
	//Sending success status to clients after each nanomsg registration
	nbytes = wrp_struct_to(&auth_msg_var, WRP_BYTES, &auth_bytes );
        if(nbytes < 0)
        {
                PARODUS_ERROR(" Failed to encode wrp struct returns %d\n", nbytes);
        }
        else
        {
	        PARODUS_INFO("Client %s Registered successfully. Sending Acknowledgement... \n ", new_node->service_name);
                size = (size_t) nbytes;
	        byte = nn_send (new_node->sock, auth_bytes, size, 0);

	        if(byte >=0)
	        {
	            PARODUS_DEBUG("send registration success status to client\n");
	        }
	        else
	        {
	            PARODUS_ERROR("send registration failed\n");
	        }
        }
	byte = 0;
	size = 0;
	free(auth_bytes);
	auth_bytes = NULL;
}
     
     
static int deleteFromList(char* service_name)
{
 	reg_list_item_t *prev_node = NULL, *curr_node = NULL;

	if( NULL == service_name ) 
	{
		PARODUS_ERROR("Invalid value for service\n");
		return -1;
	}
	PARODUS_INFO("service to be deleted: %s\n", service_name);

	prev_node = NULL;
	curr_node = head;	

	// Traverse to get the link to be deleted
	while( NULL != curr_node )
	{
		if(strcmp(curr_node->service_name, service_name) == 0)
		{
			PARODUS_DEBUG("Found the node to delete\n");
			if( NULL == prev_node )
			{
				PARODUS_DEBUG("need to delete first client\n");
			 	head = curr_node->next;
			}
			else
			{
				PARODUS_DEBUG("Traversing to find node\n");
			 	prev_node->next = curr_node->next;
			}
			
			PARODUS_DEBUG("Deleting the node\n");
			free( curr_node );
			curr_node = NULL;
			PARODUS_INFO("Deleted successfully and returning..\n");
			numOfClients =numOfClients - 1;
			PARODUS_DEBUG("numOfClients after delte is %d\n", numOfClients);
			return 0;
		}
		
		prev_node = curr_node;
		curr_node = curr_node->next;
	}
	PARODUS_ERROR("Could not find the entry to delete from list\n");
	return -1;
}
