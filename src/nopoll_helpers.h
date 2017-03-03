/**
 * @file nopoll_handlers.h
 *
 * @description This header defines functions to manage incomming and outgoing messages.
 *
 * Copyright (c) 2015  Comcast
 */

#ifndef _NOPOLL_HELPERS_H_
#define _NOPOLL_HELPERS_H_

#include "nopoll.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/**
 * @brief Interface to create WebSocket client connections.
 * Loads the WebPA config file, if not provided by the caller,
 *  and creates the intial connection and manages the connection wait, close mechanisms.
 */
int sendResponse(noPollConn * conn,void *str, size_t bufferSize);
void setMessageHandlers();
void sendMessage(noPollConn *conn, void *msg, size_t len);

/**
 * @brief __report_log Nopoll log handler 
 * Nopoll log handler for integrating nopoll logs 
 */
void __report_log (noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data);

#ifdef __cplusplus
}
#endif
    
#endif
