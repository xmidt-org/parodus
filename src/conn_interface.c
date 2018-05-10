/**
 * Copyright 2015 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
/**
 * @file conn_interface.c
 *
 * @description This decribes interface to create WebSocket client connections.
 *
 */
 
#include "connection.h"
#include "conn_interface.h"
#include "ParodusInternal.h"
#include "config.h"
#include "upstream.h"
#include "downstream.h"
#include "thread_tasks.h"
#include "nopoll_helpers.h"
#include "mutex.h"
#include "spin_thread.h"
#include "service_alive.h"
#include "seshat_interface.h"
#ifdef FEATURE_DNS_QUERY
#include <ucresolv_log.h>
#endif
#include "parodus_interface.h"
#include "peer2peer.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define HEARTBEAT_RETRY_SEC                         	30      /* Heartbeat (ping/pong) timeout in seconds */
#define HUB_STR                                         "hub"
#define SPK1_STR                                        "spk1"
#define SPK2_STR                                        "spk2"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

bool close_retry = false;
volatile unsigned int heartBeatTimer = 0;
pthread_mutex_t close_mut=PTHREAD_MUTEX_INITIALIZER;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

void createSocketConnection(void (* initKeypress)())
{
    int intTimer=0;	
    //ParodusCfg *tmpCfg = (ParodusCfg*)config_in;
    noPollCtx *ctx;
    bool seshat_registered = false;
    
    //loadParodusCfg(tmpCfg,get_parodus_cfg());
#ifdef FEATURE_DNS_QUERY
	register_ucresolv_logger (__cimplog);
#endif
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

    seshat_registered = __registerWithSeshat();

    if( 0 == strncmp(HUB_STR, get_parodus_cfg()->hub_or_spk, sizeof(HUB_STR)) ) {
        hub_setup_listener(HUB_URL);
    } else if( 0 == strncmp(SPK1_STR, get_parodus_cfg()->hub_or_spk, sizeof(SPK1_STR)) ) {
        spoke_setup_listener(SPK1_URL);
    } else if( 0 == strncmp(SPK2_STR, get_parodus_cfg()->hub_or_spk, sizeof(SPK2_STR)) ) {
        spoke_setup_listener(SPK2_URL);
    }
    StartThread(handle_and_process_P2P_IncomingMessage);
    StartThread(process_P2P_OutgoingMessage);
    
    do
    {
        nopoll_loop_wait(ctx, 5000000);
		//Add check for spoke, so that parodus won't reconect everytime for ping miss
        if (true == connectToXmidt)
		{
			intTimer = intTimer + 5;
		    if(heartBeatTimer >= get_parodus_cfg()->webpa_ping_timeout)
		    {
		        if(!close_retry)
		        {
		            ParodusError("ping wait time > %d. Terminating the connection with WebPA server and retrying\n", get_parodus_cfg()->webpa_ping_timeout);
		            ParodusInfo("Reconnect detected, setting Ping_Miss reason for Reconnect\n");
		            set_global_reconnect_reason("Ping_Miss");
		            set_global_reconnect_status(true);
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
		}
        if( false == seshat_registered ) {
            seshat_registered = __registerWithSeshat();
        }

        if(close_retry)
        {
            ParodusInfo("close_retry is %d, hence closing the connection and retrying\n", close_retry);
            close_and_unref_connection(get_global_conn());
            set_global_conn(NULL);
            createNopollConnection(ctx);
        }		
    } while(!close_retry);

    if( 0 == strncmp(HUB_STR, get_parodus_cfg()->hub_or_spk, sizeof(HUB_STR)) ) {
        hub_cleanup_listener();
    } else if( 0 == strncmp(SPK1_STR, get_parodus_cfg()->hub_or_spk, sizeof(SPK1_STR)) ) {
        spoke_cleanup_listener(); 
    } else if( 0 == strncmp(SPK2_STR, get_parodus_cfg()->hub_or_spk, sizeof(SPK2_STR)) ) {
        spoke_cleanup_listener();
    }

    close_and_unref_connection(get_global_conn());
    nopoll_ctx_unref(ctx);
    nopoll_cleanup_library();
}

