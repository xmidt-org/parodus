/**
 * @file connection.h
 *
 * @description This header defines functions required to manage WebSocket client connections.
 *
 * Copyright (c) 2015  Comcast
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
extern bool conn_retry;
extern volatile unsigned int heartBeatTimer;
extern pthread_mutex_t close_mut;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/**
 * @brief Interface to terminate WebSocket client connections and clean up resources.
 */

char *get_global_reconnect_reason();
void set_global_reconnect_reason(char *reason);
void createLWSconnection();
struct lws_context *get_global_context(void);
void set_global_context(struct lws_context *contextRef);

#ifdef __cplusplus
}
#endif
    
#endif
