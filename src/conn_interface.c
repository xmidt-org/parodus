/**
 * @file conn_interface.c
 *
 * @description This decribes interface to create WebSocket client connections.
 *
 * Copyright (c) 2015  Comcast
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

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define HEARTBEAT_RETRY_SEC                         	30      /* Heartbeat (ping/pong) timeout in seconds */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

bool close_retry = false;
bool LastReasonStatus = false;
volatile unsigned int heartBeatTimer = 0;
pthread_mutex_t close_mut=PTHREAD_MUTEX_INITIALIZER;

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
    packMetaData();
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
                set_global_reconnect_reason("Ping_Miss");
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
            close_and_unref_connection(get_global_conn());
            set_global_conn(NULL);
            createNopollConnection(ctx);
        }		
    } while(!close_retry);

    close_and_unref_connection(get_global_conn());
    nopoll_ctx_unref(ctx);
    nopoll_cleanup_library();
}

