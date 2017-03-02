/**
 * @file wss_mgr.h
 *
 * @description This header defines functions required to manage WebSocket client connections.
 *
 * Copyright (c) 2015  Comcast
 */
 
#ifndef WEBSOCKET_MGR_H_
#define WEBSOCKET_MGR_H_
#include <nopoll.h>
#include "config.h"
#ifdef __cplusplus
extern "C" {
#endif



#define HTTP_CUSTOM_HEADER_COUNT                    	4

/**
 * @brief Interface to create WebSocket client connections.
 * Loads the WebPA config file, if not provided by the caller,
 *  and creates the intial connection and manages the connection wait, close mechanisms.
 */
void createSocketConnection(void *config, void (* initKeypress)());

void parseCommandLine(int argc,char **argv,ParodusCfg * cfg);

#define UNUSED(x) (void)(x)


#ifdef __cplusplus
}
#endif


#endif /* WEBSOCKET_MGR_H_ */

