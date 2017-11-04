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
 * @file client_list.h
 *
 * @description This file is used to manage registered clients
 *
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

