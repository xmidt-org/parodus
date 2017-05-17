/**
 * @file nopoll_handlers.c
 *
 * @description This describes nopoll handler functions.
 *
 * Copyright (c) 2015  Comcast
 */

#include "ParodusInternal.h"
#include "nopoll_handlers.h"
#include "connection.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

pthread_mutex_t g_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_cond=PTHREAD_COND_INITIALIZER;
ParodusMsg *ParodusMsgQ = NULL;

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/**
 * @brief listenerOnrequest_queue function to add messages to the queue
 *
 * @param[in] reqSize size of the incoming message
 * @param[in] requestMsg The message received from server for various process requests
 */
 
void listenerOnrequest_queue(void *requestMsg,int reqSize)
{
    ParodusMsg *message;
    message = (ParodusMsg *)malloc(sizeof(ParodusMsg));
    if(message)
    {
        message->payload = requestMsg;
        message->len = reqSize;
        message->next = NULL;

        pthread_mutex_lock (&g_mutex);		
        ParodusPrint("mutex lock in producer thread\n");

        if(ParodusMsgQ == NULL)
        {
            ParodusMsgQ = message;
            ParodusPrint("Producer added message\n");
            pthread_cond_signal(&g_cond);
            pthread_mutex_unlock (&g_mutex);
            ParodusPrint("mutex unlock in producer thread\n");
        }
        else
        {
            ParodusMsg *temp = ParodusMsgQ;
            while(temp->next)
            {
                temp = temp->next;
            }
            temp->next = message;
            pthread_mutex_unlock (&g_mutex);
        }
    }
    else
    {
        //Memory allocation failed
        ParodusError("Memory allocation is failed\n");
    }
    ParodusPrint("*****listenerOnrequest_queue*****\n");
}
 
