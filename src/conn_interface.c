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
#include <libwebsockets.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define HEARTBEAT_RETRY_SEC                         	30      /* Heartbeat (ping/pong) timeout in seconds */
#define SESHAT_SERVICE_NAME                             "Parodus"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

bool conn_retry = false;
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
void createLWSsocket(void *config_in, void (* initKeypress)())
{
    bool seshat_registered = false;
    ParodusCfg *tmpCfg = (ParodusCfg*)config_in;
    
    loadParodusCfg(tmpCfg,get_parodus_cfg());
    createLWSconnection();
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
        lws_service(get_global_context(), 500);
        if(conn_retry)
        {
            //ParodusInfo("close_retry is %d, hence closing the connection and retrying\n", close_retry);
            createLWSconnection();
        }
        
    }while(!conn_retry);
    
    lws_context_destroy(get_global_context());
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
        ParodusPrint("seshatlib not initialized! (url %s)\n", seshat_url);
    }

    shutdown_seshat_lib();
    return rv;
}
