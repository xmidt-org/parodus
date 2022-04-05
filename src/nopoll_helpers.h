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
 * @file nopoll_handlers.h
 *
 * @description This header defines functions to manage incomming and outgoing messages.
 *
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
int sendMessage(noPollConn *conn, void *msg, size_t len);

/**
 * @brief __report_log Nopoll log handler 
 * Nopoll log handler for integrating nopoll logs 
 */
void __report_log (noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data);

#ifdef __cplusplus
}
#endif
    
#endif
