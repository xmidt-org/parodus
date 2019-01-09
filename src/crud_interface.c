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
 * @file crud_interface.c
 *
 * @description This file is used to manage incoming and outgoing CRUD messages.
 *
 */

#include "ParodusInternal.h"
#include "crud_tasks.h"
#include "crud_interface.h"
#include "upstream.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

pthread_mutex_t crud_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t crud_con=PTHREAD_COND_INITIALIZER;


/*----------------------------------------------------------------------------*/
/*                             Internal variables                             */
/*----------------------------------------------------------------------------*/

CrudMsg *crudMsgQ = NULL;

/*----------------------------------------------------------------------------*/
/*                             External functions                             */
/*----------------------------------------------------------------------------*/

pthread_cond_t *get_global_crud_con(void)
{
    return &crud_con;
}

pthread_mutex_t *get_global_crud_mut(void)
{
    return &crud_mut;
}

void addCRUDmsgToQueue(wrp_msg_t *crudMsg)
{
	CrudMsg * crudMessage;
	crudMessage = (CrudMsg *)malloc(sizeof(CrudMsg));
	if(crudMessage && crudMsg!=NULL)
	{
		crudMessage->msg = crudMsg;
		crudMessage->next = NULL;
		ParodusPrint("Inside addCRUDmsgToQueue : mutex lock in producer \n");
		pthread_mutex_lock(&crud_mut);
		if(crudMsgQ ==NULL)
		{
			crudMsgQ = crudMessage;
			ParodusPrint("Producer added message\n");
			pthread_cond_signal(&crud_con);
			pthread_mutex_unlock(&crud_mut);
			ParodusPrint("Inside addCRUDmsgToQueue : mutex unlock in producer \n");
		}
		else
		{
			CrudMsg *temp = crudMsgQ;
			while(temp->next)
			{
				temp = temp->next;
			}
			temp->next = crudMessage;
			pthread_mutex_unlock(&crud_mut);
		}
	}
	else
	{
		ParodusError("Memory allocation failed for CRUD\n");
	}
}


void *CRUDHandlerTask()
{
	int ret = 0;
	ssize_t resp_size = 0;
	void *resp_bytes;
	wrp_msg_t *crud_response = NULL;

	while(FOREVER())
	{
		pthread_mutex_lock(&crud_mut);
		ParodusPrint("Mutex lock in CRUD consumer thread\n");

		if(crudMsgQ !=NULL)
		{
			CrudMsg *message = crudMsgQ;
			crudMsgQ = crudMsgQ->next;
			pthread_mutex_unlock(&crud_mut);
			ParodusPrint("Mutex unlock in CRUD consumer thread\n");

			ret = processCrudRequest(message->msg, &crud_response);
			wrp_free_struct(message->msg);
			free(message);
			message = NULL;

			if(ret == 0)
			{
				ParodusInfo("CRUD processed successfully\n");
			}
			else
			{
				ParodusError("Failure in CRUD request processing !!\n");
			}
			ParodusPrint("msgpack encode to send to upstream\n");
			resp_size = wrp_struct_to( crud_response, WRP_BYTES, &resp_bytes );
			ParodusPrint("Encoded CRUD resp_size :%lu\n", resp_size);

			ParodusPrint("Adding CRUD response to upstreamQ\n");
			addCRUDresponseToUpstreamQ(resp_bytes, resp_size);
			wrp_free_struct(crud_response);
		}
		else
		{
			if (g_shutdown) {
			  pthread_mutex_unlock (&crud_mut);
			  break;
			}
			pthread_cond_wait(&crud_con, &crud_mut);
			pthread_mutex_unlock (&crud_mut);
		}
	}
	return 0;
}


//CRUD Producer adds the response into common UpStreamQ
void addCRUDresponseToUpstreamQ(void *response_bytes, ssize_t response_size)
{
	UpStreamMsg *response;

	response = (UpStreamMsg *)malloc(sizeof(UpStreamMsg));
	if(response)
	{
		response->msg =(void *)response_bytes;
		response->len =(int)response_size;
		response->next=NULL;
		pthread_mutex_lock (get_global_nano_mut());
		ParodusPrint("Mutex lock in CRUD response producer\n");

		if(get_global_UpStreamMsgQ() == NULL)
		{
			set_global_UpStreamMsgQ(response);
			ParodusPrint("Producer added CRUD response to UpStreamQ\n");
			pthread_cond_signal(get_global_nano_con());
			pthread_mutex_unlock (get_global_nano_mut());
			ParodusPrint("mutex unlock in CRUD response producer\n");
		}
		else
		{
			ParodusPrint("Producer adding CRUD response to UpStreamQ\n");
			UpStreamMsg *temp = get_global_UpStreamMsgQ();
			while(temp->next)
			{
				temp = temp->next;
			}
			temp->next = response;
			pthread_mutex_unlock (get_global_nano_mut());
		}
	}
	else
	{
		ParodusError("failure in allocation for CRUD response\n");
	}

}

