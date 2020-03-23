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
#define SHUTDOWN_REASON_PARODUS_STOP    "parodus_stopping"
#define SHUTDOWN_REASON_SYSTEM_RESTART  "system_restarting"
#define SHUTDOWN_REASON_SIGTERM 		"SIGTERM"

/**
* parodusOnPingStatusChangeHandler - Function pointer
* Used to define callback function to do additional processing 
* when websocket Ping status change event 
* i.e. ping_miss or ping receive after miss
*/
typedef void (*parodusOnPingStatusChangeHandler) (char * status);
parodusOnPingStatusChangeHandler on_ping_status_change;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

void set_server_list_null (server_list_t *server_list);
int createNopollConnection(noPollCtx *, server_list_t *);

/**
 * @brief Interface to terminate WebSocket client connections and clean up resources.
 */
void close_and_unref_connection(noPollConn *);

noPollConn *get_global_conn(void);
void set_global_conn(noPollConn *);

char *get_global_shutdown_reason();
void set_global_shutdown_reason(char *reason);

char *get_global_reconnect_reason();
void set_global_reconnect_reason(char *reason);

bool get_global_reconnect_status();
void set_global_reconnect_status(bool status);

int get_cloud_disconnect_time();
void set_cloud_disconnect_time(int disconnTime);

/**
 * @brief Interface to self heal connection in progress getting stuck
 */
void start_conn_in_progress (unsigned long start_time, bool redir_handshake_err);
void stop_conn_in_progress (void);

// To Register parodusOnPingStatusChangeHandler Callback function
void registerParodusOnPingStatusChangeHandler(parodusOnPingStatusChangeHandler on_ping_status_change);

// To stop connection and wait duing interface down state
int wait_while_interface_down (void);

void terminate_backoff_delay (void);

#ifdef __cplusplus
}
#endif
    
#endif
