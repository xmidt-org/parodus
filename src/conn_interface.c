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
#include "crud_interface.h"
#include "heartBeat.h"
#ifdef FEATURE_DNS_QUERY
#include <ucresolv_log.h>
#endif
#include <time.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define HEARTBEAT_RETRY_SEC                         	30      /* Heartbeat (ping/pong) timeout in seconds */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

bool close_retry = false;
pthread_mutex_t close_mut=PTHREAD_MUTEX_INITIALIZER;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
void timespec_diff(struct timespec *start, struct timespec *stop,
                   struct timespec *result);
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

void createSocketConnection(void (* initKeypress)())
{
    //ParodusCfg *tmpCfg = (ParodusCfg*)config_in;
    noPollCtx *ctx;
    bool seshat_registered = false;
    unsigned int webpa_ping_timeout_ms = 1000 * get_parodus_cfg()->webpa_ping_timeout;
    unsigned int heartBeatTimer = 0;
    
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
	StartThread(CRUDHandlerTask);

    if (NULL != initKeypress) 
    {
        (* initKeypress) ();
    }

    seshat_registered = __registerWithSeshat();
    
    do
    {
        struct timespec start, stop, diff;
        int time_taken_ms;

        clock_gettime(CLOCK_REALTIME, &start);
        nopoll_loop_wait(ctx, 5000000);
        clock_gettime(CLOCK_REALTIME, &stop);

        timespec_diff(&start, &stop, &diff);
        time_taken_ms = diff.tv_sec * 1000 + (diff.tv_nsec / 1000000);

        // ParodusInfo("nopoll_loop_wait() time %d msec\n", time_taken_ms);
	heartBeatTimer = get_heartBeatTimer();
        if(heartBeatTimer >= webpa_ping_timeout_ms)
        {
            ParodusInfo("heartBeatTimer %d webpa_ping_timeout_ms %d\n", heartBeatTimer, webpa_ping_timeout_ms);
            if(!close_retry) 
            {
                ParodusError("ping wait time > %d . Terminating the connection with WebPA server and retrying\n", webpa_ping_timeout_ms / 1000);
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
            reset_heartBeatTimer();
        }
        else {
            increment_heartBeatTimer(time_taken_ms);
        }

        if( false == seshat_registered ) {
            seshat_registered = __registerWithSeshat();
        }

        if(close_retry)
        {
            ParodusInfo("close_retry is %d, hence closing the connection and retrying\n", close_retry);
            close_and_unref_connection(get_global_conn());
            set_global_conn(NULL);

            get_parodus_cfg()->cloud_status = "offline";
            ParodusInfo("cloud_status set as %s after connection close\n", get_parodus_cfg()->cloud_status);
            if(get_parodus_cfg()->cloud_disconnect !=NULL)
            {
				ParodusPrint("get_parodus_cfg()->cloud_disconnect is %s\n", get_parodus_cfg()->cloud_disconnect);
				set_cloud_disconnect_time(5);
				ParodusInfo("Waiting for %d minutes for reconnecting .. \n", get_cloud_disconnect_time());

				sleep( get_cloud_disconnect_time() * 60 );
				ParodusInfo("get_parodus_cfg()->cloud_disconnect reset after %d minutes\n", get_cloud_disconnect_time());
				free(get_parodus_cfg()->cloud_disconnect);
				get_parodus_cfg()->cloud_disconnect = NULL;
            }
            createNopollConnection(ctx);
        }
       } while(!close_retry);

    close_and_unref_connection(get_global_conn());
    nopoll_ctx_unref(ctx);
    nopoll_cleanup_library();
}

