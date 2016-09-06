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
#include "websocket_mgr.h"
#include "wal.h"
#include "wrp_mgr.h"
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
/* WRP Macros */
#define WRP_MSG_TYPE                                	"msg_type"
#define WRP_SOURCE                                  	"source"
#define WRP_STATUS                                  	"status"
#define WRP_DESTINATION                             	"dest"
#define WRP_TRANSACTION_ID                          	"transaction_uuid"
#define WRP_PAYLOAD                                 	"payload"
#define WRP_SPANS                          	        "spans"
#define WRP_INCLUDE_SPANS               	        "include_spans"
#define WRP_CONTENT_TYPE				"content_type"
#define WRP_MAP_SIZE					6
#define CONTENT_TYPE_JSON				"application/json"
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

typedef struct NotifyMsg__
{
	NotifyData *notifyData;
	struct NotifyMsg__ *next;
} NotifyMsg;

typedef struct NanoMsg__
{
	char *nanoMsgData;
	struct NanoMsg__ *next;
} NanoMsg;

typedef enum
{
	WRP_MSG_INVALID = -1,
	WRP_MSG_AUTH_STATUS = 2,
	WRP_MSG_REQUEST_RESPONSE = 3,
	WRP_MSG_NOTIFY = 4
} WrpMsgStatus;

typedef enum
{
        WEBPA_NOTIFY_XPC = 0,
        WEBPA_NOTIFY_IOT = 1,
        WEBPA_NOTIFY_DEVICE_STATUS = 2,
        WEBPA_NOTIFY_NODE_CHANGE = 3,
        WEBPA_NOTIFY_TRANS_STATUS = 4,                   
        WEBPA_INVALID_REQUEST = -1 
} WEBPA_NOTIFY_TYPE;

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
static WebpaMsg *webpaMsgQ = NULL;
static NotifyMsg *notifyMsgQ = NULL;
static NanoMsg *nanoMsgQ = NULL;
static bool close_retry = false;
static bool webpaReady = false;
static bool LastReasonStatus = false;

static void __close_and_unref_connection__(noPollConn *conn);

pthread_mutex_t mut=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t notif_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t close_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t nano_prod_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t nano_cons_mut=PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t con=PTHREAD_COND_INITIALIZER;
pthread_cond_t notif_con=PTHREAD_COND_INITIALIZER;
pthread_cond_t close_con=PTHREAD_COND_INITIALIZER;
pthread_cond_t nano_con=PTHREAD_COND_INITIALIZER;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static char createNopollConnection();
static void listenerOnMessage(noPollCtx * ctx, noPollConn * conn, noPollMsg * msg, noPollPtr user_data);
static void listenerOnPingMessage (noPollCtx * ctx, noPollConn * conn, noPollMsg * msg, noPollPtr user_data);
static void listenerOnCloseMessage (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data);
static char* getKey_MsgtypeStr(const msgpack_object key, const size_t keySize, char* keyString);
static char* getKey_MsgtypeBin(const msgpack_object key, const size_t binSize, char* keyBin);
void getCurrentTime(struct timespec *timer);
uint64_t getCurrentTimeInMicroSeconds(struct timespec *timer);
void add_total_webpa_client_time(uint64_t startTime,uint32_t duration,money_trace_spans *timeSpan);
long timeValDiff(struct timespec *starttime, struct timespec *finishtime);
static void getDeviceMac();
static void initHeartBeatHandler();
static void *heartBeatHandlerTask();
static void packString(msgpack_packer *notifEncoder, char * str);
static void packInitialComponentsToResponse(msgpack_packer encodeResponseVal,int msg_type,char *source,char *dest,char *transaction_id, money_trace_spans *timeSpan); 
static void packAndSendResponse(cJSON *response, int msgType, char *source, char *dest, char *transaction_id, money_trace_spans *timeSpan,  noPollConn * conn);
static void decodeRequest(msgpack_object deserialized,int *msgType, char** source_ptr,char** dest_ptr,char** transaction_id_ptr,int *statusValue,char** payload_ptr, bool *includeTimingValues);
static WAL_STATUS copyConfigFile();
static int checkHostIp(char * serverIP);
static WAL_STATUS __loadCfgFile(cJSON *config, WebPaCfg *cfg);
static void macToLower(char macValue[], char macConverted[]);
static void listenerOnMessage_queue(noPollCtx * ctx, noPollConn * conn, noPollMsg * msg,noPollPtr user_data);
static void initMessageHandler();
static void *messageHandlerTask();
static void initWebpaTask();
static void *webpaTask();
static void initNanoMsgTask();
static void *NanoMsgHandlerTask();
static void processNanomsgTask();
static void *processHandlerTask();
static void notifyCallback(NotifyData *notifyData);
static void addNotifyMsgToQueue(NotifyData *notifyData);
static void setInitialNotify();
static void handleNotificationEvents();
static void sendParamNotifyToServer(WEBPA_NOTIFY_TYPE notifyType, unsigned int cmc, const char *str);
static void freeNotifyMessage(NotifyMsg *message);
static void sendNotificationForFactoryReset();
static void sendNotificationForFirmwareUpgrade();
static void handleUpstreamMessage(UpstreamMsg *upstreamMsg);
static void handleParamNotifyMessage(ParamNotify *message);
static void __report_log (noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data);
static void handleNanoMsgEvents();
int server_recv ();

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
  	        WalPrint("Debug: %s\n", log_msg);
	}
	if (level == NOPOLL_LEVEL_INFO) 
	{
		WalInfo("Info: %s\n", log_msg);
	}
	if (level == NOPOLL_LEVEL_WARNING) 
	{
  	        WalInfo("Warning: %s\n", log_msg);
	}
	if (level == NOPOLL_LEVEL_CRITICAL) 
	{
  	        WalError("Error: %s\n", log_msg);
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
	WalPrint("Port : %d and Address : %s and Outbound Interface: %s \n", webPaCfg.serverPort, webPaCfg.serverIP, webPaCfg.interfaceName);
	
	WalPrint("Configure nopoll thread handlers in Webpa \n");
	nopoll_thread_handlers(&createMutex, &destroyMutex, &lockMutex, &unlockMutex);

	ctx = nopoll_ctx_new();
	if (!ctx) 
	{
		WalError("\nError creating nopoll context\n");
	}

	#ifdef NOPOLL_LOGGER
  		nopoll_log_set_handler (ctx, __report_log, NULL);
	#endif

	waitForConnectReadyCondition();

	getDeviceMac();
	createNopollConnection();
	initNanoMsgTask();
	processNanomsgTask();
	initMessageHandler();
	initHeartBeatHandler();
	initWebpaTask();
	if (NULL != initKeypress) 
	{
  		(* initKeypress) ();
	}

	do
	{
		nopoll_loop_wait(ctx, 5000000);
		if(close_retry)
		{
			WalInfo("close_retry is %d, hence closing the connection and retrying\n", close_retry);
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
			WalInfo("Using the default configuration details.\n");
			walStrncpy(cfg->serverIP,WEBPA_SERVER_URL,sizeof(cfg->serverIP));
			cfg->serverPort = WEBPA_SERVER_PORT;
			cfg->secureFlag = true;
			walStrncpy(cfg->interfaceName, WEBPA_CFG_DEVICE_INTERFACE, sizeof(cfg->interfaceName));
			cfg->retryIntervalInSec = WEBPA_RETRY_INTERVAL_SEC;
			cfg->maxPingWaitTimeInSec = WEBPA_MAX_PING_WAIT_TIME_SEC;
			WalPrint("cfg->serverIP : %s cfg->serverPort : %d cfg->interfaceName :%s cfg->retryIntervalInSec : %d cfg->maxPingWaitTimeInSec : %d\n", cfg->serverIP, cfg->serverPort, cfg->interfaceName, cfg->retryIntervalInSec, cfg->maxPingWaitTimeInSec);
			return WAL_SUCCESS;
		}
		else
		{
			fp = fopen(WEBPA_CFG_FILE, "r");
		}
	
	if (fp == NULL) 
	{
		WalError("Failed to open cfg file %s\n", WEBPA_CFG_FILE);
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
		WalPrint("**********Loading Webpa Config***********\n");
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
		WalPrint("cfg->serverIP : %s cfg->serverPort : %d\n cfg->secureFlag %d\n cfg->interfaceName :%s\n cfg->retryIntervalInSec : %d cfg>maxPingWaitTimeInSec : %d\n", 
      cfg->serverIP, cfg->serverPort, cfg->secureFlag, cfg->interfaceName, cfg->retryIntervalInSec, cfg->maxPingWaitTimeInSec);

		if(cJSON_GetObjectItem(webpa_cfg, WEBPA_CFG_FIRMWARE_VER) != NULL)
		{
			temp_ptr = cJSON_GetObjectItem(webpa_cfg, WEBPA_CFG_FIRMWARE_VER)->valuestring;
			strncpy(cfg->oldFirmwareVersion, temp_ptr, strlen(temp_ptr)+1);
			WalPrint("cfg->oldFirmwareVersion : %s\n", cfg->oldFirmwareVersion);
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
		WalError("Error parsing WebPA config file\n");
	}
	if (NULL == config) 
	{
		WAL_FREE(cfg_file_content);
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
		WalError("Failed to create mutex\n");
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
		WalError("Error in init Mutex\n");
		nopoll_free(mutex);
		return NULL;
	}
	else 
	{
		WalPrint("mutex init successfully\n");
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
	/* WalPrint("Inside Lock mutex\n"); */
	if (_mutex == NULL)
	{
		WalError("Received null mutex\n");
		return;
	}
	pthread_mutex_t * mutex = _mutex;

	/* lock the mutex */
	rtn = pthread_mutex_lock (mutex);
	if (rtn != 0) 
	{
		strerror_r (rtn, errbuf, 100);
		WalError("Error in Lock mutex: %s\n", errbuf);
		/* do some reporting */
		return;
	}
	else
	{
		/* WalPrint("Mutex locked \n"); */
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
	/* WalPrint("Inside unlock mutex\n"); */
	if (_mutex == NULL)
	{
		WalError("Received null mutex\n");
		return;
	}
	pthread_mutex_t * mutex = _mutex;

	/* unlock mutex */
	rtn = pthread_mutex_unlock (mutex);
	if (rtn != 0) 
	{
		/* do some reporting */
		strerror_r (rtn, errbuf, 100);
		WalError("Error in unlock mutex: %s\n", errbuf);
		return;
	}
	else
	{
		/* WalPrint("Mutex unlocked \n"); */
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
		WalError("Received null mutex\n");
		return;
	}
	pthread_mutex_t * mutex = _mutex;
	
	if (pthread_mutex_destroy (mutex) != 0) 
	{
		/* do some reporting */
		WalError("problem in destroy\n");
		return;
	}
	else
	{
		WalPrint("Mutex destroyed \n");
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
	cJSON *resParamObj1 = NULL ,*resParamObj2 = NULL,*resParamObj3 = NULL ,*resParamObj4 = NULL ,*resParamObj5 = NULL ,*resParamObj6 = NULL, *parameters = NULL;
	cJSON *response = cJSON_CreateObject();
	char *dbCID = NULL, *dbCMC = NULL, *buffer = NULL, *firmwareVersion = NULL, *modelName = NULL, *manufacturer = NULL;
	char *reconnect_reason = NULL;
	char *reboot_reason = NULL;
	char encodedData[1024];
	int  encodedDataSize = 1024;
	int i=0, j=0, connErr=0;
	WAL_STATUS setReconnectStatus = WAL_FAILURE;
	time_t bootTime_sec;
	unsigned int upTime=0;
	char *upTimeStr = NULL;
	struct timespec currentTime;
	struct tm *info;
	char timeInUTC[256] = {'\0'};
	struct timespec start,end,connErr_start,connErr_end,*startPtr,*endPtr,*connErr_startPtr,*connErr_endPtr;
	startPtr = &start;
	endPtr = &end;
	connErr_startPtr = &connErr_start;
	connErr_endPtr = &connErr_end;
	
	snprintf(device_id, sizeof(device_id), "mac:%s", deviceMAC);
	WalInfo("Device_id %s\n",device_id);

	headerValues[0] = device_id;
	headerValues[1] = "wrp-0.11,getset-0.1";

	/* Create and encode X-WebPA-Convey header value */
	dbCID = getSyncParam(SYNC_PARAM_CID);
	dbCMC = getSyncParam(SYNC_PARAM_CMC);
	if ((NULL != dbCID) && (NULL != dbCMC)) 
	{
		if ((strncmp(dbCID, "0", 1) == 0) && (strncmp(dbCMC, "0", 1) == 0))
	    	{
			WalPrint("Set reboot as factory-reset\n");
	    		setReconnectStatus = setSyncParam(SYNC_PARAM_REBOOT_REASON, "factory-reset");
	    		
	    		if(setReconnectStatus != WAL_SUCCESS)
	    		{
	    		    WalError("Error in setting reboot reason for factory-reset\n");
	    		}	
	    	}
	}

	firmwareVersion = getSyncParam(SYNC_PARAM_FIRMWARE_VERSION);
	if (NULL == firmwareVersion) 
	{
		WalError("Error:createNopollConnection()->SYNC_PARAM_FIRMWARE_VERSION not available!\n");
	}

	//Adding uptime to convey header
	upTimeStr = getSyncParam(SYNC_PARAM_DEVICE_UP_TIME);
	if (NULL == upTimeStr) 
	{
		WalError("Error:createNopollConnection()->SYNC_PARAM_DEVICE_UP_TIME not available!\n");
		upTimeStr = malloc(sizeof(char) * 2);
		strcpy(upTimeStr,"0");
	}
	upTime = strtoul(upTimeStr,NULL,10);
	WalPrint("upTime %d\n",upTime);
	
	gettimeofday(&currentTime, NULL);
	bootTime_sec = currentTime.tv_sec - upTime;
	info = gmtime(&bootTime_sec);
	strftime(timeInUTC, sizeof(timeInUTC), "%a %b %d %H:%M:%S %Z %Y", info);
	WalPrint("UTC Boot time: %s, BootTime In sec: %d\n",timeInUTC, bootTime_sec);
	
	WalPrint("Firmware Version: %s\n", firmwareVersion);

	// Initalize User-Agent header as "WebPA-1.6 (%s; %s/%s;)",Device.DeviceInfo.X_CISCO_COM_FirmwareName,Device.DeviceInfo.ModelName,Device.DeviceInfo.Manufacturer
	modelName = getSyncParam(SYNC_PARAM_MODEL_NAME);
	if (NULL == modelName) 
	{
		WalError("Error:createNopollConnection()->SYNC_PARAM_MODEL_NAME not available!\n");
	}
	manufacturer = getSyncParam(SYNC_PARAM_MANUFACTURER);
	if (NULL == manufacturer) 
	{
		WalError("Error:createNopollConnection()->SYNC_PARAM_MANUFACTUER not available!\n");
	}


    	snprintf(user_agent, sizeof(user_agent),
             "WebPA-1.6 (%s; %s/%s;)",
             ((NULL != firmwareVersion) ? firmwareVersion : "unknown"),
             ((NULL != modelName) ? modelName : "unknown"),
             ((NULL != manufacturer) ? manufacturer : "unknown"));

	WalInfo("User-Agent: %s\n",user_agent);
	headerValues[2] = user_agent;

	reconnect_reason = getSyncParam(SYNC_PARAM_RECONNECT_REASON);
	WalInfo("Received reconnect_reason as:%s\n", reconnect_reason);

	reboot_reason = getSyncParam(SYNC_PARAM_REBOOT_REASON);
	WalInfo("Received reboot_reason as:%s\n", reboot_reason);
		
	if(dbCMC != NULL && dbCID != NULL && firmwareVersion != NULL )
	{
		cJSON_AddItemToObject(response, "parameters", parameters = cJSON_CreateArray());
		cJSON_AddItemToArray(parameters, resParamObj1 = cJSON_CreateObject());
		cJSON_AddStringToObject(resParamObj1, "name", XPC_SYNC_PARAM_CID);
		cJSON_AddStringToObject(resParamObj1, "value", dbCID);
		cJSON_AddNumberToObject(resParamObj1, "dataType", WAL_STRING);

		cJSON_AddItemToArray(parameters, resParamObj2 = cJSON_CreateObject());
		cJSON_AddStringToObject(resParamObj2, "name", XPC_SYNC_PARAM_CMC);
		cJSON_AddNumberToObject(resParamObj2, "value", atoi(dbCMC));
		cJSON_AddNumberToObject(resParamObj2, "dataType", WAL_UINT);
		
		cJSON_AddItemToArray(parameters, resParamObj3 = cJSON_CreateObject());
		cJSON_AddStringToObject(resParamObj3, "name", WEBPA_FIRMWARE_VERSION);
		cJSON_AddStringToObject(resParamObj3, "value", firmwareVersion);
		cJSON_AddNumberToObject(resParamObj3, "dataType", WAL_STRING);
		
		cJSON_AddItemToArray(parameters, resParamObj4 = cJSON_CreateObject());
		cJSON_AddStringToObject(resParamObj4, "name", "Device.DeviceInfo.BootTime");
		cJSON_AddNumberToObject(resParamObj4, "value", bootTime_sec);
		cJSON_AddNumberToObject(resParamObj4, "dataType", WAL_UINT);
		
		if(reconnect_reason !=NULL)
		{
			cJSON_AddItemToArray(parameters, resParamObj5 = cJSON_CreateObject());
			cJSON_AddStringToObject(resParamObj5, "name", WEBPA_RECONNECT_REASON);
			cJSON_AddStringToObject(resParamObj5, "value", reconnect_reason);
			cJSON_AddNumberToObject(resParamObj5, "dataType", WAL_STRING);
		}
		else
		{
		     	WalError("Failed to GET Reconnect reason value\n");
		}
		if(reboot_reason !=NULL)
		{
			
			cJSON_AddItemToArray(parameters, resParamObj6 = cJSON_CreateObject());
			cJSON_AddStringToObject(resParamObj6, "name", WEBPA_REBOOT_REASON);
			cJSON_AddStringToObject(resParamObj6, "value", reboot_reason);
			cJSON_AddNumberToObject(resParamObj6, "dataType", WAL_STRING);
		}
		else
		{
			WalError("Failed to GET Reboot reason value\n");
		}
		

		buffer = cJSON_PrintUnformatted(response);
		WalInfo("X-WebPA-Convey Header: [%d]%s\n", strlen(buffer), buffer);

		if(nopoll_base64_encode (buffer, strlen(buffer), encodedData, &encodedDataSize) != nopoll_true)
		{
			WalError("Base64 Encoding failed for Connection Header\n");
			headerValues[3] = ""; // "not-available";
                        headerCount -= 1; // WEBPA-787 
		}
		else
		{
			/* Remove \n characters from the base64 encoded data */
			for(i=0;encodedData[i] != '\0';i++)
			{
				if(encodedData[i] == '\n')
				{
					WalPrint("New line is present in encoded data at position %d\n",i);
				}
				else
				{
					encodedData[j] = encodedData[i];
					j++;
				}
			}
			encodedData[j]='\0';
			headerValues[3] = encodedData;
			WalPrint("Encoded X-WebPA-Convey Header: [%d]%s\n", strlen(encodedData), encodedData);
		}
	}
	else
	{
                headerValues[3] = ""; // "not-available";
                headerCount -= 1; // WEBPA-787 
                WalError("Failed to Get CMC, CID value\n");
	}
	WAL_FREE(dbCID);
	WAL_FREE(dbCMC);
	WAL_FREE(firmwareVersion);
	WAL_FREE(modelName);
	WAL_FREE(manufacturer);
	WAL_FREE(reconnect_reason);
	WAL_FREE(reboot_reason);
	WAL_FREE(upTimeStr);
	cJSON_Delete(response);
	WAL_FREE(buffer);
	
	snprintf(port,sizeof(port),"%d",webPaCfg.serverPort);
	walStrncpy(server_Address, webPaCfg.serverIP, sizeof(server_Address));
	WalInfo("server_Address %s\n",server_Address);
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
				WalPrint("Elapsed time for checking interface status is: %ld ms\n", timeValDiff(startPtr, endPtr));
				if(timeValDiff(startPtr, endPtr) >= (15*60*1000))
				{
					WalInfo("Proceeding to webpa connection retry irrespective of interface status as interface is down for more than 15 minutes\n");
					break;
				}
				if(count == 0)
				{
					WalError("Interface %s is down, hence waiting\n",WEBPA_CFG_DEVICE_INTERFACE);
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
				WalError("Error connecting to server\n");
				WalError("RDK-10037 - WebPA Connection Lost\n");
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
				WalPrint("Connected to Server but not yet ready\n");
				initial_retry = false;
				temp_retry = false;
			}

			if(!nopoll_conn_wait_until_connection_ready(conn, webPaCfg.retryIntervalInSec, redirectURL)) 
			{
				if (strncmp(redirectURL, "Redirect:", 9) == 0) // only when there is a http redirect
				{
					WalError("Received temporary redirection response message %s\n", redirectURL);
					// Extract server Address and port from the redirectURL
					temp_ptr = strtok(redirectURL , ":"); //skip Redirect 
					temp_ptr = strtok(NULL , ":"); // skip https
					temp_ptr = strtok(NULL , ":");
					walStrncpy(server_Address, temp_ptr+2, sizeof(server_Address));
					walStrncpy(port, strtok(NULL , "/"), sizeof(port));
					WalInfo("Trying to Connect to new Redirected server : %s with port : %s\n", server_Address, port);
				}
				else
				{
					WalError("Client connection timeout\n");	
					WalError("RDK-10037 - WebPA Connection Lost\n");
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
				WalPrint("Connection is ready\n");
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
					WalInfo("First connect error occurred, initialized the connect error timer\n");
				}
				else
				{
					getCurrentTime(connErr_endPtr);
					WalPrint("checking timeout difference:%ld\n", timeValDiff(connErr_startPtr, connErr_endPtr));
					if(timeValDiff(connErr_startPtr, connErr_endPtr) >= (15*60*1000))
					{
						WalError("WebPA unable to connect due to DNS resolving to 10.0.0.1 for over 15 minutes; crashing service.\n");
						
						WalInfo("Setting Reconnect reason as Dns_Res_webpa_reconnect\n");
						setReconnectStatus = setSyncParam(SYNC_PARAM_RECONNECT_REASON, "Dns_Res_webpa_reconnect");
				
						if(setReconnectStatus != WAL_SUCCESS)
						{
							WalError("Error setting LastReconnectReason for Dns Failure\n");
						}
	
						LastReasonStatus = true;
						
						kill(getpid(), SIGTERM);						
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
		WalInfo("Connected to server over SSL\n");
	}
	else 
	{
		WalInfo("Connected to server\n");
	}
	
	// Reset close_retry flag and heartbeatTimer once the connection retry is successful
	WalPrint("createNopollConnection(): close_mutex lock\n");
	pthread_mutex_lock (&close_mut);
	close_retry = false;
	pthread_mutex_unlock (&close_mut);
	WalPrint("createNopollConnection(): close_mutex unlock\n");
	heartBeatTimer = 0;

	// Reset connErr flag on successful connection
	connErr = 0;
	WalPrint("Reset Reconnect reason as webpa_process_starts\n");
	setReconnectStatus = setSyncParam(SYNC_PARAM_RECONNECT_REASON, "webpa_process_starts");
				
	if(setReconnectStatus != WAL_SUCCESS)
	{
		WalError("Error in resetting ReconnectReason after successful connection\n");
	}

	LastReasonStatus =false;
	WalPrint("LastReasonStatus reset after successful connection\n");

	nopoll_conn_set_on_msg(conn, (noPollOnMessageHandler) listenerOnMessage_queue, NULL);
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
		WalError("Failed to create pipe for checking device interface (errno=%d, %s)\n",errno, strerror(errno));
		return status;
	}

	if ((pid = fork()) == -1)
	{
		WalError("fork was unsuccessful while checking device interface (errno=%d, %s)\n",errno, strerror(errno));
		return status;
	}
	
	if(pid == 0) 
	{
		WalPrint("child process created\n");
		pid = getpid();
		WalPrint("child process execution with pid:%d\n", pid);
		dup2 (link[1], STDOUT_FILENO);
		close(link[0]);
		close(link[1]);	
		execl("/sbin/ifconfig", "ifconfig", WEBPA_CFG_DEVICE_INTERFACE, (char *)0);
	}	
	else 
	{
		close(link[1]);
		nbytes = read(link[0], statusValue, sizeof(statusValue));
		WalPrint("statusValue is :%s\n", statusValue);
		
		if ((data = strstr(statusValue, "inet addr:")) !=NULL)
		{
			WalPrint("Interface %s is up\n",WEBPA_CFG_DEVICE_INTERFACE);
			status = WAL_SUCCESS;
		}
		else
		{
			WalPrint("Interface %s is down\n",WEBPA_CFG_DEVICE_INTERFACE);
		}
		
		if(pid == wait(NULL))
		{
			WalPrint("child process pid %d terminated successfully\n", pid);
		}
		else
		{
			WalError("Error reading wait status of child process pid %d, killing it\n", pid);
			kill(pid, SIGKILL);
		}
		close(link[0]);

	}
	WalPrint("checkDeviceInterface:%d\n", status);
	return status;
}

/**
 * @brief checkHostIp interface to check if the Host server DNS is resolved to correct IP Address.
 * @param[in] serverIP server address DNS
 * Converts HostName to Host IP Address and checks if it is not same as 10.0.0.1 so that we can proceed with connection
 */
static int checkHostIp(char * serverIP)
{
	WalPrint("...............Inside checkHostIp..............%s \n", serverIP);
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
		WalError("getaddrinfo: %s\n", gai_strerror(retVal));
	}
	else
	{
		res = result;
		while(res)
		{  
			ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
			inet_ntop (res->ai_family, ptr, addrstr, 100);
			WalPrint ("IPv4 address of %s is %s \n", serverIP, addrstr);
			if (strcmp(localIp,addrstr) == 0)
			{
				WalPrint("Host Ip resolved to 10.0.0.1\n");
				status = -2;
			}
			else
			{
				WalPrint("Host Ip resolved correctly, proceeding with the connection\n");
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
 * @brief To initiate message handling
 */

static void initMessageHandler()
{
	int err = 0;
	pthread_t messageThreadId;
	webpaMsgQ = NULL;

	err = pthread_create(&messageThreadId, NULL, messageHandlerTask, NULL);
	if (err != 0) 
	{
		WalError("Error creating messages thread :[%s]\n", strerror(err));
	}
	else
	{
		WalPrint("messageHandlerTask thread created Successfully\n");
	}
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
		WalError("Error creating messages thread :[%s]\n", strerror(err));
	}
	else
	{
		WalPrint("NanoMsgHandlerTask thread created Successfully\n");
	}
}

/*
 * @brief To handle nano messages which will receive msg from server socket
 */
static void *NanoMsgHandlerTask()
{
	//while(1)
	//{	
	WalInfo("server is starting..\n");	
	server_recv();
	WalInfo("After server_recv..\n");
	
				
	//}
	WalInfo ("Ended NanoMsgHandlerTask\n");
}


int server_recv ()
{
	//check here
	const char* url = NULL;
	const char* msg = NULL;
	
	
	WalInfo("Inside Server function, URL = %s\n", url);
	int sock = nn_socket (AF_SP, NN_BUS);
	WalInfo("Before Assert 1\n");
	assert (sock >= 0);
	WalInfo("Before Assert 2\n");
	nn_bind (sock, url);
	//assert (nn_bind (sock, url) >= 0);
	WalInfo("Before While \n");

	pthread_mutex_lock (&nano_prod_mut);
	while(1)
	{
	
		char *buf = NULL;
		WalInfo("Gone into the listening mode...\n");
		int bytes = nn_recv (sock, &buf, NN_MSG, 0);
		//      assert (bytes >= 0);
		WalInfo ("CLIENT: RECEIVED \"%s\"\n", buf);

		//Producer adds the nanoMsg into queue
		if(nanoMsgQ == NULL)
		{
			nanoMsgQ->nanoMsgData = buf;
			nanoMsgQ->next = NULL;
			WalInfo("Producer added message\n");
		 	pthread_cond_signal(&nano_con);
			pthread_mutex_unlock (&nano_prod_mut);
			WalInfo("mutex unlock in producer thread\n");
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
		WalError("Error creating messages thread :[%s]\n", strerror(err));
	}
	else
	{
		WalPrint("processHandlerTask thread created Successfully\n");
	}
}



static void *processHandlerTask()
{
	WalInfo("Inside processHandlerTask..\n");
	handleNanoMsgEvents();
	WalInfo("After
}

static void handleNanoMsgEvents()
{
	WalInfo("Inside handleNanoMsgEvents\n");	
	msgpack_zone mempool;
	msgpack_object deserialized;
	msgpack_unpack_return unpack_ret;
	char * decodeMsg =NULL;
	int decodeMsgSize =0;
	int size =0;
	UpstreamMsg *upstreamMsg;

	pthread_mutex_lock (&nano_cons_mut);
	WalPrint("mutex lock in consumer thread\n");
	if(nanoMsgQ != NULL)
	{
		NanoMsg *message = nanoMsgQ;
		nanoMsgQ = nanoMsgQ->next;
		pthread_mutex_unlock (&nano_cons_mut);
		WalPrint("mutex unlock in consumer thread\n");
		if (!terminated) 
		{
			
			decodeMsgSize = b64_get_decoded_buffer_size(strlen(message->nanoMsgData));
			decodeMsg = (char *) malloc(sizeof(char) * decodeMsgSize);
		
			size = b64_decode( message->nanoMsgData, strlen(message->nanoMsgData), decodeMsg );

			//Start of msgpack decoding just to verify
			WalPrint("----Start of msgpack decoding----\n");
			msgpack_zone_init(&mempool, 2048);
			unpack_ret = msgpack_unpack(decodeMsg, size, NULL, &mempool, &deserialized);
			WalPrint("unpack_ret is %d\n",unpack_ret);
			switch(unpack_ret)
			{
				case MSGPACK_UNPACK_SUCCESS:
					WalInfo("MSGPACK_UNPACK_SUCCESS :%d\n",unpack_ret);
					WalPrint("\nmsgpack decoded data is:");
					msgpack_object_print(stdout, deserialized);
				break;
				case MSGPACK_UNPACK_EXTRA_BYTES:
					WalError("MSGPACK_UNPACK_EXTRA_BYTES :%d\n",unpack_ret);
				break;
				case MSGPACK_UNPACK_CONTINUE:
					WalError("MSGPACK_UNPACK_CONTINUE :%d\n",unpack_ret);
				break;
				case MSGPACK_UNPACK_PARSE_ERROR:
					WalError("MSGPACK_UNPACK_PARSE_ERROR :%d\n",unpack_ret);
				break;
				case MSGPACK_UNPACK_NOMEM_ERROR:
					WalError("MSGPACK_UNPACK_NOMEM_ERROR :%d\n",unpack_ret);
				break;
				default:
					WalError("Message Pack decode failed with error: %d\n", unpack_ret);	
			}

			msgpack_zone_destroy(&mempool);
			WalPrint("----End of msgpack decoding----\n");
			//End of msgpack decoding
			
			
			WalPrint("copying to upstreamMsg->msg,msgLength \n");

			strcpy(	upstreamMsg->msg,  decodeMsg);
			strcpy( upstreamMsg->msgLength,  size);
			WalInfo("calling handleUpstreamMessage\n");
			handleUpstreamMessage(upstreamMsg);
			
			WalPrint("After handleUpstreamMessage\n");
			
			
		}
		
		//check for free here
		WAL_FREE(message);
	}
	else
	{
		WalPrint("Before pthread cond wait in consumer thread\n");   
		pthread_cond_wait(&nano_con, &nano_prod_mut);
		pthread_mutex_unlock (&nano_cons_mut);
		WalPrint("mutex unlock in consumer thread after cond wait\n");
		if (terminated) {
			break;
		}
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
		WalPrint("mutex lock in consumer thread\n");
		if(webpaMsgQ != NULL)
		{
			WebpaMsg *message = webpaMsgQ;
			webpaMsgQ = webpaMsgQ->next;
			pthread_mutex_unlock (&mut);
			WalPrint("mutex unlock in consumer thread\n");
			if (!terminated) 
			{
				listenerOnMessage(message->ctx,message->conn,message->msg,message->user_data);
			}
			nopoll_ctx_unref(message->ctx);
			nopoll_conn_unref(message->conn);
			nopoll_msg_unref(message->msg);
			WAL_FREE(message);
		}
		else
		{
			WalPrint("Before pthread cond wait in consumer thread\n");   
			pthread_cond_wait(&con, &mut);
			pthread_mutex_unlock (&mut);
			WalPrint("mutex unlock in consumer thread after cond wait\n");
			if (terminated) 
			{
				break;
			}
		}
	}
	WalPrint ("Ended messageHandlerTask\n");
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
	WebpaMsg *message;

	if (terminated) 
	{
		return;
	}

	message = (WebpaMsg *)malloc(sizeof(WebpaMsg));

	if(message)
	{
		message->ctx = ctx;
		message->conn = conn;
		message->msg = msg;
		message->user_data = user_data;
		message->next = NULL;

		nopoll_ctx_ref(ctx);
		nopoll_conn_ref(conn);
		nopoll_msg_ref(msg);
		
		pthread_mutex_lock (&mut);		
		WalPrint("mutex lock in producer thread\n");
		
		if(webpaMsgQ == NULL)
		{
			webpaMsgQ = message;
			WalPrint("Producer added message\n");
		 	pthread_cond_signal(&con);
			pthread_mutex_unlock (&mut);
			WalPrint("mutex unlock in producer thread\n");
		}
		else
		{
			WebpaMsg *temp = webpaMsgQ;
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
		WalError("Memory allocation is failed\n");
	}
	WalPrint("*****Returned from listenerOnMessage_queue*****\n");
}

/**
 * @brief listenerOnMessage function to create WebSocket listener to receive connections
 *
 * @param[in] ctx The context where the connection happens.
 * @param[in] conn The Websocket connection object
 * @param[in] msg The message received from server for various process requests
 * @param[out] user_data data which is to be sent
 */
static void listenerOnMessage(noPollCtx * ctx, noPollConn * conn, noPollMsg * msg,noPollPtr user_data)
{
	cJSON *request = NULL;
	int msgType = WRP_MSG_INVALID;
	int msgSize = 0;
	struct timespec start,end,*startPtr,*endPtr,start1, end1;
	startPtr = &start;
	endPtr = &end;
	char* source = NULL;
	char* dest = NULL;
	char* transaction_id = NULL;
	char *payload = NULL;
	char * command = NULL;
	char *out = NULL;
	uint32_t duration = 0;
	uint64_t startTime = 0, endTime = 0;
	int statusValue=-1;
	bool includeTimingValues = false;
	msgpack_zone mempool;
	msgpack_object deserialized;
	msgpack_unpack_return unpack_ret;
	money_trace_spans *timeSpan = NULL;
	const char *recivedMsg = NULL;
	recivedMsg =  (const char *) nopoll_msg_get_payload (msg);
	if(recivedMsg!=NULL) 
	{
		startTime = getCurrentTimeInMicroSeconds(&start1);
		WalPrint("WEBPA start_time : %llu\n",startTime);
		msgSize  = nopoll_msg_get_payload_size (msg);

		getCurrentTime(startPtr);

		msgpack_zone_init(&mempool, 2048);
		unpack_ret = msgpack_unpack(recivedMsg, msgSize, NULL, &mempool, &deserialized);
		switch(unpack_ret)
		{
			case MSGPACK_UNPACK_SUCCESS:
				WalPrint("MSGPACK_UNPACK_SUCCESS :%d\n",unpack_ret);

				if(deserialized.via.map.size != 0) 
				{
					decodeRequest(deserialized,&msgType,&source,&dest,&transaction_id,&statusValue,&payload,&includeTimingValues);
				}
				msgpack_zone_destroy(&mempool);
				
				// allocate memory only when include_timing_values is true
				WalPrint("includeTimingValues : %d\n",includeTimingValues);
				//includeTimingValues = true;
				if(includeTimingValues)
				{
					timeSpan = (money_trace_spans *) malloc(sizeof(money_trace_spans));
					memset(timeSpan,0,(sizeof(money_trace_spans)));
				}
				else
				{
				        WalPrint("includeTimingValues is false\n");
				}
				
				switch(msgType) 
				{ 
					case WRP_MSG_AUTH_STATUS:
						WalInfo("Authorization Status received with Status code :%d\n",statusValue);
						break;

					case WRP_MSG_REQUEST_RESPONSE:
						request=cJSON_Parse(payload);

						if(request != NULL)
						{
							cJSON *response = cJSON_CreateObject();

							out = cJSON_PrintUnformatted(request);
							WalInfo("Request: %s\n", out);
							WalInfo("transaction_id in request: %s\n",transaction_id);
							WAL_FREE(out);
							
							if(!webpaReady)
							{
								cJSON_AddNumberToObject(response, "statusCode", 530);	
								cJSON_AddStringToObject(response, "message", "Webpa not yet Ready to process Request");
								endTime = getCurrentTimeInMicroSeconds(&end1);	
								duration = endTime - startTime;
								WalPrint("WEBPA duration : %lu\n", duration);
								if(timeSpan)
								{
								        add_total_webpa_client_time(startTime,duration,timeSpan);
							        }
								packAndSendResponse(response, msgType, source, dest, transaction_id, timeSpan, conn);
								getCurrentTime(endPtr);
								WalInfo("Elapsed time : %ld ms\n", timeValDiff(startPtr, endPtr));
							}
							else
							{
								if((dest != NULL) && (strstr(dest, "iot") != NULL))
								{
									WalInfo("IoT Request\n");
									processIoTRequest(request, response);
									packAndSendResponse(response, msgType, source, dest, transaction_id, NULL, conn);
									getCurrentTime(endPtr);
									WalInfo("Elapsed time : %ld ms\n", timeValDiff(startPtr, endPtr));
								}
								else
								{
									command = cJSON_GetObjectItem(request, "command")->valuestring;
								
									WalPrint("\ncommand %s\n",(command == NULL) ? "NULL" : command);

									if( command != NULL) 
									{
										out = cJSON_Print(request);
										if (strcmp(command, "GET") == 0)
										{
											WalPrint("Get Request %s\n", out);
											processGetRequest(request, timeSpan, response);
										}
										else if (strcmp(command, "GET_ATTRIBUTES") == 0)
										{
											WalPrint("Get attribute Request %s\n", out);
											processGetAttrRequest(request, timeSpan, response);
										}
										else if (strcmp(command, "SET") == 0)
										{
											WalPrint("\nSet Request: %s\n", out);
											processSetRequest(request, WEBPA_SET, timeSpan, response,transaction_id);
											
										}
										else if(strcmp(command, "SET_ATTRIBUTES") == 0)
										{
											WalPrint("\nSet attribute Request: %s\n", out);											
											processSetAttrRequest(request, timeSpan, response);
										}
									
										else if (strcmp(command, "SET_ATOMIC") == 0)
										{
											WalPrint("\nSet Atomic Request: %s\n", out);
											processSetRequest(request, WEBPA_ATOMIC_SET, timeSpan, response,transaction_id);
										}
										else if (strcmp(command, "TEST_AND_SET") == 0)
										{
											WalPrint("\nTest and Set Request: %s\n", out);
											processTestandSetRequest(request, timeSpan, response,transaction_id);
										}
										else if (strcmp(command, "REPLACE_ROWS") == 0)
										{
											WalPrint("\n REPLACE_ROWS Request: %s\n", out);
											processReplaceRowRequest(request, response);
										}
										else if (strcmp(command, "ADD_ROW") == 0)
										{
											WalPrint("\n ADD_ROW Request: %s\n", out);
											processAddRowRequest(request, response);
										}
										else if (strcmp(command, "DELETE_ROW") == 0)
										{
											WalPrint("\n DELETE_ROW Request: %s\n", out);
											processDeleteRowRequest(request, response);
										}
										WAL_FREE(out);
										endTime = getCurrentTimeInMicroSeconds(&end1);	
										duration = endTime - startTime;
										WalPrint("WEBPA duration : %lu\n", duration);
										if(timeSpan)
										{
								                        add_total_webpa_client_time(startTime,duration,timeSpan);
								                }
										packAndSendResponse(response, msgType, source, dest, transaction_id, timeSpan, conn);
										getCurrentTime(endPtr);
										WalInfo("Elapsed time : %ld ms\n", timeValDiff(startPtr, endPtr));
									}
								}
							}
							if(request != NULL)
							{
								cJSON_Delete(request);
							}
							if(response != NULL)
							{
								cJSON_Delete(response);
							} 
							WalPrint("Done freeing up request and response...\n");
						}
						else
						{
							WalInfo("Empty payload\n");
						}
						break;

					default:
						WalError("Invalid message type: %d\n", msgType);
				}
				if (NULL != payload) // We don't always have a payload
				{ 
					WAL_FREE(payload);
				}
				break;

			case MSGPACK_UNPACK_EXTRA_BYTES:
			case MSGPACK_UNPACK_CONTINUE:
			case MSGPACK_UNPACK_PARSE_ERROR:
			case MSGPACK_UNPACK_NOMEM_ERROR:
			default:
				WalError("Message Pack decode failed with error: %d\n", unpack_ret);	
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
		WalInfo("Ping received with payload %s, opcode %d\n",payload, nopoll_msg_opcode(msg));
		if (nopoll_msg_opcode(msg) == NOPOLL_PING_FRAME) 
		{
			nopoll_conn_send_frame (conn, nopoll_true, nopoll_true, NOPOLL_PONG_FRAME, strlen(payload), payload, 0);
			heartBeatTimer = 0;
			WalPrint("Sent Pong frame and reset HeartBeat Timer\n");
		}
	}
}

static void listenerOnCloseMessage (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data)
{
	WalPrint("listenerOnCloseMessage(): mutex lock in producer thread\n");
	WAL_STATUS setReconnectStatus = WAL_FAILURE;
	
	if((user_data != NULL) && (strstr(user_data, "SSL_Socket_Close") != NULL) && !LastReasonStatus)
	{
		WalInfo("Reconnect detected, setting Reconnect reason as Server close\n");
		setReconnectStatus = setSyncParam(SYNC_PARAM_RECONNECT_REASON, "Server_closed_connection");
				
		if(setReconnectStatus != WAL_SUCCESS)
		{
			WalError("Error setting LastReconnectReason for Server_closed_connection\n");
		}
		LastReasonStatus = true; 
		
	}
	else if ((user_data == NULL) && !LastReasonStatus)
	{
		WalInfo("Reconnect detected, setting Reconnect reason as Unknown\n");
		setReconnectStatus = setSyncParam(SYNC_PARAM_RECONNECT_REASON, "Unknown");
				
		if(setReconnectStatus != WAL_SUCCESS)
		{
			WalError("Error setting LastReconnectReason for Unknown\n");
		}
		
	}

	pthread_mutex_lock (&close_mut);
	close_retry = true;
	pthread_mutex_unlock (&close_mut);
	WalPrint("listenerOnCloseMessage(): mutex unlock in producer thread\n");
}

/*
* @brief Returns the value of a given key.
* @param[in] key key name with message type string.
*/
static char* getKey_MsgtypeStr(const msgpack_object key, const size_t keySize, char* keyString)
{
	const char* keyName = key.via.str.ptr;
	walStrncpy(keyString, keyName, keySize+1);
	return keyString;
}

/*
 * @brief Returns the value of a given key.
 * @param[in] key key name with message type binary.
 */
static char* getKey_MsgtypeBin(const msgpack_object key, const size_t binSize, char* keyBin)
{
	const char* keyName = key.via.bin.ptr;
	memcpy(keyBin, keyName, binSize);
	keyBin[binSize] = '\0';
	return keyBin;
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
        WalPrint("timer->tv_sec : %lu\n",timer->tv_sec);
        WalPrint("timer->tv_nsec : %lu\n",timer->tv_nsec);
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

void add_total_webpa_client_time(uint64_t startTime,uint32_t duration,money_trace_spans *timeSpan)
{
        WalPrint("---------------- Start of add_total_webpa_client_time ----------------\n");
        if(timeSpan->count == 0)
        {
                timeSpan->count = timeSpan->count +1;
                WalPrint("timeSpan->count : %d\n",timeSpan->count);
                timeSpan->spans = (money_trace_span *) malloc(sizeof(money_trace_span)* timeSpan->count); 
        }
        else
        {
                timeSpan->count = timeSpan->count +1;
                WalPrint("timeSpan->count : %d\n",timeSpan->count);
                timeSpan->spans = (money_trace_span *) realloc(timeSpan->spans,sizeof(money_trace_span)* timeSpan->count); 
        }
        
        timeSpan->spans[timeSpan->count - 1].name = (char *)malloc(sizeof(char)*MAX_PARAMETERNAME_LEN/2);
	strcpy(timeSpan->spans[timeSpan->count - 1].name,WEBPA_CLIENT);
	WalPrint("timeSpan->spans[%d].name : %s\n",timeSpan->count - 1,timeSpan->spans[timeSpan->count - 1].name);
	WalPrint("startTime : %llu\n",startTime);
	timeSpan->spans[timeSpan->count - 1].start = startTime;
	WalPrint("timeSpan->spans[%d].start : %llu\n",timeSpan->count - 1,timeSpan->spans[timeSpan->count - 1].start);
	WalPrint("timeDuration : %lu\n",duration);
	timeSpan->spans[timeSpan->count - 1].duration = duration;
	WalPrint("timeSpan->spans[%d].duration : %lu\n",timeSpan->count - 1,timeSpan->spans[timeSpan->count - 1].duration);
	WalPrint("---------------- End of add_total_webpa_client_time ----------------\n");
}
/*
 * @brief To get the Device Mac Address.
 * 
 * Device MAC will be CM MAC for RDKB devices and STB MAC for RDKV devices
 */
static void getDeviceMac()
{
	int paramCount = 0;
	int *count = NULL;
	int retryCount = 0;
	char deviceMACValue[32]={'\0'};

	if(strlen(deviceMAC) == 0)
	{
		const char *getParamList[]={WEBPA_DEVICE_MAC};
		paramCount = sizeof(getParamList)/sizeof(getParamList[0]);
		WAL_STATUS *ret = (WAL_STATUS *) malloc(sizeof(WAL_STATUS) * paramCount);
		count = (int *) malloc(sizeof(int) * paramCount);
		ParamVal ***parametervalArr = (ParamVal ***) malloc(sizeof(ParamVal **) * paramCount);
		do{
			getValues(getParamList, paramCount, NULL, parametervalArr, count, ret);
			 
			if (ret[0] == WAL_SUCCESS )
			{
				walStrncpy(deviceMACValue, parametervalArr[0][0]->value,sizeof(deviceMACValue));
				macToLower(deviceMACValue,deviceMAC);
				retryCount = 0;

				if(parametervalArr[0] && parametervalArr[0][0])
				{
					WAL_FREE(parametervalArr[0][0]->name);
					WAL_FREE(parametervalArr[0][0]->value);
				}
				
				if(parametervalArr[0])
				{
					WAL_FREE(parametervalArr[0][0]);
				}
				
				WAL_FREE(parametervalArr[0]);
			}
			else
			{
				WalError("Failed to GetValue for MAC. Retrying...\n");
				retryCount++;
				sleep(webPaCfg.retryIntervalInSec);
			}
		}while((retryCount >= 1) && (retryCount <= 5));

		WAL_FREE(ret);
		WAL_FREE(count);
		WAL_FREE(parametervalArr); 

		WalPrint("retryCount %d\n", retryCount);
		if(retryCount == 6)
		{
			WalError("Unable to get CM Mac. Terminate the connection\n");
			exit(1);
		}
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
			WalError("Error creating HeartBeat thread :[%s]\n", strerror(err));
		}
		else 
		{
			WalPrint("heartBeatHandlerTask Thread created successfully\n");
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
				WalError("ping wait time > %d. Terminating the connection with WebPA server and retrying\n", webPaCfg.maxPingWaitTimeInSec);
				
				WalInfo("Reconnect detected, setting Ping_Miss reason for Reconnect\n");
				setReconnectStatus = setSyncParam(SYNC_PARAM_RECONNECT_REASON, "Ping_Miss");
				
				if(setReconnectStatus != WAL_SUCCESS)
				{
					WalError("Error setting LastReconnectReason for ping miss\n");
				}
				LastReasonStatus = true;
				pthread_mutex_lock (&close_mut);
				close_retry = true;
				pthread_mutex_unlock (&close_mut);
			}
			else
			{
				WalPrint("heartBeatHandlerTask - close_retry set to %d, hence resetting the heartBeatTimer\n",close_retry);
			}
			heartBeatTimer = 0;
		}
		else 
		{
			WalPrint("heartBeatTimer %d\n",heartBeatTimer);
			heartBeatTimer += HEARTBEAT_RETRY_SEC;			
		}
	}
	heartBeatTimer = 0;
	return NULL;
}

/*
 * @brief packString is to pack the given string using msgpack_packer
 *  @param[in] notifEncoder msgpack packer *
 *  @param[in] str string to be packed
 */
static void packString(msgpack_packer *notifEncoder, char * str) 
{
	msgpack_pack_str(notifEncoder, strlen(str));
	msgpack_pack_str_body(notifEncoder, str, strlen(str));
}

/**
 * @brief packInitialComponentsToResponse packs source,dest,transaction id to the response
 *
 * @param[in] encodeResponseVal response pack object
 * @param[in] msg_type type of the message
 * @param[in] source source from which the response is generated
 * @param[in] dest Destination to which it has to be sent
 * @param[in] transaction_id Id 
 * @param[in] timeSpan timing_values for each component
 */
static void packInitialComponentsToResponse(msgpack_packer encodeResponseVal,int msg_type,char *source,char *dest, char *transaction_id, money_trace_spans *timeSpan)
{
	msgpack_pack_str(&encodeResponseVal, strlen(WRP_MSG_TYPE));
	msgpack_pack_str_body(&encodeResponseVal, WRP_MSG_TYPE,strlen(WRP_MSG_TYPE));

	msgpack_pack_int(&encodeResponseVal, msg_type);

	msgpack_pack_str(&encodeResponseVal, strlen(WRP_SOURCE));
	msgpack_pack_str_body(&encodeResponseVal, WRP_SOURCE,strlen(WRP_SOURCE));

	msgpack_pack_str(&encodeResponseVal, strlen(dest));
	msgpack_pack_str_body(&encodeResponseVal, dest,strlen(dest));

	msgpack_pack_str(&encodeResponseVal, strlen(WRP_DESTINATION));
	msgpack_pack_str_body(&encodeResponseVal, WRP_DESTINATION,strlen(WRP_DESTINATION));

	msgpack_pack_str(&encodeResponseVal, strlen(source));
	msgpack_pack_str_body(&encodeResponseVal, source,strlen(source));

	msgpack_pack_str(&encodeResponseVal, strlen(WRP_TRANSACTION_ID));
	msgpack_pack_str_body(&encodeResponseVal, WRP_TRANSACTION_ID,strlen(WRP_TRANSACTION_ID));

	msgpack_pack_str(&encodeResponseVal, strlen(transaction_id));
	msgpack_pack_str_body(&encodeResponseVal, transaction_id,strlen(transaction_id));

	msgpack_pack_str(&encodeResponseVal, strlen(WRP_CONTENT_TYPE));
	msgpack_pack_str_body(&encodeResponseVal, WRP_CONTENT_TYPE,strlen(WRP_CONTENT_TYPE));

	msgpack_pack_str(&encodeResponseVal, strlen(CONTENT_TYPE_JSON));
	msgpack_pack_str_body(&encodeResponseVal, CONTENT_TYPE_JSON,strlen(CONTENT_TYPE_JSON));
	
	if(timeSpan != NULL)
	{
		WalPrint("packing include_timing_values\n");
		msgpack_pack_str(&encodeResponseVal, strlen(WRP_INCLUDE_SPANS));
		msgpack_pack_str_body(&encodeResponseVal, WRP_INCLUDE_SPANS,strlen(WRP_INCLUDE_SPANS));
		
		msgpack_pack_true(&encodeResponseVal);
		
		msgpack_pack_str(&encodeResponseVal, strlen(WRP_SPANS));
		msgpack_pack_str_body(&encodeResponseVal, WRP_SPANS,strlen(WRP_SPANS));
		
		if(timeSpan->count != 0 && timeSpan->spans != NULL)
		{
			unsigned int cnt=0;
			WalPrint("packing timeSpan\n");
			msgpack_pack_array(&encodeResponseVal, (timeSpan->count));
			
			WalPrint("timeSpan->count : %d\n",timeSpan->count);
			for(cnt = 0; cnt < timeSpan->count; cnt++)
			{
				msgpack_pack_array(&encodeResponseVal, 3);
				
				WalPrint("packing timeSpan->spans[cnt].name\n");				
				WalPrint("timeSpan->spans[cnt].name : %s\n",timeSpan->spans[cnt].name);
				msgpack_pack_str(&encodeResponseVal, strlen(timeSpan->spans[cnt].name));
				msgpack_pack_str_body(&encodeResponseVal, timeSpan->spans[cnt].name,strlen(timeSpan->spans[cnt].name));
				
				WalPrint("packing timeSpan->spans[cnt].start\n");			
				WalPrint("timeSpan->spans[cnt].start : %llu\n",timeSpan->spans[cnt].start);
				msgpack_pack_uint64(&encodeResponseVal, timeSpan->spans[cnt].start);
				
				WalPrint("packing timeSpan->spans[cnt].duration \n");			
				WalPrint("timeSpan->spans[cnt].duration : %lu\n",timeSpan->spans[cnt].duration);
				msgpack_pack_uint32(&encodeResponseVal, timeSpan->spans[cnt].duration);
				
				WalPrint("freeing timeSpan->spans[cnt].name\n");
				WAL_FREE(timeSpan->spans[cnt].name);	
				
			}
			WalPrint("freeing timeSpan->spans\n");
			WAL_FREE(timeSpan->spans);	
		}
		
		WalPrint("Freeing timeSpan\n");
		WAL_FREE(timeSpan);
	}
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
			WalError("Error while reading webpa config file under %s\n", WEBPA_CFG_FILE_SRC);
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
			WalError("Error while writing webpa config file under /nvram.\n");
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
		WalInfo("Copied webpa_cfg.json to /nvram\n");
	}
	else 
	{
		WalInfo("webpa_cfg.json already exist in /nvram\n");
	}
	return WAL_SUCCESS;
}

/**
 * @brief packAndSendResponse function to pack the response and send to server.
 *
 * @param[in] response respose to be packed.
 * @param[in] msgType type of the message
 * @param[in] source source from which the response is generated
 * @param[in] dest Destination to which it has to be sent
 * @param[in] transaction_id Id 
 * @param[in] encodeResponse response to be encoded
 * @param[in] encodeBuffer buffer where response is stored
 * @param[in] timeSpan timing_values for each component
 * @param[in] conn Websocket connection object
 */
static void packAndSendResponse(cJSON *response, int msgType, char *source, char *dest, char *transaction_id, money_trace_spans *timeSpan,  noPollConn * conn)
{
	char *str;
	int payload_pack_ret;
	msgpack_sbuffer encodeBuffer;
	msgpack_packer encodeResponse;
	
	str = cJSON_PrintUnformatted(response);
	WalInfo("Payload Response: %s\n", str);
	
	if( msgType!=WRP_MSG_INVALID && source!=NULL && dest!=NULL && transaction_id!=NULL ) 
	{
	   	msgpack_sbuffer_init(&encodeBuffer);
		msgpack_packer_init(&encodeResponse, &encodeBuffer, msgpack_sbuffer_write);
		if(timeSpan != NULL)
		{
			msgpack_pack_map(&encodeResponse, WRP_MAP_SIZE+2);
		}
		else
		{
			msgpack_pack_map(&encodeResponse, WRP_MAP_SIZE);
		}
		packInitialComponentsToResponse(encodeResponse,msgType,source,dest,transaction_id, timeSpan);
		msgpack_pack_str(&encodeResponse,strlen(WRP_PAYLOAD));
		msgpack_pack_str_body(&encodeResponse,WRP_PAYLOAD,strlen(WRP_PAYLOAD));   
		msgpack_pack_bin( &encodeResponse, strlen(str) );
		payload_pack_ret = msgpack_pack_bin_body( &encodeResponse, str, strlen(str) );
		if (payload_pack_ret ==0)
		{
			WalPrint("Response payload length: %d\n", strlen(str));
			WalPrint("Msgpack encoded response: %s\n",encodeBuffer.data);
			WalPrint("Msgpack encoded response length: %d\n",encodeBuffer.size);
			sendResponse(conn,encodeBuffer.data,(int)encodeBuffer.size);
		}
		else
		{
			WalError("Payload Encode UnSuccessful: %d\n", payload_pack_ret);	
		}
		msgpack_sbuffer_destroy(&encodeBuffer);
	}
	WAL_FREE(source);
	WAL_FREE(dest);
	WAL_FREE(transaction_id);
	WAL_FREE(str);
}

/**
 * @brief decodeRequest function to unpack the request received from server.
 *
 * @param[in] deserialized msgpack object to deserialize
 * @param[in] msg_type type of the message
 * @param[in] source_ptr source from which the response is generated
 * @param[in] dest_ptr Destination to which it has to be sent
 * @param[in] transaction_id_ptr Id 
 * @param[in] statusValue status value of authorization request
 * @param[in] payload_ptr bin payload to be extracted from request
 */
static void decodeRequest(msgpack_object deserialized,int *msgType, char** source_ptr,char** dest_ptr,char** transaction_id_ptr,int *statusValue,char** payload_ptr, bool *includeTimingValues)
{
	int i=0;
	int keySize =0;
	char* keyString =NULL;
	char* keyName =NULL;
	int sLen=0;
	int binValueSize=0,StringValueSize = 0;
	char* NewStringVal, *StringValue;
	char* keyValue =NULL;
	char *transaction_id = NULL;
	char *source=NULL;
	char *dest=NULL;
	char *payload=NULL;
	
	*transaction_id_ptr = NULL;
	*source_ptr=NULL;
	*dest_ptr=NULL;
	*payload_ptr=NULL;
	
	msgpack_object_kv* p = deserialized.via.map.ptr;
	while(i<deserialized.via.map.size) 
	{
		sLen =0;
		msgpack_object keyType=p->key;
		msgpack_object ValueType=p->val;
		keySize = keyType.via.str.size;
		keyString = (char*)malloc(keySize+1);
		keyName = NULL;
		keyName = getKey_MsgtypeStr(keyType, keySize, keyString);
		WalPrint("keyName : %s\n",keyName);
		if(keyName!=NULL) 
		{
			switch(ValueType.type) 
			{ 
				case MSGPACK_OBJECT_POSITIVE_INTEGER:
				{ 
					if(strcmp(keyName,WRP_MSG_TYPE)==0)
					{
						*msgType=ValueType.via.i64;
					}
					else if(strcmp(keyName,WRP_STATUS)==0)
					{
						*statusValue=ValueType.via.i64;
					}
				}
				break;
				case MSGPACK_OBJECT_BOOLEAN:
				{
				        WalPrint("Inside boolean\n");
					if(strcmp(keyName,WRP_INCLUDE_SPANS)==0)
					{
						*includeTimingValues=ValueType.via.boolean ? true : false;
						WalPrint("includeTimingValues : %d\n",*includeTimingValues);
					}
				}
				break;
				case MSGPACK_OBJECT_NIL:
				{
				        WalPrint("Inside nil\n");
				        if(strcmp(keyName,WRP_SPANS)==0)
				        {
				                WalPrint("spans is nil\n");
				        }
				        
                                }
                                break;
				case MSGPACK_OBJECT_STR:
				{
					StringValueSize = ValueType.via.str.size;
					NewStringVal = (char*)malloc(StringValueSize+1);
					StringValue = getKey_MsgtypeStr(ValueType,StringValueSize,NewStringVal);

					if(strcmp(keyName,WRP_SOURCE) == 0) 
					{
						sLen=strlen(StringValue);
						source=(char *)malloc(sLen+1);
						walStrncpy(source,StringValue,sLen+1);
						*source_ptr = source;
					}
					else if(strcmp(keyName,WRP_DESTINATION) == 0) 
					{
						sLen=strlen(StringValue);
						dest=(char *)malloc(sLen+1);
						walStrncpy(dest,StringValue,sLen+1);
						*dest_ptr = dest;
					}
					else if(strcmp(keyName,WRP_TRANSACTION_ID) == 0)
					{
						sLen=strlen(StringValue);
						transaction_id=(char *)malloc(sLen+1);
						walStrncpy(transaction_id,StringValue,sLen+1);
						*transaction_id_ptr = transaction_id;
					}
					
					WAL_FREE(NewStringVal);
				}
				break;

				case MSGPACK_OBJECT_BIN:
				{
					if(strcmp(keyName,WRP_PAYLOAD) == 0) 
					{
						binValueSize = ValueType.via.bin.size;
						payload = (char*)malloc(binValueSize+1);
						keyValue = NULL;
						keyValue = getKey_MsgtypeBin(ValueType, binValueSize, payload); 
						if(keyValue!=NULL)
						{ 
							WalPrint("Binary payload %s\n",keyValue);	
						}
						*payload_ptr = keyValue;
					}
				}
				break;

				default:
				        WalError("ValueType.type %d\n",ValueType.type);
					WalError("Unknown Data Type");
			}
		}
		p++;
		i++;
		WAL_FREE(keyString);
	}
}

/*
 * @brief To initiate WebPA Task handling
 */
static void initWebpaTask()
{
	int err = 0;
	pthread_t webpaTaskThreadId;
	notifyMsgQ = NULL;

	err = pthread_create(&webpaTaskThreadId, NULL, webpaTask, NULL);
	if (err != 0) 
	{
		WalError("Error creating webpa task thread :[%s]\n", strerror(err));
	}
	else
	{
		WalPrint("Webpa Task thread created Successfully\n");
	}
}

/*
 * @brief To handle Webpa tasks
 */
static void *webpaTask()
{
	waitForOperationalReadyCondition();
	// Set webpaReady as OperationalReady is completed which means all dependent components are up and ready
	webpaReady = true;
	WalInfo("WebPA is now ready to process requests\n");
        sendParamNotifyToServer(WEBPA_NOTIFY_DEVICE_STATUS, -1, NULL);
	WALInit();
	RegisterNotifyCB(&notifyCallback);
	setInitialNotify();
	sendNotificationForFactoryReset();
	sendNotificationForFirmwareUpgrade();
	handleNotificationEvents();
	WalPrint ("webpaTask ended!\n");
}

/*
 * @brief To handle notification during Factory reset
 */
static void sendNotificationForFactoryReset()
{
	// GET CMC and CID value, if both equal to 0 then send notification to server with cmc 1 and cid 0
	char *dbCMC = NULL, *dbCID = NULL;
	char *factoryDef_CMC = "0";
	char *factoryDef_CID = "0";
	char newCMC[32]={'\0'};
	snprintf(newCMC, sizeof(newCMC), "%d", CHANGED_BY_FACTORY_DEFAULT);
	WAL_STATUS setCmcStatus = WAL_SUCCESS;
	
	dbCMC = getSyncParam(SYNC_PARAM_CMC);
	dbCID = getSyncParam(SYNC_PARAM_CID);
	if(dbCMC != NULL && dbCID != NULL)
	{
		if((strcmp(dbCMC,factoryDef_CMC) == 0) && (strcmp(dbCID,factoryDef_CID) == 0))
		{
			WalInfo("CMC and CID values are 0, indicating factory reset\n");
			// set CMC to the new value
			setCmcStatus = setSyncParam(SYNC_PARAM_CMC, newCMC);
			if(setCmcStatus == WAL_SUCCESS)
			{
				WalInfo("Successfully Set CMC to %d\n", atoi(newCMC));
				sendParamNotifyToServer(WEBPA_NOTIFY_XPC, atoi(newCMC), dbCID);
			}
			else
			{
				WAL_FREE(dbCID);
				WalError("Error setting CMC value for factory reset\n");
			}
		}
		else
		{
			WAL_FREE(dbCID);
		}
		WAL_FREE(dbCMC);		
	}
}

/*
 * @brief notifyCallback is to check if notification event needs to be sent
 *  @param[in] paramNotify parameters to be notified .
 */
static void notifyCallback(NotifyData *notifyData)
{
	addNotifyMsgToQueue(notifyData);
}

/*
 * @brief To add Notification message to queue 
 */
static void addNotifyMsgToQueue(NotifyData *notifyData)
{
	NotifyMsg *message;

	if (terminated) 
	{
		return;
	}

	message = (NotifyMsg *)malloc(sizeof(NotifyMsg));

	if(message)
	{
		message->notifyData = notifyData;
		message->next = NULL;
		pthread_mutex_lock (&notif_mut);
		WalPrint("addNotifyMsgToQueue :mutex lock in producer thread\n");
		if(notifyMsgQ == NULL)
		{
			notifyMsgQ = message;
			WalPrint("addNotifyMsgToQueue : Producer added message\n");
		 	pthread_cond_signal(&notif_con);
			pthread_mutex_unlock (&notif_mut);
			WalPrint("addNotifyMsgToQueue :mutex unlock in producer thread\n");
		}
		else
		{
			NotifyMsg *temp = notifyMsgQ;
			while(temp->next)
			{
				temp = temp->next;
			}
			temp->next = message;
                        pthread_mutex_unlock (&notif_mut);
		}
	}
	else
	{
		//Memory allocation failed
		WalError("Memory allocation is failed\n");
	}
	WalPrint("*****Returned from addNotifyMsgToQueue*****\n");
}

/**
 * @brief To turn on notification for the parameters extracted from the notifyList of the config file.
 */
static void setInitialNotify() 
{
	WalPrint("***************Inside setInitialNotify*****************\n");
	int i=0,isError=0,retry =0;
	char notif[20] = "";
	const char **notifyparameters=NULL;
	int notifyListSize =0;
	
	getNotifyParamList(&notifyparameters, &notifyListSize);
	
	//notifyparameters is empty for webpa-video

	if(notifyparameters != NULL)
	{
		int *setInitialNotifStatus = (int *) malloc(sizeof(int) * notifyListSize);
		int retryInterval = WEBPA_SET_INITIAL_NOTIFY_RETRY_SEC;
		WAL_STATUS *ret = NULL;
		AttrVal **attArr = NULL;
		char ** tempArr = NULL;
	
		for(i=0; i < notifyListSize;i++)
		{
			setInitialNotifStatus[i] = 0;
		}

		do
		{
			isError = 0;
			WalPrint("notify List Size: %d\n",notifyListSize);
			ret = (WAL_STATUS *) malloc(sizeof(WAL_STATUS) * 1);
			attArr = (AttrVal **) malloc(sizeof(AttrVal *) * 1);
			for(i=0; i < notifyListSize;i++) 
			{
				if(setInitialNotifStatus[i] == 0)
				{
					attArr[0] = (AttrVal *) malloc(sizeof(AttrVal) * 1);
					snprintf(notif, sizeof(notif), "%d", 1);
					attArr[0]->value = (char *) malloc(sizeof(char) * 20);
					walStrncpy(attArr[0]->value, notif, 20);
					attArr[0]->name= (char *) notifyparameters[i];
					attArr[0]->type = WAL_INT;
					WalPrint("notifyparameters[%d]: %s\n", i, notifyparameters[i]);
					setAttributes(&notifyparameters[i], 1, NULL, (const AttrVal **) attArr, ret);

					if(ret[0] != WAL_SUCCESS) 
					{
						isError = 1;
						setInitialNotifStatus[i] = 0;
						WalError("Failed to turn notification ON for parameter : %s ret: %d Attempt Number: %d\n", notifyparameters[i], ret[0],retry+1);
					}
					else
					{
						setInitialNotifStatus[i] = 1;	
						WalInfo("Successfully set notification ON for parameter : %s ret: %d\n",notifyparameters[i], ret[0]);
					}
					WAL_FREE(attArr[0]->value);
					WAL_FREE(attArr[0]);
				}
			}

			WAL_FREE(ret);
			WAL_FREE(attArr);

			if(isError == 0)
			{
				WalInfo("Successfully set initial notifications\n");
				break;
			}

			retryInterval = retryInterval * 2;
			sleep(retryInterval);
			WalInfo("retryInterval: %d, retry: %d\n",retryInterval,retry);
		}
		while(retry++ < WEBPA_SET_INITIAL_NOTIFY_RETRY_COUNT);

		WAL_FREE(setInitialNotifStatus);
	
		WalPrint("**********************End of setInitial Notify************************\n");
	}
	else
	{
		WalInfo("Initial Notification list is empty\n");
	}
}
/*
 * @brief To monitor notification events in Notification queue
 */
static void handleNotificationEvents()
{
	while(1)
	{
		pthread_mutex_lock (&notif_mut);
		WalPrint("handleNotificationEvents : mutex lock in consumer thread\n");
		if(notifyMsgQ != NULL)
		{
			NotifyMsg *message = notifyMsgQ;
			notifyMsgQ = notifyMsgQ->next;
			pthread_mutex_unlock (&notif_mut);
			WalPrint("handleNotificationEvents : mutex unlock in consumer thread\n");
			NotifyData *notifyData = message->notifyData;
			Notify_Data *unionData = notifyData->data;

			if (terminated) 
			{
				freeNotifyMessage(message);
				continue;
			}

			if(notifyData->type == PARAM_NOTIFY)
			{
				ParamNotify *paramNotify = unionData->notify;
				handleParamNotifyMessage(paramNotify);
			}
			else if(notifyData->type == UPSTREAM_MSG)
			{
				UpstreamMsg *upstreamMsg = unionData->msg;
				handleUpstreamMessage(upstreamMsg);
			}
			else if(notifyData->type == CONNECTED_CLIENT_NOTIFY)
			{
                                WalPrint("----- Inside CONNECTED_CLIENT_NOTIFY type -----\n");
                                char *nodeData=NULL;
                                int len = 0;
                                char nodeMAC[32]={'\0'};
                                NodeData * nodePtr= unionData->node;
                                
                                if(nodePtr != NULL)
                                {
                                        if(nodePtr->status != NULL)
                                        {
                                                len += strlen(nodePtr->status);
                                        }
                                        
                                        if(nodePtr->nodeMacId != NULL)
                                        {
                                                macToLower(nodePtr->nodeMacId, nodeMAC);
                                                WalPrint("nodeMAC %s, nodePtr->nodeMacId %s\n",nodeMAC,nodePtr->nodeMacId);
                                                len += strlen(nodeMAC);
                                        }
                                        
                                        if(len > 0)
                                        {
                                                len += strlen("/unknown/");
                                                nodeData = (char *)(malloc(sizeof(char) * (len + 1)));
                                                //E.g. connected/unknown/112233445566
                                                snprintf(nodeData, len + 1, "%s/unknown/%s",((NULL != nodePtr->status) ? nodePtr->status : "unknown"), 
                                                ((NULL != nodePtr->nodeMacId) ? nodeMAC : "unknown"));
                                                WalPrint("nodeData : %s\n",nodeData);
                                        }
                                        
                                        sendParamNotifyToServer(WEBPA_NOTIFY_NODE_CHANGE, -1 , nodeData);
                                        WAL_FREE(nodePtr->status);
                                        WAL_FREE(nodePtr->nodeMacId);
                                }
			}
			else if(notifyData->type == TRANS_STATUS)
			{
			    TransData *transData = unionData->status;
			    sendParamNotifyToServer(WEBPA_NOTIFY_TRANS_STATUS, -1 , transData->transId);
			}
			
			freeNotifyMessage(message);
		}
		else
		{		
			if (terminated) 
			{
  				pthread_mutex_unlock (&notif_mut);
				break;
			}
			WalPrint("handleNotificationEvents : Before pthread cond wait in consumer thread\n");   
			pthread_cond_wait(&notif_con, &notif_mut);
			pthread_mutex_unlock (&notif_mut);
			WalPrint("handleNotificationEvents : mutex unlock in consumer thread after cond wait\n");
		}
	}
}

static void handleUpstreamMessage(UpstreamMsg *upstreamMsg)
{
	int bytesWritten = 0;
	WalInfo("handleUpstreamMessage length %d\n", upstreamMsg->msgLength);
	if(nopoll_conn_is_ok(conn) && nopoll_conn_is_ready(conn))
	{
		bytesWritten = nopoll_conn_send_binary(conn, upstreamMsg->msg, upstreamMsg->msgLength);
		WalPrint("Number of bytes written: %d\n", bytesWritten);
		if (bytesWritten != upstreamMsg->msgLength) 
		{
			WalError("Failed to send bytes %d, bytes written were=%d (errno=%d, %s)..\n", upstreamMsg->msgLength, bytesWritten, errno, strerror(errno));
		}
	}
	else
	{
		WalError("Failed to send notification as connection is not OK\n");
	}
	WAL_FREE(upstreamMsg->msg);
	WalPrint("After upstreamMsg->msg free\n");
}

static void handleParamNotifyMessage(ParamNotify *paramNotify)
{
	char *strCMC = NULL, *strCID = NULL;
	char strNewCMC[20] = {'\0'};
	unsigned int oldCMC, newCMC;
	WAL_STATUS status = WAL_FAILURE;	

	if(strcmp(paramNotify->paramName, "IOT") != 0)
	{
		strCMC = getSyncParam(SYNC_PARAM_CMC);
		if(strCMC != NULL)
		{
			oldCMC = atoi(strCMC);
			newCMC = oldCMC | paramNotify->changeSource;
			WalInfo("Notification received from stack Old CMC: %s (%d), newCMC: %d\n", strCMC, oldCMC, newCMC);

					WAL_FREE(strCMC);
					if(newCMC != oldCMC)
					{
						WalPrint("NewCMC and OldCMC not equal.\n");
						sprintf(strNewCMC, "%d", newCMC);
						status = setSyncParam(SYNC_PARAM_CMC,strNewCMC);

						if(status == WAL_SUCCESS)
						{ 
							WalPrint("Successfully set newCMC value %s\n",strNewCMC);
						
							strCID = getSyncParam(SYNC_PARAM_CID);
							if(strCID != NULL)
							{
								WalPrint("ConfigID string is : %s\n", strCID);

								// send Notification only when CMC changes
								sendParamNotifyToServer(WEBPA_NOTIFY_XPC, newCMC, strCID);
							}
							else
							{
								WalError("Failed to Get CID value\n");
							}
						}
						else
						{
							WalError("Error in setting new CMC value\n");
						}
					}
					else
					{
						WalPrint("No change in CMC value, ignoring value change event\n");
					}
				}
				else
				{
					WalError("Failed to Get CMC Value, hence ignoring the notification\n");
				}
			}
			else // Handle IoT notification
			{
				if(paramNotify->newValue != NULL)
				{
					sendParamNotifyToServer(WEBPA_NOTIFY_IOT, 0, paramNotify->newValue);
					WAL_FREE(paramNotify->paramName);
				}
			}
}

/*
 * @brief To form, pack and send notification event message to server
 */
static void sendParamNotifyToServer(WEBPA_NOTIFY_TYPE notifyType,
                                    unsigned int cmc, const char *str)
{
	/* Form the notification message, pack and send. */
	char device_id[32]={'\0'};
	int bytesWritten;
	msgpack_packer  notifyEncoder;
	msgpack_sbuffer notifyBuf;

	cJSON * notifyPayload;
        cJSON * nodes, *one_node;
	char  * stringifiedNotifyPayload;

        /* Error Checking */
        switch(notifyType) 
	{
            case WEBPA_NOTIFY_XPC:
            case WEBPA_NOTIFY_IOT:
            case WEBPA_NOTIFY_DEVICE_STATUS:                
            case WEBPA_NOTIFY_NODE_CHANGE:
            case WEBPA_NOTIFY_TRANS_STATUS:
            /* Valid Request */
            WalPrint("sendNotifyToServer() notifyType: %d, cmc: %d, str: '%s'\n",
                      notifyType, cmc, str ? str : "unused");
            break;
                         
            default:
            WalPrint("sendParamNotifyToServer() Error: invalid request %d\n",
                      notifyType);
            return;
        }   
        

	/* Prepare payload JSON */
	notifyPayload = cJSON_CreateObject();

	snprintf(device_id, sizeof(device_id), "mac:%s", deviceMAC);
	WalPrint("Device_id %s\n",device_id);
	cJSON_AddStringToObject(notifyPayload,"device_id", device_id);
        
	msgpack_sbuffer_init(&notifyBuf);
	msgpack_packer_init(&notifyEncoder, &notifyBuf, msgpack_sbuffer_write);

	msgpack_pack_map(&notifyEncoder, (WRP_MAP_SIZE-1));

	packString(&notifyEncoder, WRP_MSG_TYPE);
	msgpack_pack_int(&notifyEncoder, WRP_MSG_NOTIFY);
	packString(&notifyEncoder, WRP_CONTENT_TYPE);
	packString(&notifyEncoder, CONTENT_TYPE_JSON);
	packString(&notifyEncoder, WRP_SOURCE);
	packString(&notifyEncoder, device_id);
	packString(&notifyEncoder, WRP_DESTINATION);
        
        switch(notifyType)
 	{
            case WEBPA_NOTIFY_XPC:
                packString(&notifyEncoder, "event:SYNC_NOTIFICATION");
 		cJSON_AddNumberToObject(notifyPayload,"cmc", cmc);
		cJSON_AddStringToObject(notifyPayload,"cid", str);
                break;
                
            case WEBPA_NOTIFY_IOT:
                packString(&notifyEncoder, "event:IOT_NOTIFICATION");
                cJSON_AddStringToObject(notifyPayload,"iot", str);
                break;
                
            case WEBPA_NOTIFY_DEVICE_STATUS:
                /* FixMe:: Uh...should we use same format as IOT/SYNC ? */
                packString(&notifyEncoder, "event:device-status");
                cJSON_AddStringToObject(notifyPayload,"status", "operational");
               break;
                
            case WEBPA_NOTIFY_NODE_CHANGE: 
                {
                        char *st, *status = NULL, *node_mac = NULL, *version = NULL, *timeStamp = NULL;
                        struct timespec sysTime;
                        char dest[256] = {'\0'};
                        char sbuf[32] = {'\0'};
                        WalPrint("str : %s\n",str);
                        snprintf(dest,sizeof(dest),"event:node-change/mac:%s/%s",deviceMAC,((NULL != str) ? str : "unknown"));
                        WalPrint("dest : %s\n",dest);
                        version = getSyncParam(SYNC_PARAM_HOSTS_VERSION);
                        WalPrint("version : %s\n",version);
                        timeStamp = getSyncParam(SYNC_PARAM_SYSTEM_TIME);
                        if(timeStamp == NULL)
                        {
                                clock_gettime(CLOCK_REALTIME, &sysTime);
                                if( sysTime.tv_nsec > 999999999L)
                                {
                                        sysTime.tv_sec = sysTime.tv_sec + 1;
                                        sysTime.tv_nsec = sysTime.tv_nsec - 1000000000L;
                                }

                                sprintf(sbuf, "%ld.%09ld", sysTime.tv_sec, sysTime.tv_nsec);
				timeStamp = (char *) malloc (sizeof(char) * 64);
                                strcpy(timeStamp, sbuf);
                        }
                        WalPrint("timeStamp : %s\n",timeStamp);
                        packString(&notifyEncoder, dest);
                        
                        if(NULL != str && (strcmp(str,"unknown") != 0))
                        {
                                status = strtok_r(str, "/", &st);
                                strtok_r(NULL, "/", &st);
                                node_mac = strtok_r(NULL,"/", &st);
                                WalPrint("status : %s node_mac: %s\n",status,node_mac);
                        }
                        
                        cJSON_AddStringToObject(notifyPayload,"timestamp", (NULL != timeStamp) ? timeStamp : "unknown");
                        cJSON_AddItemToObject(notifyPayload, "nodes", nodes = cJSON_CreateArray());
                        cJSON_AddItemToArray(nodes, one_node = cJSON_CreateObject());
                        cJSON_AddStringToObject(one_node, "name", WEBPA_PARAM_HOSTS_NAME);
                        cJSON_AddNumberToObject(one_node, "version", (NULL != version) ? atoi(version) : 0);  
                        cJSON_AddStringToObject(one_node,"node-mac",(NULL != node_mac) ? node_mac : "unknown");
                        cJSON_AddStringToObject(one_node,"status",(NULL != status) ? status : "unknown");
                        
                        WAL_FREE(timeStamp);
                        WAL_FREE(version);
                }
                break;
            case WEBPA_NOTIFY_TRANS_STATUS : 
                {
                    packString(&notifyEncoder, "event:transaction-status");
                    cJSON_AddStringToObject(notifyPayload,"state", "complete");
                    cJSON_AddStringToObject(notifyPayload,WRP_TRANSACTION_ID, str);
                        
                }
                break;
        }

        packString(&notifyEncoder, WRP_PAYLOAD);

        stringifiedNotifyPayload = cJSON_PrintUnformatted(notifyPayload);
        WalInfo("Notification payload %s\n",stringifiedNotifyPayload);

        /* Pack payload */
        msgpack_pack_bin( &notifyEncoder, strlen(stringifiedNotifyPayload) );
        msgpack_pack_bin_body( &notifyEncoder, stringifiedNotifyPayload, strlen(stringifiedNotifyPayload) );

        if(nopoll_conn_is_ok(conn) && nopoll_conn_is_ready(conn))
        {

                bytesWritten = nopoll_conn_send_binary(conn, notifyBuf.data, notifyBuf.size);
                WalPrint("Number of bytes written: %d\n", bytesWritten);
                if (bytesWritten != notifyBuf.size) 
                {
                        WalError("Failed to send bytes %d, bytes written were=%d (errno=%d, %s)..\n",
                        notifyBuf.size, bytesWritten, errno, strerror(errno));
                }

        }
        else
        {
                WalError("Failed to send notification as connection is not OK\n");
        }

	if (str) 
	{
            WAL_FREE(str);
        }
        
	WAL_FREE(stringifiedNotifyPayload);

	cJSON_Delete(notifyPayload);
	msgpack_sbuffer_destroy(&notifyBuf);

	WalPrint("End of sendNotifyToServer()\n");
}

static void freeNotifyMessage(NotifyMsg *message)
{
	WalPrint("Inside freeNotifyMessage\n");
	NotifyData *notifyDataPtr = message->notifyData;
	if(notifyDataPtr != NULL)
	{
		Notify_Data *unionData = notifyDataPtr->data;
		if(unionData != NULL)
		{
			if(notifyDataPtr->type == PARAM_NOTIFY)
			{
				WAL_FREE(unionData->notify);
			}
			else if(notifyDataPtr->type == UPSTREAM_MSG)
			{
				WAL_FREE(unionData->msg);
			}
			else if(notifyDataPtr->type == TRANS_STATUS)
			{
			    WAL_FREE(unionData->status);
			}
			else if(notifyDataPtr->type == CONNECTED_CLIENT_NOTIFY)
			{
			    WAL_FREE(unionData->node);
			}
			WAL_FREE(unionData);
		}
		WAL_FREE(notifyDataPtr);
	}	
	WAL_FREE(message);
	WalPrint("free done from freeNotifyMessage\n");
}

/*
 * @brief To add or update webpa config file with the "oldFirmwareVersion" 
 */
static WAL_STATUS addOrUpdateFirmwareVerToConfigFile(char *value)
{
        int fileSize = 0;
        char *cfg_file_content;
        char ch;
        FILE *fp;
        char * token;
	char *cfg_oldFirmwareVer;
        char str[512] = {'\0'};
        
        fp = fopen(WEBPA_CFG_FILE, "r");
        if(fp == NULL)
        {
        	WalError("Cannot open %s in read mode\n",WEBPA_CFG_FILE);
		return WAL_FAILURE;
	}	
			
	if(ferror(fp))
	{
		WalError("Error while reading webpa config file under %s\n", WEBPA_CFG_FILE);
		fclose(fp);
		return WAL_FAILURE;
	}
        
        fseek(fp, 0, SEEK_END);
        fileSize = ftell(fp);

        WalPrint("fileSize is %d\n",fileSize);
        fseek(fp, 0, SEEK_SET);
        
        //accomodate extra bytes for new field "oldFirmwareVersion"
        cfg_file_content = (char *) malloc(sizeof(char) * (fileSize + 1 + sizeof(str)));
        fread(cfg_file_content, 1, fileSize,fp);
        WalPrint("file content:  %s\n",cfg_file_content);

	// If "oldFirmwareVersion" already exists in config file update it else add it
	cfg_oldFirmwareVer = strstr(cfg_file_content, WEBPA_CFG_FIRMWARE_VER);
	if(cfg_oldFirmwareVer != NULL)
	{
		snprintf(str, sizeof(str), "%s\": \"%s\"\n}",WEBPA_CFG_FIRMWARE_VER, value); 
        	WalPrint("Ater formatting: str:%s\n", str);
        	strcpy(cfg_oldFirmwareVer, str);
	}
	else
	{
		token = strstr(cfg_file_content , "}");
		token = token - 1;
		token[0] = ',';
		snprintf(str, sizeof(str), "\n\t\"%s\": \"%s\"\n}",WEBPA_CFG_FIRMWARE_VER, value); 
		WalPrint("Ater formatting: str:%s\n", str);
		strcpy(++token, str);
	}
        WalPrint("%s\n",cfg_file_content);
        fclose(fp);
        
	WalPrint("opening WEBPA_CFG_FILE for writing the content\n");
        fp = fopen(WEBPA_CFG_FILE, "w");
        if(fp == NULL)
	{
		WalPrint("Cannot open %s in write mode\n",WEBPA_CFG_FILE);
		return WAL_FAILURE;
	}
        if(ferror(fp))
	{
		WalError("Error while writing webpa config file.\n");
		fclose(fp);
		return WAL_FAILURE;
	}	
		
        fprintf(fp, "%s",cfg_file_content);
        WalPrint("After writing cfg_file_content to config\n");
        fclose(fp);
	WAL_FREE(cfg_file_content);
        WalPrint("End of addFirmwareVerToConfig\n");
        return WAL_SUCCESS; 
}

/*
 * @brief To send notification during Firmware Upgrade
 * GET "oldFirmwareVersion" from the /nvram/webpa_cfg.json file and compare 
 * with the current device firmware version. If they dont match then update "oldFirmwareVersion" 
 * with the latest version, update CMC and send notification to XPC indicating Firmware Upgrade
 */
static void sendNotificationForFirmwareUpgrade()
{
	WAL_STATUS configUpdateStatus = WAL_FAILURE;
	WAL_STATUS setCmcStatus = WAL_FAILURE;
	char *dbCMC = NULL, *dbCID = NULL;
	char newCMC[32]={'\0'};
	char *cur_firmware_ver = NULL;

	cur_firmware_ver = getSyncParam(SYNC_PARAM_FIRMWARE_VERSION);

	if(cur_firmware_ver == NULL)
	{
		WalError("Could not GET the current device Firmware version\n");
		return;
	}
	WalPrint("cur_firmware_ver: %s\n", cur_firmware_ver);

	if(strlen(webPaCfg.oldFirmwareVersion) == 0 || 
		(strcmp(webPaCfg.oldFirmwareVersion,cur_firmware_ver) != 0))
	{
		WalInfo("oldFirmwareVer :%s, cur_firmware_ver value :%s\n", webPaCfg.oldFirmwareVersion, cur_firmware_ver);
		configUpdateStatus = addOrUpdateFirmwareVerToConfigFile(cur_firmware_ver);
		if(configUpdateStatus == WAL_SUCCESS)
    		{
    			WalInfo("Added/Updated Firmware version details to WebPA config file\n");
    		}
    		else
    		{
    			WalError("Error in adding/updating Firmware details to WebPa config file\n");
    		}
		// Update config structure with the current device firmware version
		walStrncpy(webPaCfg.oldFirmwareVersion,cur_firmware_ver,sizeof(webPaCfg.oldFirmwareVersion));

		// Send notification to server for firmware upgrade		
		dbCMC = getSyncParam(SYNC_PARAM_CMC); 
		dbCID = getSyncParam(SYNC_PARAM_CID);
	
		if(dbCMC != NULL && dbCID !=NULL)
		{
			snprintf(newCMC, sizeof(newCMC), "%d", (atoi(dbCMC) | CHANGED_BY_FIRMWARE_UPGRADE));
			WalPrint("newCMC value after firmware upgrade: %s\n", newCMC);

			if(atoi(dbCMC) != atoi(newCMC))
			{		
				setCmcStatus = setSyncParam(SYNC_PARAM_CMC, newCMC);
				if(setCmcStatus == WAL_SUCCESS)
				{
					WalInfo("Successfully Set CMC to %d\n", atoi(newCMC));
					sendParamNotifyToServer(WEBPA_NOTIFY_XPC, atoi(newCMC), dbCID);
				}
				else
				{
					WAL_FREE(dbCID);
					WalError("Error setting CMC value for firmware upgrade\n");
				}
			}
			else
			{
				WalInfo("newCMC %s is same as dbCMC %s, hence firmware upgrade notification was not sent\n",newCMC,dbCMC);
				WAL_FREE(dbCID);
			}
			WAL_FREE(dbCMC);
		}
	}
	else if(strcmp(webPaCfg.oldFirmwareVersion,cur_firmware_ver) == 0)
	{
		WalInfo("Current device firmware version %s is same as old firmware version in config file %s\n",cur_firmware_ver,webPaCfg.oldFirmwareVersion);
	}
	WAL_FREE(cur_firmware_ver);	
}
