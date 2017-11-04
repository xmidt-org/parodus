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
 * @file connection.h
 *
 * @description This header defines functions required to manage WebSocket client connections.
 *
 */
 
#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "ParodusInternal.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

extern bool close_retry;
extern volatile unsigned int heartBeatTimer;
extern pthread_mutex_t close_mut;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

int createNopollConnection(noPollCtx *);

/**
 * @brief Interface to terminate WebSocket client connections and clean up resources.
 */
void close_and_unref_connection(noPollConn *);

noPollConn *get_global_conn(void);
void set_global_conn(noPollConn *);

char *get_global_reconnect_reason();
void set_global_reconnect_reason(char *reason);

#ifdef __cplusplus
}
#endif
    
#endif
