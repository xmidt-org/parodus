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
#include <sys/time.h>
#include <sys/sysinfo.h>
#include "wss_mgr.h"
#include <pthread.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <wrp-c.h>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>


/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/


/* WebPA default Config */
#define WEBPA_SERVER_URL                                "talaria-beta.webpa.comcast.net"
#define WEBPA_SERVER_PORT                               8080
#define WEBPA_RETRY_INTERVAL_SEC                        10
#define WEBPA_MAX_PING_WAIT_TIME_SEC                    180

#define HTTP_CUSTOM_HEADER_COUNT                    	4
#define WEBPA_MESSAGE_HANDLE_INTERVAL_MSEC          	250
#define HEARTBEAT_RETRY_SEC                         	30      /* Heartbeat (ping/pong) timeout in seconds */
#define PARODUS_UPSTREAM "tcp://127.0.0.1:6666"


#define IOT "iot"
#define HARVESTER "harvester"
#define GET_SET "get_set"

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

typedef struct ParodusMsg__
{
	noPollMsg * msg;
	void * payload;
	size_t len;
	struct ParodusMsg__ *next;
} ParodusMsg;

typedef struct UpStreamMsg__
{
	void *msg;
	size_t len;
	struct UpStreamMsg__ *next;
} UpStreamMsg;

typedef struct reg_client__
{
	int sock;
	char service_name[32];
	char url[100];
} reg_client;

static int numOfClients = 0;
//Currently set the max mumber of clients as 10
reg_client *clients[10];


/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

static ParodusCfg parodusCfg;
static noPollCtx *ctx = NULL;
static noPollConn *conn = NULL;

static char deviceMAC[32]={'\0'}; 
static volatile unsigned int heartBeatTimer = 0;
static volatile bool terminated = false;
static bool close_retry = false;
static bool LastReasonStatus = false;
char *reconnect_reason = NULL;
static ParodusMsg *ParodusMsgQ = NULL;
static UpStreamMsg *UpStreamMsgQ = NULL;
static void __close_and_unref_connection__(noPollConn *conn);

pthread_mutex_t mut=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t close_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t nano_prod_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t nano_cons_mut=PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t con=PTHREAD_COND_INITIALIZER;
pthread_cond_t close_con=PTHREAD_COND_INITIALIZER;
pthread_cond_t nano_con=PTHREAD_COND_INITIALIZER;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static char createNopollConnection();
static void listenerOnMessage( void * msg, size_t msgSize);
static void listenerOnPingMessage (noPollCtx * ctx, noPollConn * conn, noPollMsg * msg, noPollPtr user_data);
static void listenerOnCloseMessage (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data);
void getCurrentTime(struct timespec *timer);
uint64_t getCurrentTimeInMicroSeconds(struct timespec *timer);
long timeValDiff(struct timespec *starttime, struct timespec *finishtime);

static int checkHostIp(char * serverIP);
static void loadParodusCfg(ParodusCfg * config,ParodusCfg *cfg);
static void listenerOnMessage_queue(noPollCtx * ctx, noPollConn * conn, noPollMsg * msg,noPollPtr user_data);
static void initMessageHandler();
static void *messageHandlerTask();
static void __report_log (noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data);
void parStrncpy(char *destStr, const char *srcStr, size_t destSize);

noPollPtr createMutex();
void lockMutex(noPollPtr _mutex);
void unlockMutex(noPollPtr _mutex);
void destroyMutex(noPollPtr _mutex);
int __attribute__ ((weak)) checkDeviceInterface();

static void initUpStreamTask();
static void *handle_upstream();
static void processUpStreamTask();
static void *processUpStreamHandler();
static void handleUpStreamEvents();
static void handleUpstreamMessage(void *msg, size_t len);

/**
 * @brief __report_log Nopoll log handler 
 * Nopoll log handler for integrating nopoll logs 
 */
static void __report_log (noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data)
{
	if (level == NOPOLL_LEVEL_DEBUG) 
	{
  	     printf("Debug: %s\n", log_msg);
	}
	if (level == NOPOLL_LEVEL_INFO) 
	{
		printf("Info: %s\n", log_msg);
	}
	if (level == NOPOLL_LEVEL_WARNING) 
	{
  	     printf("Warning: %s\n", log_msg);
	}
	if (level == NOPOLL_LEVEL_CRITICAL) 
	{
  	     printf("Error: %s\n", log_msg);
	}
	return;
}

void __close_and_unref_connection__(noPollConn *conn)
{
    if (conn) {
        nopoll_conn_close(conn);
        if (0 < nopoll_conn_ref_count (conn)) {
            nopoll_conn_unref(conn);
        }
    }
}


void __createSocketConnection(void *config_in, void (* initKeypress)())

{
    ParodusCfg *tmpCfg = (ParodusCfg*)config_in;
   loadParodusCfg(tmpCfg,&parodusCfg);
		
	printf("Configure nopoll thread handlers in Parodus \n");
	nopoll_thread_handlers(&createMutex, &destroyMutex, &lockMutex, &unlockMutex);

	ctx = nopoll_ctx_new();
	if (!ctx) 
	{
		printf("\nError creating nopoll context\n");
	}

	#ifdef NOPOLL_LOGGER
  		nopoll_log_set_handler (ctx, __report_log, NULL);
	#endif

	createNopollConnection();
	
	initUpStreamTask();
	processUpStreamTask();
		
	initMessageHandler();
	
	if (NULL != initKeypress) 
	{
  		(* initKeypress) ();
	}
	int intTimer=0;
	do
	{
		
		nopoll_loop_wait(ctx, 5000000);
		
		intTimer = intTimer + 5;
		
		if(heartBeatTimer >= parodusCfg.webpa_ping_timeout) 
		{
			if(!close_retry) 
			{
				printf("ping wait time > %d. Terminating the connection with WebPA server and retrying\n", parodusCfg.webpa_ping_timeout);
							
				reconnect_reason = "Ping_Miss";
				LastReasonStatus = true;
				pthread_mutex_lock (&close_mut);
				close_retry = true;
				pthread_mutex_unlock (&close_mut);
			}
			else
			{
				printf("heartBeatHandler - close_retry set to %d, hence resetting the heartBeatTimer\n",close_retry);
			}
			heartBeatTimer = 0;
		}
		else if(intTimer >= 30)
		{
			printf("heartBeatTimer %d\n",heartBeatTimer);
			heartBeatTimer += HEARTBEAT_RETRY_SEC;	
			intTimer = 0;		
		}
		
		
		if(close_retry)
		{
			printf("close_retry is %d, hence closing the connection and retrying\n", close_retry);
			__close_and_unref_connection__(conn);
			conn = NULL;
			createNopollConnection();
		}		
	} while(!close_retry);
	  	
	__close_and_unref_connection__(conn);
	nopoll_ctx_unref(ctx);
	nopoll_cleanup_library();
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/**
 * @brief Interface to create WebSocket client connections.
 * Loads the WebPA config file and creates the intial connection and manages the connection wait, close mechanisms.
 */
void createSocketConnection()
{
	__createSocketConnection (NULL, NULL);
}

/**
 * @brief Interface to terminate WebSocket client connections and clean up resources.
 */
void terminateSocketConnection()
{
	int i;
	terminated = true;
	LastReasonStatus = true;
	
	for (i=0; i<15; i++) 
	{
		pthread_mutex_lock (&mut);		
		if(ParodusMsgQ == NULL)
		{
		 	pthread_cond_signal(&con);
			pthread_mutex_unlock (&mut);
			break;
		}
		pthread_mutex_unlock (&mut);
	}
	nopoll_loop_stop(ctx);
	// Wait for nopoll_loop_wait to finish
	for (i=0; i<15; i++) 
	{
		sleep(2);
		if (nopoll_loop_ended(ctx))
			break;
	}
	__close_and_unref_connection__(conn);	
	nopoll_ctx_unref(ctx);
	nopoll_cleanup_library();
}

static void loadParodusCfg(ParodusCfg * config,ParodusCfg *cfg)
{
    ParodusCfg *pConfig =config;
    
    if(strlen (pConfig->hw_model) !=0)
    {
        strncpy(cfg->hw_model, pConfig->hw_model,strlen(pConfig->hw_model)+1);
        
    }
    else
    {
        printf("hw_model is NULL. read from tmp file\n");
    }
    if( strlen(pConfig->hw_serial_number) !=0)
    {
        strncpy(cfg->hw_serial_number, pConfig->hw_serial_number,strlen(pConfig->hw_serial_number)+1);
    }
    else
    {
        printf("hw_serial_number is NULL. read from tmp file\n");
    }
    if(strlen(pConfig->hw_manufacturer) !=0)
    {
        strncpy(cfg->hw_manufacturer, pConfig->hw_manufacturer,strlen(pConfig->hw_manufacturer)+1);
    }
    else
    {
        printf("hw_manufacturer is NULL. read from tmp file\n");
    }
    if(strlen(pConfig->hw_mac) !=0)
    {
       strncpy(cfg->hw_mac, pConfig->hw_mac,strlen(pConfig->hw_mac)+1);
    }
    else
    {
        printf("hw_mac is NULL. read from tmp file\n");
    }
    if(strlen (pConfig->hw_last_reboot_reason) !=0)
    {
         strncpy(cfg->hw_last_reboot_reason, pConfig->hw_last_reboot_reason,strlen(pConfig->hw_last_reboot_reason)+1);
    }
    else
    {
        printf("hw_last_reboot_reason is NULL. read from tmp file\n");
    }
    if(strlen(pConfig->fw_name) !=0)
    {   
        strncpy(cfg->fw_name, pConfig->fw_name,strlen(pConfig->fw_name)+1);
    }
    else
    {
        printf("fw_name is NULL. read from tmp file\n");
    }
    if( strlen(pConfig->webpa_url) !=0)
    {
        strncpy(cfg->webpa_url, pConfig->webpa_url,strlen(pConfig->webpa_url)+1);
    }
    else
    {
        printf("webpa_url is NULL. read from tmp file\n");
    }
    if(strlen(pConfig->webpa_interface_used )!=0)
    {
        strncpy(cfg->webpa_interface_used, pConfig->webpa_interface_used,strlen(pConfig->webpa_interface_used)+1);
    }
    else
    {
        printf("webpa_interface_used is NULL. read from tmp file\n");
    }
        
    cfg->boot_time = pConfig->boot_time;
    cfg->webpa_ping_timeout = pConfig->webpa_ping_timeout;
    cfg->webpa_backoff_max = pConfig->webpa_backoff_max;
      
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/** 
 * @brief createMutex Nopoll create mutex handler
 */ 
noPollPtr createMutex()
{
  	pthread_mutexattr_t attr;
	pthread_mutex_t * mutex;
  	int rtn;
	mutex = nopoll_new (pthread_mutex_t, 1);
	if (mutex == NULL)
	{
		printf("Failed to create mutex\n");
		return NULL;
	}
	pthread_mutexattr_init( &attr);
	/*pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK);*/
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE);
	
	/* init the mutex using default values */
	rtn = pthread_mutex_init (mutex, &attr);
  	pthread_mutexattr_destroy (&attr);
	if (rtn != 0) 
	{
		printf("Error in init Mutex\n");
		nopoll_free(mutex);
		return NULL;
	}
	else 
	{
		printf("mutex init successfully\n");
	} 

	return mutex;
}

/** 
 * @brief lockMutex Nopoll lock mutex handler
 */
void lockMutex(noPollPtr _mutex)
{
	int rtn;
	char errbuf[100];
	/* printf("Inside Lock mutex\n"); */
	if (_mutex == NULL)
	{
		printf("Received null mutex\n");
		return;
	}
	pthread_mutex_t * mutex = _mutex;

	/* lock the mutex */
	rtn = pthread_mutex_lock (mutex);
	if (rtn != 0) 
	{
		strerror_r (rtn, errbuf, 100);
		printf("Error in Lock mutex: %s\n", errbuf);
		/* do some reporting */
		return;
	}
	else
	{
		/* printf("Mutex locked \n"); */
	} /* end if */
	return;
}

/** 
 * @brief unlockMutex Nopoll unlock mutex handler
 */
void unlockMutex(noPollPtr _mutex)
{
	int rtn;
	char errbuf[100];
	/* printf("Inside unlock mutex\n"); */
	if (_mutex == NULL)
	{
		printf("Received null mutex\n");
		return;
	}
	pthread_mutex_t * mutex = _mutex;

	/* unlock mutex */
	rtn = pthread_mutex_unlock (mutex);
	if (rtn != 0) 
	{
		/* do some reporting */
		strerror_r (rtn, errbuf, 100);
		printf("Error in unlock mutex: %s\n", errbuf);
		return;
	}
	else
	{
		/* printf("Mutex unlocked \n"); */
	} /* end if */
	return;
}

/** 
 * @brief destroyMutex Nopoll destroy mutex handler
 */
void destroyMutex(noPollPtr _mutex)
{
	if (_mutex == NULL)
	{
		printf("Received null mutex\n");
		return;
	}
	pthread_mutex_t * mutex = _mutex;
	
	if (pthread_mutex_destroy (mutex) != 0) 
	{
		/* do some reporting */
		printf("problem in destroy\n");
		return;
	}
	else
	{
		printf("Mutex destroyed \n");
	}
	nopoll_free (mutex);

	return;
}

/**
 * @brief createNopollConnection interface to create WebSocket client connections.
 *Loads the WebPA config file and creates the intial connection and manages the connection wait, close mechanisms.
 */
static char createNopollConnection()
{
	bool initial_retry = false;
	bool temp_retry = false;
	char device_id[32]={'\0'};
	char user_agent[512]={'\0'};
	const char * headerNames[HTTP_CUSTOM_HEADER_COUNT] = {"X-WebPA-Device-Name","X-WebPA-Device-Protocols","User-Agent", "X-WebPA-Convey"};
	const char * headerValues[HTTP_CUSTOM_HEADER_COUNT];
        int headerCount = HTTP_CUSTOM_HEADER_COUNT; /* Invalid X-Webpa-Convey header Bug # WEBPA-787 */
	char port[8];
	noPollConnOpts * opts;
	char server_Address[256];
	char redirectURL[128]={'\0'};
	char *temp_ptr;
	cJSON *resParamObj1 = NULL ,*resParamObj2 = NULL,*resParamObj3 = NULL ,*resParamObj4 = NULL , *parameters = NULL;
	cJSON *response = cJSON_CreateObject();
	char *buffer = NULL, *firmwareVersion = NULL, *modelName = NULL, *manufacturer = NULL;
	
	char *reboot_reason = NULL;
	char encodedData[1024];
	int  encodedDataSize = 1024;
	int i=0, j=0, connErr=0;
	int bootTime_sec;
	
	struct timespec start,end,connErr_start,connErr_end,*startPtr,*endPtr,*connErr_startPtr,*connErr_endPtr;
	startPtr = &start;
	endPtr = &end;
	connErr_startPtr = &connErr_start;
	connErr_endPtr = &connErr_end;
	strcpy(deviceMAC, parodusCfg.hw_mac);
	snprintf(device_id, sizeof(device_id), "mac:%s", deviceMAC);
	printf("Device_id %s\n",device_id);

	headerValues[0] = device_id;
	headerValues[1] = "wrp-0.11,getset-0.1";    
	
	
	bootTime_sec = parodusCfg.boot_time;
	printf("BootTime In sec: %d\n", bootTime_sec);
	firmwareVersion = parodusCfg.fw_name;
    modelName = parodusCfg.hw_model;
    manufacturer = parodusCfg.hw_manufacturer;
    	snprintf(user_agent, sizeof(user_agent),
             "WebPA-1.6 (%s; %s/%s;)",
             ((0 != strlen(firmwareVersion)) ? firmwareVersion : "unknown"),
             ((0 != strlen(modelName)) ? modelName : "unknown"),
             ((0 != strlen(manufacturer)) ? manufacturer : "unknown"));

	printf("User-Agent: %s\n",user_agent);
	headerValues[2] = user_agent;
	reconnect_reason = "webpa_process_starts";	
	printf("Received reconnect_reason as:%s\n", reconnect_reason);
	reboot_reason = parodusCfg.hw_last_reboot_reason;
	printf("Received reboot_reason as:%s\n", reboot_reason);
	
	if(firmwareVersion != NULL )
	{
		cJSON_AddItemToObject(response, "parameters", parameters = cJSON_CreateArray());
		
		cJSON_AddItemToArray(parameters, resParamObj1 = cJSON_CreateObject());
		cJSON_AddStringToObject(resParamObj1, "name", "FirmwareVersion");
		cJSON_AddStringToObject(resParamObj1, "value", firmwareVersion);
		cJSON_AddNumberToObject(resParamObj1, "dataType", 0);
		
		cJSON_AddItemToArray(parameters, resParamObj2 = cJSON_CreateObject());
		cJSON_AddStringToObject(resParamObj2, "name", "BootTime");
		cJSON_AddNumberToObject(resParamObj2, "value", bootTime_sec);
		cJSON_AddNumberToObject(resParamObj2, "dataType", 2);
		
		if(reconnect_reason !=NULL)
		{
			cJSON_AddItemToArray(parameters, resParamObj3 = cJSON_CreateObject());
			cJSON_AddStringToObject(resParamObj3, "name", "Reconnect Reason");
			cJSON_AddStringToObject(resParamObj3, "value", reconnect_reason);
			cJSON_AddNumberToObject(resParamObj3, "dataType", 0);
		}
		else
		{
		     	printf("Failed to GET Reconnect reason value\n");
		}
		if(reboot_reason !=NULL)
		{
			
			cJSON_AddItemToArray(parameters, resParamObj4 = cJSON_CreateObject());
			cJSON_AddStringToObject(resParamObj4, "name", "Reboot reason");
			cJSON_AddStringToObject(resParamObj4, "value", reboot_reason);
			cJSON_AddNumberToObject(resParamObj4, "dataType", 0);
		}
		else
		{
			printf("Failed to GET Reboot reason value\n");
		}
		

		buffer = cJSON_PrintUnformatted(response);
		printf("X-WebPA-Convey Header: [%zd]%s\n", strlen(buffer), buffer);

		if(nopoll_base64_encode (buffer, strlen(buffer), encodedData, &encodedDataSize) != nopoll_true)
		{
			printf("Base64 Encoding failed for Connection Header\n");
			headerValues[3] = ""; 
                        headerCount -= 1; 
		}
		else
		{
			/* Remove \n characters from the base64 encoded data */
			for(i=0;encodedData[i] != '\0';i++)
			{
				if(encodedData[i] == '\n')
				{
					printf("New line is present in encoded data at position %d\n",i);
				}
				else
				{
					encodedData[j] = encodedData[i];
					j++;
				}
			}
			encodedData[j]='\0';
			headerValues[3] = encodedData;
			printf("Encoded X-WebPA-Convey Header: [%zd]%s\n", strlen(encodedData), encodedData);
		}
	}
	else
	{
                headerValues[3] = ""; 
                headerCount -= 1; 
                
	}
	
	cJSON_Delete(response);
	
	
	snprintf(port,sizeof(port),"%d",8080);
	parStrncpy(server_Address, parodusCfg.webpa_url, sizeof(server_Address));
	printf("server_Address %s\n",server_Address);
	
	do
	{
		/* Check if device interface is up and has IP then proceed with connection else wait till interface is up.
		 * Interface status will be checked during retry for all nopoll connect errors. This check will be done always except during redirect scenario. 
		 * As a fall back if interface is down for more than 15 minutes, try the connection as there might be some issue.
		 */
		if(!temp_retry)
		{
			getCurrentTime(startPtr);
			int count = 0;
			while(checkDeviceInterface() != 0)
			{
				getCurrentTime(endPtr);

				// If interface status check fails for more than 15 minutes, proceed with webpa connection.
				printf("Elapsed time for checking interface status is: %ld ms\n", timeValDiff(startPtr, endPtr));
				if(timeValDiff(startPtr, endPtr) >= (15*60*1000))
				{
					printf("Proceeding to webpa connection retry irrespective of interface status as interface is down for more than 15 minutes\n");
					break;
				}
				if(count == 0)
				{
					printf("Interface %s is down, hence waiting\n",parodusCfg.webpa_interface_used);
				}
				else if(count > 5)
					count = -1;

				sleep(10);
				count++;
			}
		}	
		parodusCfg.secureFlag = 1;	
		if(parodusCfg.secureFlag) 
		{
		    printf("secure true\n");
			/* disable verification */
			opts = nopoll_conn_opts_new ();
			nopoll_conn_opts_ssl_peer_verify (opts, nopoll_false);
			nopoll_conn_opts_set_ssl_protocol (opts, NOPOLL_METHOD_TLSV1_2); 
			conn = nopoll_conn_tls_new(ctx, opts, server_Address, port, NULL,
                               "/api/v2/device", NULL, NULL, parodusCfg.webpa_interface_used,
                                headerNames, headerValues, headerCount);// WEBPA-787
		}
		else 
		{
		    printf("secure false\n");
			conn = nopoll_conn_new(ctx, server_Address, port, NULL,
                               "/api/v2/device", NULL, NULL, parodusCfg.webpa_interface_used,
                                headerNames, headerValues, headerCount);// WEBPA-787
		}

		if(conn != NULL)
		{
			if(!nopoll_conn_is_ok(conn)) 
			{
				printf("Error connecting to server\n");
				printf("RDK-10037 - WebPA Connection Lost\n");
				// Copy the server address from config to avoid retrying to the same failing talaria redirected node
				parStrncpy(server_Address, parodusCfg.webpa_url, sizeof(server_Address));
				__close_and_unref_connection__(conn);
				conn = NULL;
				initial_retry = true;
				// temp_retry flag if made false to force interface status check
				temp_retry = false;
				sleep(10);
				continue;
			}
			else 
			{
				printf("Connected to Server but not yet ready\n");
				initial_retry = false;
				temp_retry = false;
			}

			if(!nopoll_conn_wait_until_connection_ready(conn, 10, redirectURL)) 
			{
				if (strncmp(redirectURL, "Redirect:", 9) == 0) // only when there is a http redirect
				{
					printf("Received temporary redirection response message %s\n", redirectURL);
					// Extract server Address and port from the redirectURL
					temp_ptr = strtok(redirectURL , ":"); //skip Redirect 
					temp_ptr = strtok(NULL , ":"); // skip https
					temp_ptr = strtok(NULL , ":");
					parStrncpy(server_Address, temp_ptr+2, sizeof(server_Address));
					parStrncpy(port, strtok(NULL , "/"), sizeof(port));
					printf("Trying to Connect to new Redirected server : %s with port : %s\n", server_Address, port);
				}
				else
				{
					printf("Client connection timeout\n");	
					printf("RDK-10037 - WebPA Connection Lost\n");
					// Copy the server address from config to avoid retrying to the same failing talaria redirected node
					parStrncpy(server_Address, parodusCfg.webpa_url, sizeof(server_Address));
					sleep(10);
				}
				__close_and_unref_connection__(conn);
				conn = NULL;
				initial_retry = true;
				// temp_retry flag if made true to avoid interface status check
				temp_retry = true;
			}
			else 
			{
				initial_retry = false;
				temp_retry = false;
				printf("Connection is ready\n");
			}
		}
		else
		{
			/* If the connect error is due to DNS resolving to 10.0.0.1 then start timer.
			 * Timeout after 15 minutes if the error repeats continuously and kill itself. 
			 */
			if((checkHostIp(server_Address) == -2) && (checkDeviceInterface() == 0)) 	
			{
				if(connErr == 0)
				{
					getCurrentTime(connErr_startPtr);
					connErr = 1;
					printf("First connect error occurred, initialized the connect error timer\n");
				}
				else
				{
					getCurrentTime(connErr_endPtr);
					printf("checking timeout difference:%ld\n", timeValDiff(connErr_startPtr, connErr_endPtr));
					if(timeValDiff(connErr_startPtr, connErr_endPtr) >= (15*60*1000))
					{
						printf("WebPA unable to connect due to DNS resolving to 10.0.0.1 for over 15 minutes; crashing service.\n");
						reconnect_reason = "Dns_Res_webpa_reconnect";
						LastReasonStatus = true;
						
						kill(getpid(),SIGTERM);						
					}
				}			
			}
			initial_retry = true;
			temp_retry = false;
			sleep(10);
			// Copy the server address from config to avoid retrying to the same failing talaria redirected node
			parStrncpy(server_Address, parodusCfg.webpa_url, sizeof(server_Address));
		}
	}while(initial_retry);
	
	if(parodusCfg.secureFlag) 
	{
		printf("Connected to server over SSL\n");
	}
	else 
	{
		printf("Connected to server\n");
	}
	
	// Reset close_retry flag and heartbeatTimer once the connection retry is successful
	printf("createNopollConnection(): close_mutex lock\n");
	pthread_mutex_lock (&close_mut);
	close_retry = false;
	pthread_mutex_unlock (&close_mut);
	printf("createNopollConnection(): close_mutex unlock\n");
	heartBeatTimer = 0;

	// Reset connErr flag on successful connection
	connErr = 0;
	
	reconnect_reason = "webpa_process_starts";
	LastReasonStatus = true;

	nopoll_conn_set_on_msg(conn, (noPollOnMessageHandler) listenerOnMessage_queue, NULL);
	nopoll_conn_set_on_ping_msg(conn, (noPollOnMessageHandler)listenerOnPingMessage, NULL);
	nopoll_conn_set_on_close(conn, (noPollOnCloseHandler)listenerOnCloseMessage, NULL);
	return nopoll_true;
}

int checkDeviceInterface()
{
	int link[2];
	pid_t pid;
	char statusValue[512] = {'\0'};
	char *ipv4_valid =NULL, *addr_str = NULL, *ipv6_valid = NULL;

    int status = -3;
	
	if (pipe(link) == -1)
	{
		printf("Failed to create pipe for checking device interface (errno=%d, %s)\n",errno, strerror(errno));
		return -1;
	}

	if ((pid = fork()) == -1)
	{
		printf("fork was unsuccessful while checking device interface (errno=%d, %s)\n",errno, strerror(errno));
		return -2;
	}
	
	if(pid == 0) 
	{
		printf("child process created\n");
		pid = getpid();
		printf("child process execution with pid:%d\n", pid);
		dup2 (link[1], STDOUT_FILENO);
		close(link[0]);
		close(link[1]);	
		execl("/sbin/ifconfig", "ifconfig", parodusCfg.webpa_interface_used, (char *)0);
	}	
	else 
	{
		close(link[1]);
		read(link[0], statusValue, sizeof(statusValue)-1);
		
		printf("statusValue is :%s\n", statusValue);
		ipv4_valid = strstr(statusValue, "inet addr:");
		printf("ipv4_valid %s\n", (ipv4_valid != NULL) ? ipv4_valid : "NULL");
		addr_str = strstr(statusValue, "inet6 addr:");
		printf("addr_str %s\n", (addr_str != NULL) ? addr_str : "NULL");
		if(addr_str != NULL)
		{
			ipv6_valid = strstr(addr_str, "Scope:Global");
			printf("ipv6_valid %s\n", (ipv6_valid != NULL) ? ipv6_valid : "NULL");
		}

		if (ipv4_valid != NULL || ipv6_valid != NULL)
		{
			printf("Interface %s is up\n",parodusCfg.webpa_interface_used);
			status = 0;
		}
		else
		{
			printf("Interface %s is down\n",parodusCfg.webpa_interface_used);
		}
		
		if(pid == wait(NULL))
		{
			printf("child process pid %d terminated successfully\n", pid);
		}
		else
		{
			printf("Error reading wait status of child process pid %d, killing it\n", pid);
			kill(pid, SIGKILL);
		}
		close(link[0]);

	}
	printf("checkDeviceInterface:%d\n", status);
	return status;
}

/**
 * @brief checkHostIp interface to check if the Host server DNS is resolved to correct IP Address.
 * @param[in] serverIP server address DNS
 * Converts HostName to Host IP Address and checks if it is not same as 10.0.0.1 so that we can proceed with connection
 */
static int checkHostIp(char * serverIP)
{
	printf("...............Inside checkHostIp..............%s \n", serverIP);
	int status = -1;
	struct addrinfo *res, *result;
	int retVal;
	char addrstr[100];
	void *ptr;
	char *localIp = "10.0.0.1";

	struct addrinfo hints;
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;

	retVal = getaddrinfo(serverIP, "http", &hints, &result);
	if (retVal != 0) 
	{
		printf("getaddrinfo: %s\n", gai_strerror(retVal));
	}
	else
	{
		res = result;
		while(res)
		{  
			ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
			inet_ntop (res->ai_family, ptr, addrstr, 100);
			printf ("IPv4 address of %s is %s \n", serverIP, addrstr);
			if (strcmp(localIp,addrstr) == 0)
			{
				printf("Host Ip resolved to 10.0.0.1\n");
				status = -2;
			}
			else
			{
				printf("Host Ip resolved correctly, proceeding with the connection\n");
				status = 0;
				break;
			}
			res = res->ai_next;
		}
		freeaddrinfo(result);
	}
	return status; 
}    
       
 
 /*
 * @brief To initiate UpStream message handling
 */

static void initUpStreamTask()
{
	int err = 0;
	pthread_t UpStreamMsgThreadId;
	UpStreamMsgQ = NULL;

	err = pthread_create(&UpStreamMsgThreadId, NULL, handle_upstream, NULL);
	if (err != 0) 
	{
		printf("Error creating messages thread :[%s]\n", strerror(err));
	}
	else
	{
		printf("handle_upstream thread created Successfully\n");
	}
}

/*
 * @brief To handle UpStream messages which is received from nanomsg server socket
 */

static void *handle_upstream()
{

	printf("******** Start of handle_upstream ********\n");
	
	UpStreamMsg *message;
	int sock;
	int bytes =0;
	void *buf;
		
		
	sock = nn_socket( AF_SP, NN_PULL );
	nn_bind(sock, PARODUS_UPSTREAM );
	
	
	while( 1 ) 
	{
		
		buf = NULL;
		printf("nanomsg server gone into the listening mode...\n");
		
		bytes = nn_recv (sock, &buf, NN_MSG, 0);
			
		printf ("Upstream message received from nanomsg client: \"%s\"\n", (char*)buf);
		
		message = (UpStreamMsg *)malloc(sizeof(UpStreamMsg));
		
		if(message)
		{
			message->msg =buf;
			message->len =bytes;
			message->next=NULL;
			pthread_mutex_lock (&nano_prod_mut);
			
			//Producer adds the nanoMsg into queue
			if(UpStreamMsgQ == NULL)
			{
	
				UpStreamMsgQ = message;
				
				printf("Producer added message\n");
			 	pthread_cond_signal(&nano_con);
				pthread_mutex_unlock (&nano_prod_mut);
				printf("mutex unlock in producer thread\n");
			}
			else
			{
				UpStreamMsg *temp = UpStreamMsgQ;
				while(temp->next)
				{
					temp = temp->next;
				}
			
				temp->msg = buf;
				temp->len = bytes;
				temp->next = NULL;
			
				pthread_mutex_unlock (&nano_prod_mut);
			}
					
		}
		else
		{
			printf("failure in allocation for message\n");
		}
				
	}
	printf ("End of handle_upstream\n");
	return 0;
}


/*
 * @brief To process the received msg and to send upstream
 */

static void processUpStreamTask()
{
	int err = 0;
	pthread_t processUpStreamThreadId;
	UpStreamMsgQ = NULL;

	err = pthread_create(&processUpStreamThreadId, NULL, processUpStreamHandler, NULL);
	if (err != 0) 
	{
		printf("Error creating messages thread :[%s]\n", strerror(err));
	}
	else
	{
		printf("processUpStreamHandler thread created Successfully\n");
	}
}



static void *processUpStreamHandler()
{
	printf("Inside processUpStreamHandler..\n");
	handleUpStreamEvents();
	return 0;
}

static void handleUpStreamEvents()
{		
	int rv=-1;	
	int msgType;
	wrp_msg_t *msg;		
	int i =0;	
	int size=0;
	int matchFlag = 0;	
	int sock =0;
	int byte = 0;
	void *bytes;	
	void *auth_bytes;
	wrp_msg_t auth_msg_var;


	auth_msg_var.msg_type = WRP_MSG_TYPE__AUTH;
	auth_msg_var.u.auth.status = 200;
		
	while(1)
	{
		pthread_mutex_lock (&nano_cons_mut);
		printf("mutex lock in consumer thread\n");
		
		if(UpStreamMsgQ != NULL)
		{
			UpStreamMsg *message = UpStreamMsgQ;
			UpStreamMsgQ = UpStreamMsgQ->next;
			pthread_mutex_unlock (&nano_cons_mut);
			printf("mutex unlock in consumer thread\n");
			
			if (!terminated) 
			{
				
				/*** Decoding Upstream Msg to check msgType ***/
				/*** For MsgType 9 Perform Nanomsg client Registration else Send to server ***/	
				
				printf("---- Decoding Upstream Msg ----\n");
								
				rv = wrp_to_struct( message->msg, message->len, WRP_BYTES, &msg );
				
				if(rv > 0)
				{
				
				   msgType = msg->msg_type;				   
				
				   if(msgType == 9)
				   {
					printf("\n Nanomsg client Registration for Upstream\n");
					
					//Extract serviceName and url & store it in a struct for reg_clients
					
					if(numOfClients !=0)
					{
					    for( i = 0; i < numOfClients; i++ ) 
					    {
																						
						if(strcmp(clients[i]->service_name, msg->u.reg.service_name)==0)
						{
							printf("match found, client is already registered\n");
							strcpy(clients[i]->url,msg->u.reg.url);
							nn_shutdown(clients[i]->sock, 0);


							clients[i]->sock = nn_socket( AF_SP, NN_PUSH );
							int t = 20000;
							nn_setsockopt(clients[i]->sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
							
							nn_connect(clients[i]->sock, msg->u.reg.url);  
			
							
							size = wrp_struct_to( &auth_msg_var, WRP_BYTES, &auth_bytes );
							printf("Client registered before. Sending acknowledgement \n");
							byte = nn_send (clients[i]->sock, auth_bytes, size, 0);
						
							byte = 0;
							size = 0;
							matchFlag = 1;
							break;
						}
					    }

					}

					printf("matchFlag is :%d\n", matchFlag);

					if((matchFlag == 0) || (numOfClients == 0))
					{
					
						clients[numOfClients] = (reg_client*)malloc(sizeof(reg_client));

						clients[numOfClients]->sock = nn_socket( AF_SP, NN_PUSH );
						nn_connect(clients[numOfClients]->sock, msg->u.reg.url);  

						int t = 20000;
						nn_setsockopt(clients[numOfClients]->sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
						 
						strcpy(clients[numOfClients]->service_name,msg->u.reg.service_name);
						strcpy(clients[numOfClients]->url,msg->u.reg.url);

						printf("%s\n",clients[numOfClients]->service_name);
						printf("%s\n",clients[numOfClients]->url);
						
						if((strcmp(clients[numOfClients]->service_name, msg->u.reg.service_name)==0)&& (strcmp(clients[numOfClients]->url, msg->u.reg.url)==0))
					{


						//Sending success status to clients after each nanomsg registration
						size = wrp_struct_to(&auth_msg_var, WRP_BYTES, &auth_bytes );

						printf("Client %s Registered successfully. Sending Acknowledgement... \n ", clients[numOfClients]->service_name);

						byte = nn_send (clients[numOfClients]->sock, auth_bytes, size, 0);
						
						//condition needs to be changed depending upon acknowledgement
						if(byte >=0)
						{
							printf("send registration success status to client\n");
						}
						else
						{
							printf("send registration failed\n");
						}
						numOfClients =numOfClients+1;
						printf("Number of clients registered= %d\n", numOfClients);
						byte = 0;
						size = 0;
						free(auth_bytes);

						
					
					}
					else
					{
						printf("nanomsg client registration failed\n");
					}
					
					}
										      

				    }
				    else
				    {
				    	//Sending to server for msgTypes 3, 4, 5, 6, 7, 8.
					
			   					
					printf("\n Received upstream data with MsgType: %d\n", msgType);   					
    					printf("Sending upstream msg to server\n");
					handleUpstreamMessage(message->msg, message->len);
							        
				    
				    }
					
				}
				else
				{
					printf("Error in msgpack decoding for upstream\n");
				
				}
				
			
			}
		
			
			nn_freemsg (message->msg);
			free(message);
			message = NULL;
		}
		else
		{
			printf("Before pthread cond wait in consumer thread\n");   
			pthread_cond_wait(&nano_con, &nano_prod_mut);
			pthread_mutex_unlock (&nano_cons_mut);
			printf("mutex unlock in consumer thread after cond wait\n");
			if (terminated) {
				break;
			}
		}
	}

}
 
 
        
/*
 * @brief To initiate message handling
 */

static void initMessageHandler()
{
	int err = 0;
	pthread_t messageThreadId;
	ParodusMsgQ = NULL;

	err = pthread_create(&messageThreadId, NULL, messageHandlerTask, NULL);
	if (err != 0) 
	{
		printf("Error creating messages thread :[%s]\n", strerror(err));
	}
	else
	{
		printf("messageHandlerTask thread created Successfully\n");
	}
}

/*
 * @brief To handle messages
 */
static void *messageHandlerTask()
{
	while(1)
	{
		pthread_mutex_lock (&mut);
		printf("mutex lock in consumer thread\n");
		if(ParodusMsgQ != NULL)
		{
			ParodusMsg *message = ParodusMsgQ;
			ParodusMsgQ = ParodusMsgQ->next;
			pthread_mutex_unlock (&mut);
			printf("mutex unlock in consumer thread\n");
			if (!terminated) 
			{
				
				listenerOnMessage(message->payload, message->len);
			}
						
			nopoll_msg_unref(message->msg);
			free(message);
			message = NULL;
		}
		else
		{
			printf("Before pthread cond wait in consumer thread\n");   
			pthread_cond_wait(&con, &mut);
			pthread_mutex_unlock (&mut);
			printf("mutex unlock in consumer thread after cond wait\n");
			if (terminated) 
			{
				break;
			}
		}
	}
	
	printf ("Ended messageHandlerTask\n");
	return 0;
}

/**
 * @brief listenerOnMessage_queue function to add messages to the queue
 *
 * @param[in] ctx The context where the connection happens.
 * @param[in] conn The Websocket connection object
 * @param[in] msg The message received from server for various process requests
 * @param[out] user_data data which is to be sent
 */

static void listenerOnMessage_queue(noPollCtx * ctx, noPollConn * conn, noPollMsg * msg,noPollPtr user_data)
{
	ParodusMsg *message;

	if (terminated) 
	{
		return;
	}

	message = (ParodusMsg *)malloc(sizeof(ParodusMsg));

	if(message)
	{

		message->msg = msg;
		message->payload = (void *)nopoll_msg_get_payload (msg);
		message->len = nopoll_msg_get_payload_size (msg);
		message->next = NULL;

	
		nopoll_msg_ref(msg);
		
		pthread_mutex_lock (&mut);		
		printf("mutex lock in producer thread\n");
		
		if(ParodusMsgQ == NULL)
		{
			ParodusMsgQ = message;
			printf("Producer added message\n");
		 	pthread_cond_signal(&con);
			pthread_mutex_unlock (&mut);
			printf("mutex unlock in producer thread\n");
		}
		else
		{
			ParodusMsg *temp = ParodusMsgQ;
			while(temp->next)
			{
				temp = temp->next;
			}
			temp->next = message;
			pthread_mutex_unlock (&mut);
		}
	}
	else
	{
		//Memory allocation failed
		printf("Memory allocation is failed\n");
	}
	printf("*****Returned from listenerOnMessage_queue*****\n");
}

/**
 * @brief listenerOnMessage function to create WebSocket listener to receive connections
 *
 * @param[in] ctx The context where the connection happens.
 * @param[in] conn The Websocket connection object
 * @param[in] msg The message received from server for various process requests
 * @param[out] user_data data which is to be sent
 */
static void listenerOnMessage(void * msg, size_t msgSize)
{
	
	int rv =0;
	wrp_msg_t *message;
	char* destVal = NULL;
	char dest[32] = {'\0'};
	
	int msgType;
	int p =0;
	int bytes =0;
	int destFlag =0;	
	const char *recivedMsg = NULL;
	recivedMsg =  (const char *) msg;
	
	printf("Received msg from server:%s\n", recivedMsg);	
	if(recivedMsg!=NULL) 
	{
	
		/*** Decoding downstream recivedMsg to check destination ***/
		
		rv = wrp_to_struct(recivedMsg, msgSize, WRP_BYTES, &message);
				
		if(rv > 0)
		{
			printf("\nDecoded recivedMsg of size:%d\n", rv);
			msgType = message->msg_type;
			printf("msgType received:%d\n", msgType);
		
			if((message->u.req.dest !=NULL))
			{
				destVal = message->u.req.dest;
				strtok(destVal , "/");
				strcpy(dest,strtok(NULL , "/"));
				printf("Received downstream dest as :%s\n", dest);
			
				//Checking for individual clients & Sending to each client
				
				for( p = 0; p < numOfClients; p++ ) 
				{
				    // Sending message to registered clients
				    if( strcmp(dest, clients[p]->service_name) == 0) 
				    {  
				    	printf("sending to nanomsg client %s\n", dest);     
					bytes = nn_send(clients[p]->sock, recivedMsg, msgSize, 0);
					printf("sent downstream message '%s' to reg_client '%s'\n",recivedMsg,clients[p]->url);
					printf("downstream bytes sent:%d\n", bytes);
					destFlag =1;
			
				    } 
				    
				}
				
				if(destFlag ==0)
				{
					printf("Unknown dest:%s\n", dest);
				}
			
		  	 }
	  	}
	  	
	  	else
	  	{
	  		printf( "Failure in msgpack decoding for receivdMsg: rv is %d\n", rv );
	  	}
	  

        }
                
     
}


/**
 * @brief listenerOnPingMessage function to create WebSocket listener to receive heartbeat ping messages
 *
 * @param[in] ctx The context where the connection happens.
 * @param[in] conn Websocket connection object
 * @param[in] msg The ping message received from the server
 * @param[out] user_data data which is to be sent
 */
static void listenerOnPingMessage (noPollCtx * ctx, noPollConn * conn, noPollMsg * msg, noPollPtr user_data)
{
    noPollPtr payload = NULL;
	payload = (noPollPtr ) nopoll_msg_get_payload(msg);

	if ((payload!=NULL) && !terminated) 
	{
		printf("Ping received with payload %s, opcode %d\n",(char *)payload, nopoll_msg_opcode(msg));
		if (nopoll_msg_opcode(msg) == NOPOLL_PING_FRAME) 
		{
			nopoll_conn_send_frame (conn, nopoll_true, nopoll_true, NOPOLL_PONG_FRAME, strlen(payload), payload, 0);
			heartBeatTimer = 0;
			printf("Sent Pong frame and reset HeartBeat Timer\n");
		}
	}
}

static void listenerOnCloseMessage (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data)
{
		
	printf("listenerOnCloseMessage(): mutex lock in producer thread\n");
	
	if((user_data != NULL) && (strstr(user_data, "SSL_Socket_Close") != NULL) && !LastReasonStatus)
	{
		reconnect_reason = "Server_closed_connection";
		LastReasonStatus = true;
		
	}
	else if ((user_data == NULL) && !LastReasonStatus)
	{
		reconnect_reason = "Unknown";
	}

	pthread_mutex_lock (&close_mut);
	close_retry = true;
	pthread_mutex_unlock (&close_mut);
	printf("listenerOnCloseMessage(): mutex unlock in producer thread\n");

}


/*
 * @brief displays the current time.
 * @param[in] timer current time.
 */
void getCurrentTime(struct timespec *timer)
{
	clock_gettime(CLOCK_REALTIME, timer);
}
/*
 * @brief displays the current time in microseconds.
 * @param[in] timer current time.
 */

uint64_t getCurrentTimeInMicroSeconds(struct timespec *timer)
{
        uint64_t systime = 0;
	    clock_gettime(CLOCK_REALTIME, timer);       
        printf("timer->tv_sec : %lu\n",timer->tv_sec);
        printf("timer->tv_nsec : %lu\n",timer->tv_nsec);
        systime = (uint64_t)timer->tv_sec * 1000000L + timer->tv_nsec/ 1000;
        return systime;	
}

/*
 * @brief Returns the time difference between start and end time of request processed.
 * @param[in] starttime starting time of request processed.
 * @param[in] finishtime ending time of request processed.
 * @return msec.
 */
long timeValDiff(struct timespec *starttime, struct timespec *finishtime)
{
	long msec;
	msec=(finishtime->tv_sec-starttime->tv_sec)*1000;
	msec+=(finishtime->tv_nsec-starttime->tv_nsec)/1000000;
	return msec;
}



void parStrncpy(char *destStr, const char *srcStr, size_t destSize)
{
    strncpy(destStr, srcStr, destSize-1);
    destStr[destSize-1] = '\0';
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
          printf("hw-model is %s\n",cfg->hw_model);
         break;
        
        case 's':
          strncpy(cfg->hw_serial_number,optarg,strlen(optarg));
          printf("hw_serial_number is %s\n",cfg->hw_serial_number);
          break;

        case 'f':
          strncpy(cfg->hw_manufacturer, optarg,strlen(optarg));
          printf("hw_manufacturer is %s\n",cfg->hw_manufacturer);
          break;

        case 'd':
           strncpy(cfg->hw_mac, optarg,strlen(optarg));
           printf("hw_mac is %s\n",cfg->hw_mac);
          break;
        
        case 'r':
          strncpy(cfg->hw_last_reboot_reason, optarg,strlen(optarg));
          printf("hw_last_reboot_reason is %s\n",cfg->hw_last_reboot_reason);
          break;

        case 'n':
          strncpy(cfg->fw_name, optarg,strlen(optarg));
          printf("fw_name is %s\n",cfg->fw_name);
          break;

        case 'b':
          cfg->boot_time = atoi(optarg);
          printf("boot_time is %d\n",cfg->boot_time);
          break;
       
         case 'u':
          strncpy(cfg->webpa_url, optarg,strlen(optarg));
          printf("webpa_url is %s\n",cfg->webpa_url);
          break;
        
        case 'p':
          cfg->webpa_ping_timeout = atoi(optarg);
          printf("webpa_ping_timeout is %d\n",cfg->webpa_ping_timeout);
          break;

        case 'o':
          cfg->webpa_backoff_max = atoi(optarg);
          printf("webpa_backoff_max is %d\n",cfg->webpa_backoff_max);
          break;

        case 'i':
          strncpy(cfg->webpa_interface_used, optarg,strlen(optarg));
          printf("webpa_inteface_used is %s\n",cfg->webpa_interface_used);
          break;

        case '?':
          /* getopt_long already printed an error message. */
          break;

        default:
           printf("Enter Valid commands..\n");
          abort ();
        }
    }
  
 printf("argc is :%d\n", argc);
 printf("optind is :%d\n", optind);

  /* Print any remaining command line arguments (not options). */
  if (optind < argc)
    {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
        printf ("%s ", argv[optind++]);
      putchar ('\n');
    }

}

/** To send upstream msgs to server ***/

static void handleUpstreamMessage(void *msg, size_t len)
{
	int bytesWritten = 0;
	
	printf("handleUpstreamMessage length %zu\n", len);
	if(nopoll_conn_is_ok(conn) && nopoll_conn_is_ready(conn))
	{
		bytesWritten = nopoll_conn_send_binary(conn, msg, len);
		printf("Number of bytes written: %d\n", bytesWritten);
		if (bytesWritten != (int) len) 
		{
			printf("Failed to send bytes %zu, bytes written were=%d (errno=%d, %s)..\n", len, bytesWritten, errno, strerror(errno));
		}
	}
	else
	{
		printf("Failed to send msg upstream as connection is not OK\n");
	}
	
}

