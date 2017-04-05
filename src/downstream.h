/**
 * @file downstream.h
 *
 * @description This header defines functions required to manage downstream messages.
 *
 * Copyright (c) 2015  Comcast
 */
 
#ifndef _DOWNSTREAM_H_
#define _DOWNSTREAM_H_

#include "wrp-c.h"
#include "client_list.h"


#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/**
 * @brief listenerOnMessage function to create WebSocket listener to receive connections
 *
 * @param[in] msg The message received from server for various process requests
 * @param[in] msgSize message size
 */
void listenerOnMessage(void * msg, size_t msgSize);

#ifdef __cplusplus
}
#endif


#endif /* WEBSOCKET_MGR_H_ */

