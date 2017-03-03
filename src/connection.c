/**
 * @file connection.c
 *
 * @description This decribes functions required to manage WebSocket client connections.
 *
 * Copyright (c) 2015  Comcast
 */
 
#include "connection.h"
#include "ParodusInternal.h"
#include "time.h"
#include "config.h"
#include "upstream.h"
#include "downstream.h"
#include "nopoll_helpers.h"
#include "mutex.h"
#include "spin_thread.h"
#include "service_alive.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define HTTP_CUSTOM_HEADER_COUNT                    	4
#define HEARTBEAT_RETRY_SEC                         	30      /* Heartbeat (ping/pong) timeout in seconds */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

char deviceMAC[32]={'\0'};
bool close_retry = false;
bool LastReasonStatus = false;
char *reconnect_reason = "webpa_process_starts";
volatile unsigned int heartBeatTimer = 0;
static noPollConn *g_conn = NULL;
pthread_mutex_t close_mut=PTHREAD_MUTEX_INITIALIZER;

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

noPollConn *get_global_conn(void)
{
    return g_conn;
}

void set_global_conn(noPollConn *conn)
{
    g_conn = conn;
}

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
    packMetaData();
    setMessageHandlers();
    getParodusUrl();
    UpStreamMsgQ = NULL;
    StartThread(handle_upstream);
    StartThread(processUpstreamMessage);
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
                ParodusError("ping wait time > %d. Terminating the connection with WebPA server and retrying\n", get_parodus_cfg()->webpa_ping_timeout);
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
 * @brief createNopollConnection interface to create WebSocket client connections.
 *Loads the WebPA config file and creates the intial connection and manages the connection wait, close mechanisms.
 */
int createNopollConnection(noPollCtx *ctx)
{
    bool initial_retry = false;
    int backoffRetryTime = 0;
    int max_retry_sleep;
    char device_id[32]={'\0'};
    char user_agent[512]={'\0'};
    const char * headerNames[HTTP_CUSTOM_HEADER_COUNT] = {"X-WebPA-Device-Name","X-WebPA-Device-Protocols","User-Agent", "X-WebPA-Convey"};
    const char *headerValues[HTTP_CUSTOM_HEADER_COUNT];
    int headerCount = HTTP_CUSTOM_HEADER_COUNT; /* Invalid X-Webpa-Convey header Bug # WEBPA-787 */
    char port[8];
    noPollConnOpts * opts;
    char server_Address[256];
    char redirectURL[128]={'\0'};
    char *temp_ptr, *conveyHeader;
    int connErr=0;
    struct timespec connErr_start,connErr_end,*connErr_startPtr,*connErr_endPtr;
    connErr_startPtr = &connErr_start;
    connErr_endPtr = &connErr_end;
    //Retry Backoff count shall start at c=2 & calculate 2^c - 1.
    int c=2;
    
    if(ctx == NULL)
        return nopoll_false;

	FILE *fp;
	fp = fopen("/tmp/parodus_ready", "r");

	if (fp!=NULL)
	{
		unlink("/tmp/parodus_ready");
		ParodusPrint("Closing Parodus_Ready FIle \n");
		fclose(fp);
	}

	parStrncpy(deviceMAC, get_parodus_cfg()->hw_mac,sizeof(deviceMAC));
	snprintf(device_id, sizeof(device_id), "mac:%s", deviceMAC);
	ParodusInfo("Device_id %s\n",device_id);

	headerValues[0] = device_id;
	headerValues[1] = "wrp-0.11,getset-0.1";    
	
	ParodusPrint("BootTime In sec: %d\n", get_parodus_cfg()->boot_time);
	ParodusInfo("Received reconnect_reason as:%s\n", reconnect_reason);
	snprintf(user_agent, sizeof(user_agent),
         "%s (%s; %s/%s;)",
         ((0 != strlen(get_parodus_cfg()->webpa_protocol)) ? get_parodus_cfg()->webpa_protocol : "unknown"),
         ((0 != strlen(get_parodus_cfg()->fw_name)) ? get_parodus_cfg()->fw_name : "unknown"),
         ((0 != strlen(get_parodus_cfg()->hw_model)) ? get_parodus_cfg()->hw_model : "unknown"),
         ((0 != strlen(get_parodus_cfg()->hw_manufacturer)) ? get_parodus_cfg()->hw_manufacturer : "unknown"));

	ParodusInfo("User-Agent: %s\n",user_agent);
	headerValues[2] = user_agent;
	conveyHeader = getWebpaConveyHeader();
	if(strlen(conveyHeader) > 0)
	{
        headerValues[3] = conveyHeader;
	}
	else
	{
	    headerValues[3] = ""; 
        headerCount -= 1;
	}
	snprintf(port,sizeof(port),"%d",8080);
	parStrncpy(server_Address, get_parodus_cfg()->webpa_url, sizeof(server_Address));
	ParodusInfo("server_Address %s\n",server_Address);
					
	max_retry_sleep = (int) pow(2, get_parodus_cfg()->webpa_backoff_max) -1;
	ParodusPrint("max_retry_sleep is %d\n", max_retry_sleep );
	
	do
	{
		//calculate backoffRetryTime and to perform exponential increment during retry
		if(backoffRetryTime < max_retry_sleep)
		{
			backoffRetryTime = (int) pow(2, c) -1;
		}
		ParodusPrint("New backoffRetryTime value calculated as %d seconds\n", backoffRetryTime);
								
        noPollConn *connection;
		if(get_parodus_cfg()->secureFlag) 
		{                    
		    ParodusPrint("secure true\n");
			/* disable verification */
			opts = nopoll_conn_opts_new ();
			nopoll_conn_opts_ssl_peer_verify (opts, nopoll_false);
			nopoll_conn_opts_set_ssl_protocol (opts, NOPOLL_METHOD_TLSV1_2); 
			connection = nopoll_conn_tls_new(ctx, opts, server_Address, port, NULL,
                               get_parodus_cfg()->webpa_path_url, NULL, NULL, get_parodus_cfg()->webpa_interface_used,
                                headerNames, headerValues, headerCount);// WEBPA-787
		}
		else 
		{
		    ParodusPrint("secure false\n");
            connection = nopoll_conn_new(ctx, server_Address, port, NULL,
                       get_parodus_cfg()->webpa_path_url, NULL, NULL, get_parodus_cfg()->webpa_interface_used,
                        headerNames, headerValues, headerCount);// WEBPA-787
		}
        set_global_conn(connection);

		if(get_global_conn() != NULL)
		{
			if(!nopoll_conn_is_ok(get_global_conn())) 
			{
				ParodusError("Error connecting to server\n");
				ParodusError("RDK-10037 - WebPA Connection Lost\n");
				// Copy the server address from config to avoid retrying to the same failing talaria redirected node
				parStrncpy(server_Address, get_parodus_cfg()->webpa_url, sizeof(server_Address));
				close_and_unref_connection(get_global_conn());
				set_global_conn(NULL);
				initial_retry = true;
				
				ParodusInfo("Waiting with backoffRetryTime %d seconds\n", backoffRetryTime);
				sleep(backoffRetryTime);
				continue;
			}
			else 
			{
				ParodusPrint("Connected to Server but not yet ready\n");
				initial_retry = false;
				//reset backoffRetryTime back to the starting value, as next reason can be different					
				c = 2;
				backoffRetryTime = (int) pow(2, c) -1;
			}

			if(!nopoll_conn_wait_until_connection_ready(get_global_conn(), 10, redirectURL)) 
			{
				
				if (strncmp(redirectURL, "Redirect:", 9) == 0) // only when there is a http redirect
				{
					ParodusError("Received temporary redirection response message %s\n", redirectURL);
					// Extract server Address and port from the redirectURL
					temp_ptr = strtok(redirectURL , ":"); //skip Redirect 
					temp_ptr = strtok(NULL , ":"); // skip https
					temp_ptr = strtok(NULL , ":");
					parStrncpy(server_Address, temp_ptr+2, sizeof(server_Address));
					parStrncpy(port, strtok(NULL , "/"), sizeof(port));
					ParodusInfo("Trying to Connect to new Redirected server : %s with port : %s\n", server_Address, port);
					//reset c=2 to start backoffRetryTime as retrying using new redirect server
					c = 2;
				}
				else
				{
					ParodusError("Client connection timeout\n");	
					ParodusError("RDK-10037 - WebPA Connection Lost\n");
					// Copy the server address from config to avoid retrying to the same failing talaria redirected node
					parStrncpy(server_Address, get_parodus_cfg()->webpa_url, sizeof(server_Address));
					ParodusInfo("Waiting with backoffRetryTime %d seconds\n", backoffRetryTime);
					sleep(backoffRetryTime);
					c++;
				}
				close_and_unref_connection(get_global_conn());
				set_global_conn(NULL);
				initial_retry = true;
				
			}
			else 
			{
				initial_retry = false;				
				ParodusInfo("Connection is ready\n");
			}
		}
		else
		{
			
			/* If the connect error is due to DNS resolving to 10.0.0.1 then start timer.
			 * Timeout after 15 minutes if the error repeats continuously and kill itself. 
			 */
			if((checkHostIp(server_Address) == -2)) 	
			{
				if(connErr == 0)
				{
					getCurrentTime(connErr_startPtr);
					connErr = 1;
					ParodusInfo("First connect error occurred, initialized the connect error timer\n");
				}
				else
				{
					getCurrentTime(connErr_endPtr);
					ParodusPrint("checking timeout difference:%ld\n", timeValDiff(connErr_startPtr, connErr_endPtr));
					if(timeValDiff(connErr_startPtr, connErr_endPtr) >= (15*60*1000))
					{
						ParodusError("WebPA unable to connect due to DNS resolving to 10.0.0.1 for over 15 minutes; crashing service.\n");
						reconnect_reason = "Dns_Res_webpa_reconnect";
						LastReasonStatus = true;
						
						kill(getpid(),SIGTERM);						
					}
				}			
			}
			initial_retry = true;
			ParodusInfo("Waiting with backoffRetryTime %d seconds\n", backoffRetryTime);
			sleep(backoffRetryTime);
			c++;
			// Copy the server address from config to avoid retrying to the same failing talaria redirected node
			parStrncpy(server_Address, get_parodus_cfg()->webpa_url, sizeof(server_Address));
		}
				
	}while(initial_retry);
	
	if(get_parodus_cfg()->secureFlag) 
	{
		ParodusInfo("Connected to server over SSL\n");
	}
	else 
	{
		ParodusInfo("Connected to server\n");
	}
	
	//creating tmp file to signal parodus_ready status once connection is successful
	fp = fopen("/tmp/parodus_ready", "w");
	fflush(fp);
	fclose(fp);
	
	// Reset close_retry flag and heartbeatTimer once the connection retry is successful
	ParodusPrint("createNopollConnection(): close_mut lock\n");
	pthread_mutex_lock (&close_mut);
	close_retry = false;
	pthread_mutex_unlock (&close_mut);
	ParodusPrint("createNopollConnection(): close_mut unlock\n");
	heartBeatTimer = 0;
	// Reset connErr flag on successful connection
	connErr = 0;
	reconnect_reason = "webpa_process_starts";
	LastReasonStatus =false;
	ParodusPrint("LastReasonStatus reset after successful connection\n");
	
	return nopoll_true;
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

