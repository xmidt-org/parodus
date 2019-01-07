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
 * @file crud_interface.h
 *
 * @description This header defines functions required to manage CRUD messages.
 *
 */
 
#ifndef _CRUD_INTERFACE_H_
#define _CRUD_INTERFACE_H_

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif


/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct CrudMsg__
{
	wrp_msg_t *msg;
	struct CrudMsg__ *next;
} CrudMsg;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

void addCRUDresponseToUpstreamQ(void *response_bytes, ssize_t response_size);
pthread_cond_t *get_global_crud_con(void);
pthread_mutex_t *get_global_crud_mut(void);

#ifdef __cplusplus
}
#endif


#endif /* _CRUD_INTERFACE_H_ */

