/**
 * @file conn_interface.h
 *
 * @description This header defines interface to create WebSocket client connections.
 *
 * Copyright (c) 2015  Comcast
 */
 
#ifndef _CONN_INTERFACE_H_
#define _CONN_INTERFACE_H_

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
void createLWSsocket(void *config_in, void (* initKeypress)());
   
#ifdef __cplusplus
}
#endif
    
#endif
