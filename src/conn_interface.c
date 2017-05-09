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
#include <libseshat.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define HEARTBEAT_RETRY_SEC                         	30      /* Heartbeat (ping/pong) timeout in seconds */
#define SESHAT_SERVICE_NAME                             "Parodus"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

bool close_retry = false;
bool LastReasonStatus = false;
volatile unsigned int heartBeatTimer = 0;
pthread_mutex_t close_mut=PTHREAD_MUTEX_INITIALIZER;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

static bool __registerWithSeshat(void);

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

void createSocketConnection(void *config_in, void (* initKeypress)())
{
    int intTimer=0;	
    ParodusCfg *tmpCfg = (ParodusCfg*)config_in;
    noPollCtx *ctx;
    bool seshat_registered = false;
    
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
    
    do
    {
        nopoll_loop_wait(ctx, 5000000);
        intTimer = intTimer + 5;

        if(heartBeatTimer >= get_parodus_cfg()->webpa_ping_timeout) 
        {
            if(!close_retry) 
            {
                ParodusError("ping wait time > %d. Terminating the connection with WebPA server and retrying\n", get_parodus_cfg()->webpa_ping_timeout);
                ParodusInfo("Reconnect detected, setting Ping_Miss reason for Reconnect\n");
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

        if( false == seshat_registered ) {
            __registerWithSeshat();
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

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/**
 * @brief Helper function to register with seshat.
 * 
 * @note return whether successfully registered.
 *
 * @return true when registered, false otherwise.
 */
static bool __registerWithSeshat()
{
    char *seshat_url = get_parodus_cfg()->seshat_url;
    char *parodus_url = get_parodus_cfg()->local_url;
    char *discover_url = NULL;
    bool rv = false;

    if( 0 == init_lib_seshat(seshat_url) ) {
        ParodusInfo("seshatlib initialized! (url %s)\n", seshat_url);

        if( 0 == seshat_register(SESHAT_SERVICE_NAME, parodus_url) ) {
            ParodusInfo("seshatlib registered! (url %s)\n", parodus_url);

            discover_url = seshat_discover(SESHAT_SERVICE_NAME);
            if( 0 == strcmp(parodus_url, discover_url) ) {
                ParodusInfo("seshatlib discovered url = %s\n", discover_url);
                rv = true;
            } else {
                ParodusError("seshatlib registration error (url %s)!", discover_url);
            }
            free(discover_url);
        } else {
            ParodusError("seshatlib not registered! (url %s)\n", parodus_url);
        }
    } else {
        ParodusError("seshatlib not initialized! (url %s)\n", seshat_url);
    }

    shutdown_seshat_lib();
    return rv;
}
