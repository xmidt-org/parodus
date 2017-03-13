/**
 * @file client_list.h
 *
 * @description This file is used to manage registered clients
 *
 * Copyright (c) 2015  Comcast
 */
#ifndef _CLIENTLIST_H_
#define _CLIENTLIST_H_

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

typedef struct reg_list_item
{
	int sock;
	char service_name[32];
	char url[100];
	struct reg_list_item *next;
} reg_list_item_t;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif
  

int addToList( wrp_msg_t **msg);

int sendAuthStatus(reg_list_item_t *new_node);

int deleteFromList(char* service_name);
int get_numOfClients();

reg_list_item_t * get_global_node(void);

#ifdef __cplusplus
}
#endif
    

#endif

