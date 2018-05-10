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
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

void handle_P2P_Incoming();
void process_P2P_IncomingMessage();

/*----------------------------------------------------------------------------*/
/*                             External functions                             */
/*----------------------------------------------------------------------------*/
void *handle_and_process_P2P_IncomingMessage()
{
    ParodusInfo("****** %s *******\n",__FUNCTION__);
    while( FOREVER() )
    {
        handle_P2P_Incoming();
        process_P2P_IncomingMessage();
    }
    return NULL;
}

/**
 * For outgoing of type HUB, use hub_send_msg() to propagate message to hardcoded spoke
 * For outgoing of type spoke, use spoke_send_msg()
**/
void *process_P2P_OutgoingMessage()
{
    int rv=-1; (void) rv;
    wrp_msg_t *msg; (void) msg;
    bool status;
    ParodusInfo("****** %s *******\n",__FUNCTION__);
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
            ParodusInfo("process_P2P_OutgoingMessage - message->msg = %p, message->len = %zd\n", message->msg, message->len);
		    if (0 == strncmp("hub", get_parodus_cfg()->hub_or_spk, 3) )
		    {
                            ParodusInfo("Just before hub send message\n");
		            status = hub_send_msg(SPK1_URL, message->msg, message->len);
		            if(status == true)
		            {
		                ParodusInfo("Successfully sent OutgoingMessage to spoke\n");
		            }
		            else
		            {
		                ParodusError("Failed to send OutgoingMessage to spoke\n");
		            }
		     }
		     else
		     {
		            status = spoke_send_msg(HUB_URL, message->msg, message->len);
		            if(status == true)
		            {
		                ParodusInfo("Successfully sent OutgoingMessage to hub\n");
		            }
		            else
		            {
		                ParodusError("Failed to send OutgoingMessage to hub\n");
		            }
            }
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
/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

void handle_P2P_Incoming()
{
    void *ptr;
    int l;
    P2P_Msg *inMsg;
    ParodusInfo("****** Start of %s *******\n",__FUNCTION__);

    if (0 == strncmp("hub", get_parodus_cfg()->hub_or_spk, 3) ) 
    {
        l = hub_check_inbox(&ptr);
    }
    else
    {
        l = spoke_check_inbox(&ptr);
    }
    if (l > 0 && ptr != NULL)
    {
        inMsg = (P2P_Msg *)malloc(sizeof(P2P_Msg));
        inMsg->msg = malloc(l);
        memcpy(inMsg->msg,ptr,l);
        inMsg->len = l;
        inMsg->next = NULL;
    }
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
    ParodusInfo("****** End of %s *******\n",__FUNCTION__);
}

void process_P2P_IncomingMessage()
{
    int rv=-1; (void) rv;
    wrp_msg_t *msg; (void) msg;
    bool status;
    ParodusInfo("****** Start of %s *******\n",__FUNCTION__);
    pthread_mutex_lock (&inMsgQ_mut);
    ParodusPrint("mutex lock in consumer thread\n");
    if(inMsgQ != NULL)
    {
        P2P_Msg *message = inMsgQ;
        inMsgQ = inMsgQ->next;
        pthread_mutex_unlock (&inMsgQ_mut);
        ParodusPrint("mutex unlock in consumer thread\n");
		// For incoming of type HUB, use hub_send_msg() to propagate message to hardcoded spoke
		if (0 == strncmp("hub", get_parodus_cfg()->hub_or_spk, 3) )
		{
	            status = hub_send_msg(SPK1_URL, message->msg, message->len);
	            if(status == true)
	            {
	                ParodusInfo("Successfully sent incomingMessage to spoke\n");
	            }
	            else
	            {
	                ParodusError("Failed to send incomingMessage to spoke\n");
	            }
		}
		ParodusInfo("%s: B4 sendToAllRegisteredClients()\n",__FUNCTION__);
		//Send event to all registered clients for both hub and spoke incoming msg 
		sendToAllRegisteredClients(&message->msg, message->len);
    }
    else
    {
        ParodusPrint("Before pthread cond wait in consumer thread\n");
        pthread_cond_wait(&inMsgQ_con, &inMsgQ_mut);
        pthread_mutex_unlock (&inMsgQ_mut);
        ParodusPrint("mutex unlock in consumer thread after cond wait\n");
    }
    ParodusInfo("****** End of %s *******\n",__FUNCTION__);
}

