/**
 * Copyright 2016 Comcast Cable Communications Management, LLC
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
#include "thread_tasks.h"
#include "downstream.h"
#include "ParodusInternal.h"
#include "client_list.h"

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void *messageHandlerTask()
{
    while(FOREVER())
    {
        pthread_mutex_lock (&g_mutex);
        ParodusPrint("mutex lock in consumer thread\n");
        if(ParodusMsgQ != NULL)
        {
            ParodusMsg *message = ParodusMsgQ;
            ParodusMsgQ = ParodusMsgQ->next;
            pthread_mutex_unlock (&g_mutex);
            ParodusPrint("mutex unlock in consumer thread\n");
            int numOfClients = get_numOfClients();
            listenerOnMessage(message->payload, message->len, &numOfClients, get_global_node());

            nopoll_msg_unref(message->msg);
            free(message);
            message = NULL;
        }
        else
        {
            ParodusPrint("Before pthread cond wait in consumer thread\n");
            pthread_cond_wait(&g_cond, &g_mutex);
            pthread_mutex_unlock (&g_mutex);
            ParodusPrint("mutex unlock in consumer thread after cond wait\n");
        }
    }
    ParodusPrint ("Ended messageHandlerTask\n");
    return 0;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
