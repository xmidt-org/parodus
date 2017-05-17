/**
 * @file nopoll_handlers.h
 *
 * @description This header defines nopoll handler functions.
 *
 * Copyright (c) 2015  Comcast
 */

#ifndef _NOPOLL_HANDLERS_H_
#define _NOPOLL_HANDLERS_H_

#include "nopoll.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

extern pthread_mutex_t g_mutex;
extern pthread_cond_t g_cond; 
extern pthread_mutex_t close_mut;
extern volatile unsigned int heartBeatTimer;
extern bool close_retry;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/**
 * @brief listenerOnMessage_queue function to add messages to the queue
 *
 * @param[in] ctx The context where the connection happens.
 * @param[in] conn The Websocket connection object
 * @param[in] msg The message received from server for various process requests
 * @param[out] user_data data which is to be sent
 */

void listenerOnrequest_queue(void *requestMsg,int reqSize);

/**
 * @brief listenerOnPingMessage function to create WebSocket listener to receive heartbeat ping messages
 *
 * @param[in] ctx The context where the connection happens.
 * @param[in] conn Websocket connection object
 * @param[in] msg The ping message received from the server
 * @param[out] user_data data which is to be sent
 */
void listenerOnPingMessage (noPollCtx * ctx, noPollConn * conn, noPollMsg * msg, noPollPtr user_data);


void listenerOnCloseMessage (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data);

#ifdef __cplusplus
}
#endif
    
#endif
