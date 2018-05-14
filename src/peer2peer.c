/**
 * @file peer2peer.c
 *
 * @description This describes functions required
 * to manage parodus peer to peer messages.
 *
 * Copyright (c) 2018  Comcast
 */

#include "ParodusInternal.h"
#include "config.h"
#include "upstream.h"
#include "parodus_interface.h"
#include "peer2peer.h"

P2P_Msg *inMsgQ = NULL;
pthread_mutex_t inMsgQ_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t inMsgQ_con=PTHREAD_COND_INITIALIZER;

P2P_Msg *outMsgQ = NULL;
pthread_mutex_t outMsgQ_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t outMsgQ_con=PTHREAD_COND_INITIALIZER;

/*----------------------------------------------------------------------------*/
/*                             External functions                             */
/*----------------------------------------------------------------------------*/

void *handle_P2P_Incoming(void *args)
{
    void *ptr;
    int l;
    P2P_Msg *inMsg;
    socket_handles_t *p_sock;
    ParodusPrint("****** %s *******\n",__FUNCTION__);
	int msgAdded=0;
    p_sock = (socket_handles_t *) args;
    while( FOREVER() )
    {
	    if (0 == strncmp("hub", get_parodus_cfg()->hub_or_spk, 3) ) 
	    {
            	l = check_inbox(p_sock->pipeline, &ptr);
		if (l > 0 && ptr != NULL)
		{
			inMsg = (P2P_Msg *)malloc(sizeof(P2P_Msg));
			inMsg->msg = malloc(l);
			memcpy(inMsg->msg,ptr,l);
			inMsg->len = l;
            		inMsg->next = NULL;
                        free_msg(ptr);
                        msgAdded = 1;
		}
	    } 
	    else 
	    {
		l = check_inbox(p_sock->pubsub, &ptr);
		if (l > 0 && ptr != NULL)
		{
			inMsg = (P2P_Msg *)malloc(sizeof(P2P_Msg));
			inMsg->msg = malloc(l);
			memcpy(inMsg->msg,ptr,l);
			inMsg->len = l;
            		inMsg->next = NULL;
                        free_msg(ptr);
                        msgAdded = 1;
		}
	    }
	    if(msgAdded)
        {
            pthread_mutex_lock (&inMsgQ_mut);
            if(inMsgQ == NULL)
            {
                inMsgQ = inMsg;
                ParodusPrint("Producer added message\n");
                pthread_cond_signal(&inMsgQ_con);
                pthread_mutex_unlock (&inMsgQ_mut);
                ParodusPrint("mutex unlock in producer thread\n");
            }
            else
            {
                P2P_Msg *temp = inMsgQ;
                while(temp->next)
                {
                    temp = temp->next;
                }
                temp->next = inMsg;
                pthread_mutex_unlock (&inMsgQ_mut);
            }
           msgAdded = 0;
         }
    }
    return NULL;
}

void *process_P2P_IncomingMessage(void *args)
{
    int rv=-1; (void) rv;
    wrp_msg_t *msg; (void) msg;
    bool status;
    socket_handles_t *p_sock;
    ParodusPrint("****** %s *******\n",__FUNCTION__);

    p_sock = (socket_handles_t *) args;
    while( FOREVER() )
    {
        pthread_mutex_lock (&inMsgQ_mut);
        ParodusPrint("mutex lock in consumer thread\n");
        if(inMsgQ != NULL)
        {
            P2P_Msg *message = inMsgQ;
            inMsgQ = inMsgQ->next;
            pthread_mutex_unlock (&inMsgQ_mut);
            ParodusPrint("mutex unlock in consumer thread\n");
            /*rv = wrp_to_struct( message->msg, message->len, WRP_BYTES, &msg );
            if(rv > 0)
            {
                if(msg->msg_type == WRP_MSG_TYPE__EVENT)
                {*/
			// For incoming of type HUB, use hub_send_msg() to propagate message to hardcoded spoke
			if (0 == strncmp("hub", get_parodus_cfg()->hub_or_spk, 3) )
			{
		            status = send_msg(p_sock->pubsub, message->msg, message->len);
		            if(status == true)
		            {
		                ParodusInfo("Successfully sent event to spoke\n");
		            }
		            else
		            {
		                ParodusError("Failed to send event to spoke\n");
		            }
			}
			//Send event to all registered clients for both hub and spoke incoming msg 
			sendToAllRegisteredClients(&message->msg, message->len);
			
			
			/*else if (0 == strncmp("spk", get_parodus_cfg()->hub_or_spk, 3) )
                        {
                            status = spoke_send_msg(SPK1_URL, message->msg, message->len);
                            if(status == true)
                            {
                                ParodusInfo("Successfully sent event to hub\n");
                            }
                            else
                            {
                                ParodusError("Failed to send event to hub\n");
                            }
                        }
                        else
			{
				// For incoming of type spoke, use sendToAllRegisteredClients() to propagate message to its local registered printer. 
                                // If source is itself then ignore the message
				sendToAllRegisteredClients(&message->msg, message->len);
			}*/
                /*}
            }
            else
            {
                ParodusError("Error in msgpack decoding for upstream\n");
            }

            wrp_free_struct(msg);
            msg = NULL;

            if(nn_freemsg (message->msg) < 0)
            {
                ParodusError ("Failed to free msg\n");
            }
            free(message);
            message = NULL;*/
        }
        else
        {
            ParodusPrint("Before pthread cond wait in consumer thread\n");
            pthread_cond_wait(&inMsgQ_con, &inMsgQ_mut);
            pthread_mutex_unlock (&inMsgQ_mut);
            ParodusPrint("mutex unlock in consumer thread after cond wait\n");
        }
    }
    return NULL;
}


/**
 * For outgoing of type HUB, use hub_send_msg() to propagate message to hardcoded spoke
 * For outgoing of type spoke, use spoke_send_msg()
**/
void *process_P2P_OutgoingMessage(void *args)
{
    int rv=-1; (void) rv;
    wrp_msg_t *msg; (void) msg;
    bool status;
    socket_handles_t *p_sock;
    ParodusInfo("****** %s *******\n",__FUNCTION__);

    p_sock = (socket_handles_t *) args;
    while( FOREVER() )
    {
        pthread_mutex_lock (&outMsgQ_mut);
        ParodusPrint("mutex lock in consumer thread\n");
        if(outMsgQ != NULL)
        {
            P2P_Msg *message = outMsgQ;
            outMsgQ = outMsgQ->next;
            pthread_mutex_unlock (&outMsgQ_mut);
            ParodusPrint("mutex unlock in consumer thread\n");
            //rv = wrp_to_struct( message->msg, message->len, WRP_BYTES, &msg );
            ParodusInfo("process_P2P_OutgoingMessage - message->msg = %p, message->len = %zd\n", message->msg, message->len);
            //if(rv > 0)
            {
                //if(msg->msg_type == WRP_MSG_TYPE__EVENT)
                {
		    if (0 == strncmp("hub", get_parodus_cfg()->hub_or_spk, 3) )
		    {
                            ParodusInfo("Just before hub send message\n");
		            status = send_msg(p_sock->pubsub, message->msg, message->len);
		            if(status == true)
		            {
		                ParodusInfo("Successfully sent event to spoke\n");
		            }
		            else
		            {
		                ParodusError("Failed to send event to spoke\n");
		            }
		     }
		     else
		     {
		            status = send_msg(p_sock->pipeline, message->msg, message->len);
		            if(status == true)
		            {
		                ParodusInfo("Successfully sent event to hub\n");
		            }
		            else
		            {
		                ParodusError("Failed to send event to hub\n");
		            }
		     }
                }
            }
            /* else
            {
                ParodusError("Error in msgpack decoding for upstream\n");
            }
             
            wrp_free_struct(msg);
            msg = NULL;

            if(nn_freemsg (message->msg) < 0)
            {
                ParodusError ("Failed to free msg\n");
            }*/
            free(message);
            message = NULL;
        }
        else
        {
            ParodusPrint("Before pthread cond wait in consumer thread\n");
            pthread_cond_wait(&outMsgQ_con, &outMsgQ_mut);
            pthread_mutex_unlock (&outMsgQ_mut);
            ParodusPrint("mutex unlock in consumer thread after cond wait\n");
        }
    }
    return NULL;
}

void add_P2P_OutgoingMessage(void **message, size_t len)
{
	P2P_Msg *outMsg;
	void *bytes;
    	ParodusInfo("****** %s *******\n",__FUNCTION__);

	outMsg = (P2P_Msg *)malloc(sizeof(P2P_Msg));

        if(outMsg)
        {
            ParodusInfo("add_P2P_OutgoingMessage - *message = %p, len = %zd\n", *message, len);
	    bytes = malloc(len);
	    memcpy(bytes,*message,len);
	    outMsg->msg = bytes;
            outMsg->len = len;
            outMsg->next = NULL;
            pthread_mutex_lock (&outMsgQ_mut);
            if(outMsgQ == NULL)
            {
                outMsgQ = outMsg;
                ParodusPrint("Producer added message\n");
                pthread_cond_signal(&outMsgQ_con);
                pthread_mutex_unlock (&outMsgQ_mut);
                ParodusPrint("mutex unlock in producer thread\n");
            }
            else
            {
                P2P_Msg *temp = outMsgQ;
                while(temp->next)
                {
                    temp = temp->next;
                }
                temp->next = outMsg;
                pthread_mutex_unlock (&outMsgQ_mut);
            }
        }
        else
        {
            ParodusError("Failed in memory allocation\n");
        }
}
