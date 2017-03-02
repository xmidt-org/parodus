/**
 * @file client_list.h
 *
 * @description This file is used to manage registered clients
 *
 * Copyright (c) 2015  Comcast
 */
#ifndef _CLIENTLIST_H_
#define _CLIENTLIST_H_

#include <stdio.h>
#include <wrp-c.h>

#include "ParodusInternal.h"
#include "parodus_log.h"

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif
  

int addToList( wrp_msg_t **msg);

int sendAuthStatus(reg_list_item_t *new_node);

int deleteFromList(char* service_name);

reg_list_item_t * get_global_node(void);

void *serviceAliveTask();

#ifdef __cplusplus
}
#endif
    

#endif

