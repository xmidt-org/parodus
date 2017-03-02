/**
 * @file downstream.h
 *
 * @description This header defines functions required to manage downstream messages.
 *
 * Copyright (c) 2015  Comcast
 */
 
#ifndef _DOWNSTREAM_H_
#define _DOWNSTREAM_H_

#include <nopoll.h>
#include "config.h"
#ifdef __cplusplus
extern "C" {
#endif

void *messageHandlerTask();
/**
 * @brief listenerOnMessage function to create WebSocket listener to receive connections
 *
 * @param[in] ctx The context where the connection happens.
 * @param[in] conn The Websocket connection object
 * @param[in] msg The message received from server for various process requests
 * @param[out] user_data data which is to be sent
 */
void listenerOnMessage(void * msg, size_t msgSize, int *numOfClients, reg_list_item_t **head );

#ifdef __cplusplus
}
#endif


#endif /* WEBSOCKET_MGR_H_ */

