/**
 * @file internal.h
 *
 * @description This file is used to manage internal functions of parodus
 *
 * Copyright (c) 2015  Comcast
 */
#ifndef _CLIENTLIST_H_
#define _CLIENTLIST_H_

#include <stdio.h>

#include "ParodusInternal.h"
#include <wrp-c.h>

#include "wss_mgr.h"
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
extern int numOfClients;

reg_list_item_t * get_global_node(void);

#ifdef __cplusplus
}
#endif
    

#endif

