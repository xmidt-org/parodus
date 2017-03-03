/**
 * @file connection.h
 *
 * @description This header defines functions required to manage WebSocket client connections.
 *
 * Copyright (c) 2015  Comcast
 */
 
#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "nopoll.h"
#include "upstream.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

extern UpStreamMsg *UpStreamMsgQ;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/**
 * @brief Interface to create WebSocket client connections.
 * Loads the WebPA config file, if not provided by the caller,
 *  and creates the intial connection and manages the connection wait, close mechanisms.
 */
void createSocketConnection(void *config_in, void (* initKeypress)());
   
int createNopollConnection(noPollCtx *);

/**
 * @brief Interface to terminate WebSocket client connections and clean up resources.
 */
void close_and_unref_connection(noPollConn *);

noPollConn *get_global_conn(void);
void set_global_conn(noPollConn *);

#ifdef __cplusplus
}
#endif
    
#endif
