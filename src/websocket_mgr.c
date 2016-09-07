/**
 * @file websocket_mgr.c
 *
 * @description This file is used to manage websocket connection, websocket messages and ping/pong (heartbeat) mechanism.
 *
 * Copyright (c) 2015  Comcast
 */
#include "stdlib.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "cJSON.h"
#include "nopoll.h"
#include <sys/time.h>

#include "pthread.h"
#include "msgpack.h"
#include <netdb.h>
#include <arpa/inet.h>
#include "signal.h"
#include <nanomsg/nn.h>
#include <nanomsg/bus.h>
#include <nanomsg/reqrep.h>
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define WEBPA_CLIENT					"webpa_client"

/* WebPA default Config */
#define WEBPA_SERVER_URL                                "talaria-beta.webpa.comcast.net"
#define WEBPA_SERVER_PORT                               8080
#define WEBPA_RETRY_INTERVAL_SEC                        10
#define WEBPA_MAX_PING_WAIT_TIME_SEC                    180

/* Notify Macros */
#define WEBPA_SET_INITIAL_NOTIFY_RETRY_COUNT            5 
#define WEBPA_SET_INITIAL_NOTIFY_RETRY_SEC              15
#define WEBPA_NOTIFY_EVENT_HANDLE_INTERVAL_MSEC         250

#define HTTP_CUSTOM_HEADER_COUNT                    	4
#define WEBPA_MESSAGE_HANDLE_INTERVAL_MSEC          	250
#define HEARTBEAT_RETRY_SEC                         	30      /* Heartbeat (ping/pong) timeout in seconds */
#define MAX_PARAMETERNAME_LEN				512
#define parodus_free(__x__) if(__x__ != NULL) { free((void*)(__x__)); __x__ = NULL;} else {printf("Trying to free null pointer\n");}

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct WebpaMsg__
{
	noPollCtx * ctx;
	noPollConn * conn;
	noPollMsg * msg;
	noPollPtr user_data;
	struct WebpaMsg__ *next;
} WebpaMsg;


typedef struct NanoMsg__
{
	char *nanoMsgData;
	struct NanoMsg__ *next;
} NanoMsg;



/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static WebPaCfg webPaCfg;
static noPollCtx *ctx = NULL;
static noPollConn *conn = NULL;
static pthread_t heartBeatThreadId;
static char deviceMAC[32]={'\0'}; 
static volatile int heartBeatTimer = 0;
static volatile bool terminated = false;
char *reconnect_reason = NULL;
static bool LastReasonStatus = false;


static NanoMsg *nanoMsgQ = NULL;
static bool close_retry = false;


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

static void listenerOnPingMessage (noPollCtx * ctx, noPollConn * conn, noPollMsg * msg, noPollPtr user_data);
static void listenerOnCloseMessage (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data);

void getCurrentTime(struct timespec *timer);
uint64_t getCurrentTimeInMicroSeconds(struct timespec *timer);

long timeValDiff(struct timespec *starttime, struct timespec *finishtime);
static void getDeviceMac();
static void initHeartBeatHandler();
static void *heartBeatHandlerTask();


static WAL_STATUS copyConfigFile();
static int checkHostIp(char * serverIP);
static WAL_STATUS __loadCfgFile(cJSON *config, WebPaCfg *cfg);
static void macToLower(char macValue[], char macConverted[]);


static void initNanoMsgTask();
static void *NanoMsgHandlerTask();
static void processNanomsgTask();
static void *processHandlerTask();
static void handleUpstreamMessage(UpstreamMsg *upstreamMsg);
static void handleNanoMsgEvents();
int server_recv ();

static void __report_log (noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data);


noPollPtr createMutex();
void lockMutex(noPollPtr _mutex);
void unlockMutex(noPollPtr _mutex);
void destroyMutex(noPollPtr _mutex);

WAL_STATUS __attribute__ ((weak)) checkDeviceInterface();

/**
 * @brief __report_log Nopoll log handler 
 * Nopoll log handler for integrating nopoll logs with webpa logger
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
    cJSON *config = (cJSON*) config_in;

	__loadCfgFile(config, &webPaCfg);
	printf("Port : %d and Address : %s and Outbound Interface: %s \n", webPaCfg.serverPort, webPaCfg.serverIP, webPaCfg.interfaceName);
	
	printf("Configure nopoll thread handlers in Webpa \n");
	nopoll_thread_handlers(&createMutex, &destroyMutex, &lockMutex, &unlockMutex);

	ctx = nopoll_ctx_new();
	if (!ctx) 
	{
		printf("\nError creating nopoll context\n");
	}

	#ifdef NOPOLL_LOGGER
  		nopoll_log_set_handler (ctx, __report_log, NULL);
	#endif


	getDeviceMac();
	createNopollConnection();
	initNanoMsgTask();
	processNanomsgTask();
	initMessageHandler();
	initHeartBeatHandler();
	
	if (NULL != initKeypress) 
	{
  		(* initKeypress) ();
	}

	do
	{
		nopoll_loop_wait(ctx, 5000000);
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
		sleep(2);
		if (NULL == notifyMsgQ) 
		{
			break;
		}
	}
	for (i=0; i<15; i++) 
	{
		pthread_mutex_lock (&mut);		
		if(webpaMsgQ == NULL)
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


/*
 * @brief __loadCfgFile To load the config file.
 */
static WAL_STATUS __loadCfgFile(cJSON *config, WebPaCfg *cfg)
{
	FILE *fp;
	cJSON *webpa_cfg = NULL;
	char *cfg_file_content = NULL, *temp_ptr = NULL;
	int ch_count = 0;
	cJSON *notifyArray = NULL;
	int i = 0;

  	if (NULL != config) 
	{
   	 	webpa_cfg = config;
	} 
	else // NULL == config
	{ 

		if(WAL_SUCCESS != copyConfigFile())
		{
			printf("Using the default configuration details.\n");
			walStrncpy(cfg->serverIP,WEBPA_SERVER_URL,sizeof(cfg->serverIP));
			cfg->serverPort = WEBPA_SERVER_PORT;
			cfg->secureFlag = true;
			walStrncpy(cfg->interfaceName, WEBPA_CFG_DEVICE_INTERFACE, sizeof(cfg->interfaceName));
			cfg->retryIntervalInSec = WEBPA_RETRY_INTERVAL_SEC;
			cfg->maxPingWaitTimeInSec = WEBPA_MAX_PING_WAIT_TIME_SEC;
			printf("cfg->serverIP : %s cfg->serverPort : %d cfg->interfaceName :%s cfg->retryIntervalInSec : %d cfg->maxPingWaitTimeInSec : %d\n", cfg->serverIP, cfg->serverPort, cfg->interfaceName, cfg->retryIntervalInSec, cfg->maxPingWaitTimeInSec);
			return WAL_SUCCESS;
		}
		else
		{
			fp = fopen(WEBPA_CFG_FILE, "r");
		}
	
	if (fp == NULL) 
	{
		printf("Failed to open cfg file %s\n", WEBPA_CFG_FILE);
		return WAL_FAILURE;
	}
	
	fseek(fp, 0, SEEK_END);
	ch_count = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	cfg_file_content = (char *) malloc(sizeof(char) * (ch_count + 1));
	fread(cfg_file_content, 1, ch_count,fp);
	cfg_file_content[ch_count] ='\0';
	fclose(fp);

	webpa_cfg = cJSON_Parse(cfg_file_content);
	} // end NULL == config

	if(webpa_cfg) 
	{
    		cJSON * json_obj;
		printf("**********Loading Webpa Config***********\n");
		temp_ptr = cJSON_GetObjectItem(webpa_cfg, WEBPA_CFG_SERVER_IP)->valuestring;
		strncpy(cfg->serverIP, temp_ptr,strlen(temp_ptr));
		cfg->serverPort = cJSON_GetObjectItem(webpa_cfg, WEBPA_CFG_SERVER_PORT)->valueint;
    		json_obj = cJSON_GetObjectItem(webpa_cfg, WEBPA_CFG_SERVER_SECURE);
		if (NULL == json_obj) 
		{
			cfg->secureFlag = true;
		} 
		else
 		{
			cfg->secureFlag = json_obj->valueint;
		}
		walStrncpy(cfg->interfaceName, WEBPA_CFG_DEVICE_INTERFACE, sizeof(cfg->interfaceName));
		cfg->retryIntervalInSec = cJSON_GetObjectItem(webpa_cfg, WEBPA_CFG_RETRY_INTERVAL)->valueint;
		cfg->maxPingWaitTimeInSec = cJSON_GetObjectItem(webpa_cfg, WEBPA_CFG_PING_WAIT_TIME)->valueint;
		printf("cfg->serverIP : %s cfg->serverPort : %d\n cfg->secureFlag %d\n cfg->interfaceName :%s\n cfg->retryIntervalInSec : %d cfg>maxPingWaitTimeInSec : %d\n", 
      cfg->serverIP, cfg->serverPort, cfg->secureFlag, cfg->interfaceName, cfg->retryIntervalInSec, cfg->maxPingWaitTimeInSec);

		if(cJSON_GetObjectItem(webpa_cfg, WEBPA_CFG_FIRMWARE_VER) != NULL)
		{
			temp_ptr = cJSON_GetObjectItem(webpa_cfg, WEBPA_CFG_FIRMWARE_VER)->valuestring;
			strncpy(cfg->oldFirmwareVersion, temp_ptr, strlen(temp_ptr)+1);
			printf("cfg->oldFirmwareVersion : %s\n", cfg->oldFirmwareVersion);
		}
		else
		{
			strcpy(cfg->oldFirmwareVersion,"");
		}

		if (NULL == config) 
		{		
			cJSON_Delete(webpa_cfg);
		}
	}
	else 
	{
		printf("Error parsing WebPA config file\n");
	}
	if (NULL == config) 
	{
		parodus_free(cfg_file_content);
	}
	return WAL_SUCCESS;
}

WAL_STATUS loadCfgFile(const char *cfgFileName, WebPaCfg *cfg)
{
	// first parameter is not used
	return __loadCfgFile (NULL, cfg);
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
	char *dbCID = NULL, *dbCMC = NULL, *buffer = NULL, *firmwareVersion = NULL, *modelName = NULL, *manufacturer = NULL;
	
	char *reboot_reason = NULL;
	char encodedData[1024];
	int  encodedDataSize = 1024;
	int i=0, j=0, connErr=0;

	time_t bootTime_sec;
	struct sysinfo s_info;
	unsigned int upTime=0;
	struct timespec currentTime;
	struct tm *info;
	char timeInUTC[256] = {'\0'};
	struct timespec start,end,connErr_start,connErr_end,*startPtr,*endPtr,*connErr_startPtr,*connErr_endPtr;
	startPtr = &start;
	endPtr = &end;
	connErr_startPtr = &connErr_start;
	connErr_endPtr = &connErr_end;
	
	snprintf(device_id, sizeof(device_id), "mac:%s", deviceMAC);
	printf("Device_id %s\n",device_id);

	headerValues[0] = device_id;
	headerValues[1] = "wrp-0.11,getset-0.1";
	
        
	if(sysinfo(&s_info))
	{   
	    	printf("Failed to get system uptime, sysinfo retunrs failure\n");   
		upTime = 0;
	}      
	else   
	{     
	    upTime = s_info.uptime;
	    printf("upTime is:%d\n",  upTime);
	
	}              
	
	printf("upTime %d\n",upTime);
	
	gettimeofday(&currentTime, NULL);
	bootTime_sec = currentTime.tv_sec - upTime;
	info = gmtime(&bootTime_sec);
	strftime(timeInUTC, sizeof(timeInUTC), "%a %b %d %H:%M:%S %Z %Y", info);
	printf("UTC Boot time: %s, BootTime In sec: %d\n",timeInUTC, bootTime_sec);


    	snprintf(user_agent, sizeof(user_agent),
             "WebPA-1.6 (%s; %s/%s;)",
             ((NULL != firmwareVersion) ? firmwareVersion : "unknown"),
             ((NULL != modelName) ? modelName : "unknown"),
             ((NULL != manufacturer) ? manufacturer : "unknown"));

	printf("User-Agent: %s\n",user_agent);
	headerValues[2] = user_agent;

	
	reconnect_reason = "webpa_process_starts";	
	printf("Received reconnect_reason as:%s\n", reconnect_reason);

	reboot_reason = "unknown";
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
		printf("X-WebPA-Convey Header: [%d]%s\n", strlen(buffer), buffer);

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
			printf("Encoded X-WebPA-Convey Header: [%d]%s\n", strlen(encodedData), encodedData);
		}
	}
	else
	{
                headerValues[3] = ""; 
                headerCount -= 1; 
                
	}
	
	parodus_free(reconnect_reason);
	parodus_free(reboot_reason);
	cJSON_Delete(response);
	parodus_free(buffer);
	
	snprintf(port,sizeof(port),"%d",webPaCfg.serverPort);
	walStrncpy(server_Address, webPaCfg.serverIP, sizeof(server_Address));
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
			while(checkDeviceInterface() != WAL_SUCCESS)
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
					printf("Interface %s is down, hence waiting\n",WEBPA_CFG_DEVICE_INTERFACE);
				}
				else if(count > 5)
					count = -1;

				sleep(webPaCfg.retryIntervalInSec);
				count++;
			}
		}	
				
		if(webPaCfg.secureFlag) 
		{
			/* disable verification */
			opts = nopoll_conn_opts_new ();
			nopoll_conn_opts_ssl_peer_verify (opts, nopoll_false);
			nopoll_conn_opts_set_ssl_protocol (opts, NOPOLL_METHOD_TLSV1_2); 
			conn = nopoll_conn_tls_new(ctx, opts, server_Address, port, NULL,
                               "/api/v2/device", NULL, NULL, webPaCfg.interfaceName,
                                headerNames, headerValues, headerCount);// WEBPA-787
		}
		else 
		{
			conn = nopoll_conn_new(ctx, server_Address, port, NULL,
                               "/api/v2/device", NULL, NULL, webPaCfg.interfaceName,
                                headerNames, headerValues, headerCount);// WEBPA-787
		}

		if(conn != NULL)
		{
			if(!nopoll_conn_is_ok(conn)) 
			{
				printf("Error connecting to server\n");
				printf("RDK-10037 - WebPA Connection Lost\n");
				// Copy the server address from config to avoid retrying to the same failing talaria redirected node
				walStrncpy(server_Address, webPaCfg.serverIP, sizeof(server_Address));
				__close_and_unref_connection__(conn);
				conn = NULL;
				initial_retry = true;
				// temp_retry flag if made false to force interface status check
				temp_retry = false;
				sleep(webPaCfg.retryIntervalInSec);
				continue;
			}
			else 
			{
				printf("Connected to Server but not yet ready\n");
				initial_retry = false;
				temp_retry = false;
			}

			if(!nopoll_conn_wait_until_connection_ready(conn, webPaCfg.retryIntervalInSec, redirectURL)) 
			{
				if (strncmp(redirectURL, "Redirect:", 9) == 0) // only when there is a http redirect
				{
					printf("Received temporary redirection response message %s\n", redirectURL);
					// Extract server Address and port from the redirectURL
					temp_ptr = strtok(redirectURL , ":"); //skip Redirect 
					temp_ptr = strtok(NULL , ":"); // skip https
					temp_ptr = strtok(NULL , ":");
					walStrncpy(server_Address, temp_ptr+2, sizeof(server_Address));
					walStrncpy(port, strtok(NULL , "/"), sizeof(port));
					printf("Trying to Connect to new Redirected server : %s with port : %s\n", server_Address, port);
				}
				else
				{
					printf("Client connection timeout\n");	
					printf("RDK-10037 - WebPA Connection Lost\n");
					// Copy the server address from config to avoid retrying to the same failing talaria redirected node
					walStrncpy(server_Address, webPaCfg.serverIP, sizeof(server_Address));
					sleep(webPaCfg.retryIntervalInSec);
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
			if((checkHostIp(server_Address) == -2) && (checkDeviceInterface() == WAL_SUCCESS)) 	
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
			sleep(webPaCfg.retryIntervalInSec);
			// Copy the server address from config to avoid retrying to the same failing talaria redirected node
			walStrncpy(server_Address, webPaCfg.serverIP, sizeof(server_Address));
		}
	}while(initial_retry);

	if(webPaCfg.secureFlag) 
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

	//nopoll_conn_set_on_msg(conn, (noPollOnMessageHandler) listenerOnMessage_queue, NULL);
	nopoll_conn_set_on_ping_msg(conn, (noPollOnMessageHandler)listenerOnPingMessage, NULL);
	nopoll_conn_set_on_close(conn, (noPollOnCloseHandler)listenerOnCloseMessage, NULL);
	return nopoll_true;
}

WAL_STATUS checkDeviceInterface()
{
	int link[2];
	pid_t pid;
	char statusValue[512] = {'\0'};
	char* data =NULL;
	int nbytes =0;
	WAL_STATUS status = WAL_FAILURE;
	
	if (pipe(link) == -1)
	{
		printf("Failed to create pipe for checking device interface (errno=%d, %s)\n",errno, strerror(errno));
		return status;
	}

	if ((pid = fork()) == -1)
	{
		printf("fork was unsuccessful while checking device interface (errno=%d, %s)\n",errno, strerror(errno));
		return status;
	}
	
	if(pid == 0) 
	{
		printf("child process created\n");
		pid = getpid();
		printf("child process execution with pid:%d\n", pid);
		dup2 (link[1], STDOUT_FILENO);
		close(link[0]);
		close(link[1]);	
		execl("/sbin/ifconfig", "ifconfig", WEBPA_CFG_DEVICE_INTERFACE, (char *)0);
	}	
	else 
	{
		close(link[1]);
		nbytes = read(link[0], statusValue, sizeof(statusValue));
		printf("statusValue is :%s\n", statusValue);
		
		if ((data = strstr(statusValue, "inet addr:")) !=NULL)
		{
			printf("Interface %s is up\n",WEBPA_CFG_DEVICE_INTERFACE);
			status = WAL_SUCCESS;
		}
		else
		{
			printf("Interface %s is down\n",WEBPA_CFG_DEVICE_INTERFACE);
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

	struct addrinfo hints = {};
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
 * @brief To initiate nanomessage handling
 */

static void initNanoMsgTask()
{
	int err = 0;
	pthread_t NanoMsgThreadId;
	nanoMsgQ = NULL;

	err = pthread_create(&NanoMsgThreadId, NULL, NanoMsgHandlerTask, NULL);
	if (err != 0) 
	{
		printf("Error creating messages thread :[%s]\n", strerror(err));
	}
	else
	{
		printf("NanoMsgHandlerTask thread created Successfully\n");
	}
}

/*
 * @brief To handle nano messages which will receive msg from server socket
 */
static void *NanoMsgHandlerTask()
{
	//while(1)
	//{	
	printf("server is starting..\n");	
	server_recv();
	printf("After server_recv..\n");
	
				
	//}
	printf ("Ended NanoMsgHandlerTask\n");
}


int server_recv ()
{
	//check here
	const char* url = NULL;
	const char* msg = NULL;
	
	
	printf("Inside Server function, URL = %s\n", url);
	int sock = nn_socket (AF_SP, NN_BUS);
	printf("Before Assert 1\n");
	assert (sock >= 0);
	printf("Before Assert 2\n");
	nn_bind (sock, url);
	//assert (nn_bind (sock, url) >= 0);
	printf("Before While \n");

	pthread_mutex_lock (&nano_prod_mut);
	while(1)
	{
	
		char *buf = NULL;
		printf("Gone into the listening mode...\n");
		int bytes = nn_recv (sock, &buf, NN_MSG, 0);
		//      assert (bytes >= 0);
		printf ("CLIENT: RECEIVED \"%s\"\n", buf);

		//Producer adds the nanoMsg into queue
		if(nanoMsgQ == NULL)
		{
			nanoMsgQ->nanoMsgData = buf;
			nanoMsgQ->next = NULL;
			printf("Producer added message\n");
		 	pthread_cond_signal(&nano_con);
			pthread_mutex_unlock (&nano_prod_mut);
			printf("mutex unlock in producer thread\n");
		}
		else
		{
			NanoMsg *temp = nanoMsgQ;
			while(temp->next)
			{
				temp = temp->next;
			}
			
			temp->nanoMsgData = buf;
			temp->next = NULL;
			
			pthread_mutex_unlock (&nano_prod_mut);
		}
		
	        nn_freemsg (buf);
	}

	return nn_shutdown (sock, 0);
}

/*
 * @brief To process the received msg and to send upstream
 */

static void processNanomsgTask()
{
	int err = 0;
	pthread_t processNanomsgThreadId;
	nanoMsgQ = NULL;

	err = pthread_create(&processNanomsgThreadId, NULL, processHandlerTask, NULL);
	if (err != 0) 
	{
		printf("Error creating messages thread :[%s]\n", strerror(err));
	}
	else
	{
		printf("processHandlerTask thread created Successfully\n");
	}
}



static void *processHandlerTask()
{
	printf("Inside processHandlerTask..\n");
	handleNanoMsgEvents();
	printf("After
}

static void handleNanoMsgEvents()
{
	printf("Inside handleNanoMsgEvents\n");	
	msgpack_zone mempool;
	msgpack_object deserialized;
	msgpack_unpack_return unpack_ret;
	char * decodeMsg =NULL;
	int decodeMsgSize =0;
	int size =0;
	UpstreamMsg *upstreamMsg;

	pthread_mutex_lock (&nano_cons_mut);
	printf("mutex lock in consumer thread\n");
	if(nanoMsgQ != NULL)
	{
		NanoMsg *message = nanoMsgQ;
		nanoMsgQ = nanoMsgQ->next;
		pthread_mutex_unlock (&nano_cons_mut);
		printf("mutex unlock in consumer thread\n");
		if (!terminated) 
		{
			
			decodeMsgSize = b64_get_decoded_buffer_size(strlen(message->nanoMsgData));
			decodeMsg = (char *) malloc(sizeof(char) * decodeMsgSize);
		
			size = b64_decode( message->nanoMsgData, strlen(message->nanoMsgData), decodeMsg );

			//Start of msgpack decoding just to verify -->need to replace this with wrp-c msgpack apis
			printf("----Start of msgpack decoding----\n");
			msgpack_zone_init(&mempool, 2048);
			unpack_ret = msgpack_unpack(decodeMsg, size, NULL, &mempool, &deserialized);
			printf("unpack_ret is %d\n",unpack_ret);
			switch(unpack_ret)
			{
				case MSGPACK_UNPACK_SUCCESS:
					printf("MSGPACK_UNPACK_SUCCESS :%d\n",unpack_ret);
					printf("\nmsgpack decoded data is:");
					msgpack_object_print(stdout, deserialized);
				break;
				case MSGPACK_UNPACK_EXTRA_BYTES:
					printf("MSGPACK_UNPACK_EXTRA_BYTES :%d\n",unpack_ret);
				break;
				case MSGPACK_UNPACK_CONTINUE:
					printf("MSGPACK_UNPACK_CONTINUE :%d\n",unpack_ret);
				break;
				case MSGPACK_UNPACK_PARSE_ERROR:
					printf("MSGPACK_UNPACK_PARSE_ERROR :%d\n",unpack_ret);
				break;
				case MSGPACK_UNPACK_NOMEM_ERROR:
					printf("MSGPACK_UNPACK_NOMEM_ERROR :%d\n",unpack_ret);
				break;
				default:
					printf("Message Pack decode failed with error: %d\n", unpack_ret);	
			}

			msgpack_zone_destroy(&mempool);
			printf("----End of msgpack decoding----\n");
			//End of msgpack decoding
			
			
			printf("copying to upstreamMsg->msg,msgLength \n");

			strcpy(	upstreamMsg->msg,  decodeMsg);
			strcpy( upstreamMsg->msgLength,  size);
			printf("calling handleUpstreamMessage\n");
			handleUpstreamMessage(upstreamMsg);
			
			printf("After handleUpstreamMessage\n");
			
			
		}
		
		//check for free here
		parodus_free(message);
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
		printf("Ping received with payload %s, opcode %d\n",payload, nopoll_msg_opcode(msg));
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
	WAL_STATUS setReconnectStatus = WAL_FAILURE;
	
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


/*
 * @brief To get the Device Mac Address.
 * 
 * Device MAC will be CM MAC for RDKB devices and STB MAC for RDKV devices
 */
static void getDeviceMac()
{

	char deviceMACValue[32]={'\0'};

	if(strlen(deviceMAC) == 0)
	{	
		
		printf("Inside getDeviceMac\n");
		FILE *fp;
		char path[1035];

		/* Open the command for reading. */
		fp = popen("ifconfig|grep eth0|tr -s ' ' |cut -d ' ' -f5", "r");

		if (fp == NULL) 
		{
			printf("Failed to run command\n" );
			exit(1);
		}


		/* Read the output a line at a time - output it. */
		if (fgets(path, sizeof(path)-1, fp) != NULL) 
		{
			printf("path:%scheck\n", path);
		}

		/* close */
		pclose(fp);
		path[strlen(path)-1] = '\0';
		strcpy(deviceMACValue, path);
		printf("getDeviceMacID: deviceMac:%scheck1\n",deviceMACValue);
	
		macToLower(deviceMACValue,deviceMAC);
		printf("deviceMAC after macToLower..%s\n", deviceMAC);
		
	}
}

/*
 * @brief To convert MAC to lower case without colon
 * assuming max MAC size as 32
 */
static void macToLower(char macValue[],char macConverted[])
{
	int i = 0;
	int j;
	char *token[32];
	char tmp[32];
	walStrncpy(tmp, macValue,sizeof(tmp));
	token[i] = strtok(tmp, ":");
	if(token[i]!=NULL)
	{
	    strncat(macConverted, token[i],31);
	    macConverted[31]='\0';
	    i++;
	}
	while ((token[i] = strtok(NULL, ":")) != NULL) 
	{
	    strncat(macConverted, token[i],31);
	    macConverted[31]='\0';
	    i++;
	}
	macConverted[31]='\0';
	for(j = 0; macConverted[j]; j++)
	{
	    macConverted[j] = tolower(macConverted[j]);
	}
}

/*
 * @brief To initiate heartbeat mechanism 
 */
static void initHeartBeatHandler()
{
	int err = 0;
	{
		err = pthread_create(&heartBeatThreadId, NULL, heartBeatHandlerTask, NULL);
		if (err != 0) 
		{
			printf("Error creating HeartBeat thread :[%s]\n", strerror(err));
		}
		else 
		{
			printf("heartBeatHandlerTask Thread created successfully\n");
		}
	}
}


/*
 * @brief To handle heartbeat mechanism
 */
static void *heartBeatHandlerTask()
{
	WAL_STATUS setReconnectStatus = WAL_FAILURE;
	while (!terminated)
	{
		sleep(HEARTBEAT_RETRY_SEC);
		if(heartBeatTimer >= webPaCfg.maxPingWaitTimeInSec) 
		{
			if(!close_retry) 
			{
				printf("ping wait time > %d. Terminating the connection with WebPA server and retrying\n", webPaCfg.maxPingWaitTimeInSec);
				
				
				reconnect_reason = "Ping_Miss";
				LastReasonStatus = true;
				
				pthread_mutex_lock (&close_mut);
				close_retry = true;
				pthread_mutex_unlock (&close_mut);
			}
			else
			{
				printf("heartBeatHandlerTask - close_retry set to %d, hence resetting the heartBeatTimer\n",close_retry);
			}
			heartBeatTimer = 0;
		}
		else 
		{
			printf("heartBeatTimer %d\n",heartBeatTimer);
			heartBeatTimer += HEARTBEAT_RETRY_SEC;			
		}
	}
	heartBeatTimer = 0;
	return NULL;
}


/*
 * @brief to copy config file
 */
static WAL_STATUS copyConfigFile()
{
	char ch;
	FILE *source, *target;
	if (access(WEBPA_CFG_FILE, F_OK) == -1) 
	{
		source = fopen(WEBPA_CFG_FILE_SRC, "r");
		if(source == NULL)
			return WAL_FAILURE;
		if(ferror(source))
		{
			printf("Error while reading webpa config file under %s\n", WEBPA_CFG_FILE_SRC);
			fclose(source);
			return WAL_FAILURE;
		}
		target = fopen(WEBPA_CFG_FILE, "w");

		if(target == NULL)
		{
			fclose(source);
			return WAL_FAILURE;
		}
		if(ferror(target))
		{
			printf("Error while writing webpa config file under /nvram.\n");
			fclose(source);
			fclose(target);
			return WAL_FAILURE;
		}	
		while ((ch = fgetc(source)) != EOF) 
		{
			fputc(ch, target);
			if (feof(source))
			{
				break;
			}
		}
		fclose(source);
		fclose(target);
		printf("Copied webpa_cfg.json to /nvram\n");
	}
	else 
	{
		printf("webpa_cfg.json already exist in /nvram\n");
	}
	return WAL_SUCCESS;
}



static void handleUpstreamMessage(UpstreamMsg *upstreamMsg)
{
	int bytesWritten = 0;
	printf("handleUpstreamMessage length %d\n", upstreamMsg->msgLength);
	if(nopoll_conn_is_ok(conn) && nopoll_conn_is_ready(conn))
	{
		bytesWritten = nopoll_conn_send_binary(conn, upstreamMsg->msg, upstreamMsg->msgLength);
		printf("Number of bytes written: %d\n", bytesWritten);
		if (bytesWritten != upstreamMsg->msgLength) 
		{
			printf("Failed to send bytes %d, bytes written were=%d (errno=%d, %s)..\n", upstreamMsg->msgLength, bytesWritten, errno, strerror(errno));
		}
	}
	else
	{
		printf("Failed to send notification as connection is not OK\n");
	}
	parodus_free(upstreamMsg->msg);
	printf("After upstreamMsg->msg free\n");
}

