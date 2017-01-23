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
#include "parodus_log.h"
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
static void deleteFromList(char* service_name);
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
  	    ParodusPrint("%s\n", log_msg);
	}
	if (level == NOPOLL_LEVEL_INFO) 
	{
		ParodusInfo ("%s\n", log_msg);
	}
	if (level == NOPOLL_LEVEL_WARNING) 
	{
  	     ParodusPrint("%s\n", log_msg);
	}
	if (level == NOPOLL_LEVEL_CRITICAL) 
	{
  	     ParodusError("%s\n", log_msg );
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
    ParodusInfo("formatted parodus Url %s\n",parodus_url);
	
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
			
	ParodusPrint("Configure nopoll thread handlers in Parodus\n");
	
	nopoll_thread_handlers(&createMutex, &destroyMutex, &lockMutex, &unlockMutex);

	ctx = nopoll_ctx_new();
	if (!ctx) 
	{
		ParodusError("\nError creating nopoll context\n");
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
				ParodusError("ping wait time > %d. Terminating the connection with WebPA server and retrying\n",
                                        get_parodus_cfg()->webpa_ping_timeout);
							
				reconnect_reason = "Ping_Miss";
				LastReasonStatus = true;
				pthread_mutex_lock (&close_mut);
				close_retry = true;
				pthread_mutex_unlock (&close_mut);
			}
			else
			{			
				ParodusPrint("heartBeatHandler - close_retry set to %d, hence resetting the heartBeatTimer\n",close_retry);
			}
			heartBeatTimer = 0;
		}
		else if(intTimer >= 30)
		{

			ParodusPrint("heartBeatTimer %d\n",heartBeatTimer);
			heartBeatTimer += HEARTBEAT_RETRY_SEC;	
			intTimer = 0;		
		}
		
		
		if(close_retry)
		{
			ParodusInfo("close_retry is %d, hence closing the connection and retrying\n", close_retry);
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

	ParodusPrint("******** Start of handle_upstream ********\n");
	
	UpStreamMsg *message;
	int sock;
	int bytes =0;
	void *buf;
		
		
	sock = nn_socket( AF_SP, NN_PULL );
	nn_bind(sock, parodus_url );
	
	
	while( 1 ) 
	{
		
		buf = NULL;
		ParodusInfo("nanomsg server gone into the listening mode...\n");
		
		bytes = nn_recv (sock, &buf, NN_MSG, 0);
			
		ParodusInfo ("Upstream message received from nanomsg client: \"%s\"\n", (char*)buf);
		
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
				
				ParodusPrint("Producer added message\n");
			 	pthread_cond_signal(&nano_con);
				pthread_mutex_unlock (&nano_mut);
				ParodusPrint("mutex unlock in producer thread\n");
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
			ParodusError("failure in allocation for message\n");
		}
				
	}
	ParodusPrint ("End of handle_upstream\n");
	return 0;
}


static void *handleUpStreamEvents()
{		
	int rv=-1;	
	int msgType;
	wrp_msg_t *msg;	
	void *appendData;
	size_t encodedSize;
	reg_list_item_t *temp;
	int matchFlag = 0;
	
		
	while(1)
	{
		pthread_mutex_lock (&nano_mut);
		ParodusPrint("mutex lock in consumer thread\n");
		
		if(UpStreamMsgQ != NULL)
		{
			UpStreamMsg *message = UpStreamMsgQ;
			UpStreamMsgQ = UpStreamMsgQ->next;
			pthread_mutex_unlock (&nano_mut);
			ParodusPrint("mutex unlock in consumer thread\n");
			
			if (!terminated) 
			{
				
				/*** Decoding Upstream Msg to check msgType ***/
				/*** For MsgType 9 Perform Nanomsg client Registration else Send to server ***/	
				
				ParodusPrint("---- Decoding Upstream Msg ----\n");
								
				rv = wrp_to_struct( message->msg, message->len, WRP_BYTES, &msg );
				
				if(rv > 0)
				{
				
				   msgType = msg->msg_type;				   
				
				   if(msgType == 9)
				   {
					ParodusInfo("\n Nanomsg client Registration for Upstream\n");
					
					//Extract serviceName and url & store it in a linked list for reg_clients
					
					if(numOfClients !=0)
					{
					    temp = head;
					    do
					    {
						if(strcmp(temp->service_name, msg->u.reg.service_name)==0)
						{
							ParodusInfo("match found, client is already registered\n");
							strcpy(temp->url,msg->u.reg.url);
							nn_shutdown(temp->sock, 0);

							temp->sock = nn_socket( AF_SP, NN_PUSH );
							int t = 20000;
							nn_setsockopt(temp->sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
							nn_connect(temp->sock, msg->u.reg.url); 
							
							ParodusInfo("Client registered before. Sending acknowledgement \n"); 
			
							sendAuthStatus(temp);
							matchFlag = 1;
							
							break;
						}
						
						ParodusPrint("checking the next item in the list\n");
						temp= temp->next;
						
					     }while(temp !=NULL);

					}
					
					ParodusPrint("matchFlag is :%d\n", matchFlag);

					if((matchFlag == 0) || (numOfClients == 0))
					{
					    numOfClients = numOfClients + 1;
					    ParodusPrint("Adding nanomsg clients to list\n");
			                    addToList(&msg);
					    
					}
				    }
				    else
				    {
				    	//Sending to server for msgTypes 3, 4, 5, 6, 7, 8.			
					ParodusInfo("\n Received upstream data with MsgType: %d\n", msgType);   					
					//Appending metadata with packed msg received from client
					
					if(metaPackSize > 0)
					{
					   	ParodusPrint("Appending received msg with metadata\n");
					   	encodedSize = appendEncodedData( &appendData, message->msg, message->len, metadataPack, metaPackSize );
					   	ParodusPrint("encodedSize after appending :%zu\n", encodedSize);
					   	ParodusPrint("metadata appended upstream msg %s\n", (char *)appendData);
					   
						ParodusInfo("Sending metadata appended upstream msg to server\n");
					   	handleUpstreamMessage(g_conn,appendData, encodedSize);
					   	
						free( appendData);
						appendData =NULL;
					}
					
					else
					{		
						ParodusError("Failed to send upstream as metadata packing is not successful\n");
					}
				    }
				}
				else
				{
					ParodusError("Error in msgpack decoding for upstream\n");
				}
				ParodusPrint("Free for upstream decoded msg\n");
			        wrp_free_struct(msg);
			}
		
			
			nn_freemsg (message->msg);
			free(message);
			message = NULL;
		}
		else
		{
			ParodusPrint("Before pthread cond wait in consumer thread\n");   
			pthread_cond_wait(&nano_con, &nano_mut);
			pthread_mutex_unlock (&nano_mut);
			ParodusPrint("mutex unlock in consumer thread after cond wait\n");
			if (terminated) {
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
		ParodusPrint("mutex lock in consumer thread\n");
		if(ParodusMsgQ != NULL)
		{
			ParodusMsg *message = ParodusMsgQ;
			ParodusMsgQ = ParodusMsgQ->next;
			pthread_mutex_unlock (&g_mutex);
			ParodusPrint("mutex unlock in consumer thread\n");
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
			ParodusPrint("Before pthread cond wait in consumer thread\n");   
			pthread_cond_wait(&g_cond, &g_mutex);
			pthread_mutex_unlock (&g_mutex);
			ParodusPrint("mutex unlock in consumer thread after cond wait\n");
			if (terminated) 
			{
				break;
			}
		}
	}
	
	ParodusPrint ("Ended messageHandlerTask\n");
	return 0;
}

/*
 * @brief To handle registered services to indicate that the service is still alive.
 */
static void *serviceAliveTask()
{
	void *svc_bytes;
	wrp_msg_t svc_alive_msg;
	int byte =0, size =0;
	reg_list_item_t *temp; 
	
	svc_alive_msg.msg_type = WRP_MSG_TYPE__SVC_ALIVE;	
	
	size = wrp_struct_to( &svc_alive_msg, WRP_BYTES, &svc_bytes );

	while(1)
	{
		ParodusInfo("serviceAliveTask: numOfClients registered is %d\n", numOfClients);
		if(numOfClients > 0)
		{
			//sending svc msg to all the clients every 30s
			temp = head;
			
			do
			{
				byte = nn_send (temp->sock, svc_bytes, size, 0);
				
				ParodusPrint("svc byte sent :%d\n", byte);
				if(byte == size)
				{
					ParodusPrint("Keep alive msg sent to client\n");
					ParodusInfo("temp->service_name: %s is alive\n",temp->service_name);
				}
				else
				{
					ParodusPrint("Failed to send keep alive msg to client\n");
					ParodusInfo("service %s is dead\n", temp->service_name);
					//need to delete this client service from list
					deleteFromList((char*)temp->service_name);
				}
				byte = 0;
				temp= temp->next;	    	
			
			}while(temp !=NULL);
			
		 	ParodusPrint("Waiting for 30s to send keep alive msg \n");
		 	sleep(KEEPALIVE_INTERVAL_SEC);
		 
	    	}
	    	else
	    	{
	    		ParodusInfo("No clients are registered, waiting ..\n");
	    		sleep(70);
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
          strncpy(cfg->hw_model, optarg,strlen(optarg));
          ParodusInfo("hw-model is %s\n",cfg->hw_model);
         break;
        
        case 's':
          strncpy(cfg->hw_serial_number,optarg,strlen(optarg));
          ParodusInfo("hw_serial_number is %s\n",cfg->hw_serial_number);
          break;

        case 'f':
          strncpy(cfg->hw_manufacturer, optarg,strlen(optarg));
          ParodusInfo("hw_manufacturer is %s\n",cfg->hw_manufacturer);
          break;

        case 'd':
           strncpy(cfg->hw_mac, optarg,strlen(optarg));
           ParodusInfo("hw_mac is %s\n",cfg->hw_mac);
          break;
        
        case 'r':
          strncpy(cfg->hw_last_reboot_reason, optarg,strlen(optarg));
          ParodusInfo("hw_last_reboot_reason is %s\n",cfg->hw_last_reboot_reason);
          break;

        case 'n':
          strncpy(cfg->fw_name, optarg,strlen(optarg));
          ParodusInfo("fw_name is %s\n",cfg->fw_name);
          break;

        case 'b':
          cfg->boot_time = atoi(optarg);
          ParodusInfo("boot_time is %d\n",cfg->boot_time);
          break;
       
         case 'u':
          strncpy(cfg->webpa_url, optarg,strlen(optarg));
          ParodusInfo("webpa_url is %s\n",cfg->webpa_url);
          break;
        
        case 'p':
          cfg->webpa_ping_timeout = atoi(optarg);
          ParodusInfo("webpa_ping_timeout is %d\n",cfg->webpa_ping_timeout);
          break;

        case 'o':
          cfg->webpa_backoff_max = atoi(optarg);
          ParodusInfo("webpa_backoff_max is %d\n",cfg->webpa_backoff_max);
          break;

        case 'i':
          strncpy(cfg->webpa_interface_used, optarg,strlen(optarg));
          ParodusInfo("webpa_inteface_used is %s\n",cfg->webpa_interface_used);
          break;

        case '?':
          /* getopt_long already printed an error message. */
          break;

        default:
           ParodusError("Enter Valid commands..\n");
          abort ();
        }
    }
  
 ParodusPrint("argc is :%d\n", argc);
 ParodusPrint("optind is :%d\n", optind);

  /* Print any remaining command line arguments (not options). */
  if (optind < argc)
    {
      ParodusPrint ("non-option ARGV-elements: ");
      while (optind < argc)
        ParodusPrint ("%s ", argv[optind++]);
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
	   	ParodusPrint("metadata appended upstream response %s\n", (char *)appendData);
	   	ParodusPrint("encodedSize after appending :%zu\n", encodedSize);
	   		   
		ParodusInfo("Sending response to server\n");
	   	handleUpstreamMessage(g_conn,appendData, encodedSize);
	   	
		free( appendData);
		appendData =NULL;
	}

	else
	{		
		ParodusError("Failed to send upstream as metadata packing is not successful\n");
	}

}


static void addToList( wrp_msg_t **msg)
{   
    //new_node indicates the new clients which needs to be added to list
    
    reg_list_item_t *temp; 
    reg_list_item_t *new_node; 
    new_node=(reg_list_item_t *)malloc(sizeof(reg_list_item_t));
 
    new_node->sock = nn_socket( AF_SP, NN_PUSH );
    ParodusPrint("new_node->sock is %d\n", new_node->sock);
    nn_connect(new_node->sock, (*msg)->u.reg.url);
    
    int t = 20000;
    nn_setsockopt(new_node->sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
    
    ParodusPrint("(*msg)->u.reg.service_name is %s\n", (*msg)->u.reg.service_name);
    ParodusPrint("(*msg)->u.reg.url is %s\n", (*msg)->u.reg.url);
    
    strcpy(new_node->service_name, (*msg)->u.reg.service_name);
    strcpy(new_node->url, (*msg)->u.reg.url);
    
    new_node->next=NULL;
	 
    if (head== NULL) //adding first client
    {
        ParodusInfo("Adding first client to list\n");
    	head=new_node;
    	
    }
    else   //client2 onwards           
    {
    	ParodusInfo("Adding clients to list\n");
    	temp=head;
    	
    	while(temp->next !=NULL)
    	{
    		temp=temp->next;
    	}
    
    
    	temp->next=new_node;
    }
    
    ParodusPrint("client is added to list\n");
    ParodusInfo("client service %s is added to list with url: %s\n", new_node->service_name, new_node->url);
    
    
    if((strcmp(new_node->service_name, (*msg)->u.reg.service_name)==0)&& (strcmp(new_node->url, (*msg)->u.reg.url)==0))
    {
    	ParodusInfo("sending auth status to reg client\n");
    	sendAuthStatus(new_node);
    }
    else
    {
    	ParodusError("nanomsg client registration failed\n");
    }
    
}


static void sendAuthStatus(reg_list_item_t *new_node)
{
	int byte = 0;	
	int size=0;
	void *auth_bytes;
	wrp_msg_t auth_msg_var;
	
	auth_msg_var.msg_type = WRP_MSG_TYPE__AUTH;
	auth_msg_var.u.auth.status = 200;
	
	//Sending success status to clients after each nanomsg registration
	size = wrp_struct_to(&auth_msg_var, WRP_BYTES, &auth_bytes );

	ParodusInfo("Client %s Registered successfully. Sending Acknowledgement... \n ", new_node->service_name);

	byte = nn_send (new_node->sock, auth_bytes, size, 0);

	if(byte >=0)
	{
	    ParodusPrint("send registration success status to client\n");
	}
	else
	{
	    ParodusError("send registration failed\n");
	}

	byte = 0;
	size = 0;
	free(auth_bytes);
	auth_bytes = NULL;
}
     
     
static void deleteFromList(char* service_name)
{
 	reg_list_item_t *prev_node = NULL, *curr_node = NULL;

	if( NULL == service_name ) 
	{
		ParodusInfo("Invalid value for service\n");
		return;
	}
	ParodusInfo("service to be deleted: %s\n", service_name);

	prev_node = NULL;
	curr_node = head;	

	// Traverse to get the link to be deleted
	while( NULL != curr_node )
	{
		if(strcmp(curr_node->service_name, service_name) == 0)
		{
			ParodusPrint("Found the node to delete\n");
			if( NULL == prev_node )
			{
				ParodusPrint("need to delete first client\n");
			 	head = curr_node->next;
			}
			else
			{
				ParodusPrint("Traversing to find node\n");
			 	prev_node->next = curr_node->next;
			}
			
			ParodusPrint("Deleting the node\n");
			free( curr_node );
			curr_node = NULL;
			ParodusInfo("Deleted successfully and returning..\n");
			numOfClients =numOfClients - 1;
			break;
		}
		
		prev_node = curr_node;
		curr_node = curr_node->next;
	}
	return;
}
