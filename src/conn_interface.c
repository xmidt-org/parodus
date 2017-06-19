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

bool LastReasonStatus = false;
volatile unsigned int heartBeatTimer = 0;
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

static bool __registerWithSeshat(void);
static void *heartBeatHandlerTask();
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
    StartThread(heartBeatHandlerTask);
    StartThread(serviceAliveTask);

    if (NULL != initKeypress) 
    {
        (* initKeypress) ();
    }

    seshat_registered = __registerWithSeshat();
    UNUSED(seshat_registered);
    
    do
    {
        lws_service(get_global_context(), 50);
        if(conn_retry)
        {
            wsi_dumb = NULL;
            lws_context_destroy(get_global_context());
            ParodusInfo("conn_retry is %d, hence closing the connection and retrying\n", conn_retry);
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
            if( (NULL != discover_url) && (0 == strcmp(parodus_url, discover_url)) ) {
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

/*
 * @brief To handle heartbeat mechanism
 */
static void *heartBeatHandlerTask()
{
    while (FOREVER())
    {
        sleep(HEARTBEAT_RETRY_SEC);
        if(heartBeatTimer >= get_parodus_cfg()->webpa_ping_timeout)
        {
            if(!conn_retry)
            {
                ParodusError("ping wait time > %d. Terminating the connection with WebPA server and retrying\n", get_parodus_cfg()->webpa_ping_timeout);
                ParodusInfo("Reconnect detected, setting Ping_Miss reason for Reconnect\n");
                set_global_reconnect_reason("Ping_Miss");
                LastReasonStatus = true;
                conn_retry = true;
            }
            else
            {
                ParodusPrint("heartBeatHandler - conn_retry set to %d, hence resetting the heartBeatTimer\n",conn_retry);
            }
            heartBeatTimer = 0;
        }
        else
        {
            ParodusPrint("heartBeatTimer %d\n",heartBeatTimer);
            heartBeatTimer += HEARTBEAT_RETRY_SEC;
        }
    }
    heartBeatTimer = 0;
    return NULL;
}

