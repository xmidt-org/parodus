/**
 * @file websocket_mgr.c
 *
 * @description This file is used to manage websocket connection, websocket messages and ping/pong (heartbeat) mechanism.
 *
 * Copyright (c) 2015  Comcast
 */

#include "ParodusInternal.h"
#include "time.h"
#include "connection.h"
#include "spin_thread.h"
#include "client_list.h"
#include "nopoll_helpers.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
reg_list_item_t * head = NULL;

void *metadataPack;
size_t metaPackSize=-1;

UpStreamMsg *UpStreamMsgQ = NULL;

pthread_mutex_t nano_mut=PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t nano_con=PTHREAD_COND_INITIALIZER;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/


/**
 * @brief Interface to terminate WebSocket client connections and clean up resources.
 */

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

       
/*
 * @brief To handle UpStream messages which is received from nanomsg server socket
 */

void *handle_upstream()
{
	UpStreamMsg *message;
	int sock, bind;
	int bytes =0;
	void *buf;
	
	ParodusPrint("******** Start of handle_upstream ********\n");
		
	sock = nn_socket( AF_SP, NN_PULL );
	if(sock >= 0)
	{
                bind = nn_bind(sock, parodus_url);
                if(bind < 0)
                {
                        ParodusError("Unable to bind socket (errno=%d, %s)\n",errno, strerror(errno));
                }
                else
                {
                        while( 1 ) 
                        {
                                buf = NULL;
                                ParodusInfo("nanomsg server gone into the listening mode...\n");
                                bytes = nn_recv (sock, &buf, NN_MSG, 0);
                                ParodusInfo ("Upstream message received from nanomsg client: \"%s\"\n", (char*)buf);
                                message = (UpStreamMsg *)malloc(sizeof(UpStreamMsg));

                                if(message)
                                {
                                        message->msg =buf;
                                        message->len =bytes;
                                        message->next=NULL;
                                        pthread_mutex_lock (&nano_mut);
                                        //Producer adds the nanoMsg into queue
                                        if(UpStreamMsgQ == NULL)
                                        {
                                                UpStreamMsgQ = message;

                                                ParodusPrint("Producer added message\n");
                                                pthread_cond_signal(&nano_con);
                                                pthread_mutex_unlock (&nano_mut);
                                                ParodusPrint("mutex unlock in producer thread\n");
                                        }
                                        else
                                        {
                                                UpStreamMsg *temp = UpStreamMsgQ;
                                                while(temp->next)
                                                {
                                                        temp = temp->next;
                                                }

                                                temp->next = message;
                                                pthread_mutex_unlock (&nano_mut);
                                        }
                                }
                                else
                                {
                                        ParodusError("failure in allocation for message\n");
                                }
                        }
                }
	}
	else
	{
		    ParodusError("Unable to create socket (errno=%d, %s)\n",errno, strerror(errno));
	}
	ParodusPrint ("End of handle_upstream\n");
	return 0;
}


void *handleUpStreamEvents()
{		
        int rv=-1, rc = -1;	
        int msgType;
        wrp_msg_t *msg;	
        void *appendData;
        size_t encodedSize;
        reg_list_item_t *temp = NULL;
        int matchFlag = 0;
        int status = -1;

        while(1)
        {
                pthread_mutex_lock (&nano_mut);
                ParodusPrint("mutex lock in consumer thread\n");
                if(UpStreamMsgQ != NULL)
                {
                        UpStreamMsg *message = UpStreamMsgQ;
                        UpStreamMsgQ = UpStreamMsgQ->next;
                        pthread_mutex_unlock (&nano_mut);
                        ParodusPrint("mutex unlock in consumer thread\n");

                        if (!terminated) 
                        {
                                /*** Decoding Upstream Msg to check msgType ***/
                                /*** For MsgType 9 Perform Nanomsg client Registration else Send to server ***/	
                                ParodusPrint("---- Decoding Upstream Msg ----\n");

                                rv = wrp_to_struct( message->msg, message->len, WRP_BYTES, &msg );
                                if(rv > 0)
                                {
                                        msgType = msg->msg_type;				   
                                        if(msgType == 9)
                                        {
                                                ParodusInfo("\n Nanomsg client Registration for Upstream\n");
                                                //Extract serviceName and url & store it in a linked list for reg_clients
                                                if(numOfClients !=0)
                                                {
                                                        temp = head;
                                                        while(temp!=NULL)
                                                        {
                                                                if(strcmp(temp->service_name, msg->u.reg.service_name)==0)
                                                                {
                                                                        ParodusInfo("match found, client is already registered\n");
                                                                        strncpy(temp->url,msg->u.reg.url, strlen(msg->u.reg.url)+1);
                                                                        if(nn_shutdown(temp->sock, 0) < 0)
                                                                        {
                                                                                ParodusError ("Failed to shutdown\n");
                                                                        }

                                                                        temp->sock = nn_socket(AF_SP,NN_PUSH );
                                                                        if(temp->sock >= 0)
                                                                        {					
                                                                                int t = NANOMSG_SOCKET_TIMEOUT_MSEC;
                                                                                rc = nn_setsockopt(temp->sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
                                                                                if(rc < 0)
                                                                                {
                                                                                        ParodusError ("Unable to set socket timeout (errno=%d, %s)\n",errno, strerror(errno));
                                                                                }
                                                                                rc = nn_connect(temp->sock, msg->u.reg.url); 
                                                                                if(rc < 0)
                                                                                {
                                                                                        ParodusError ("Unable to connect socket (errno=%d, %s)\n",errno, strerror(errno));
                                                                                }
                                                                                else
                                                                                {
                                                                                        ParodusInfo("Client registered before. Sending acknowledgement \n"); 
                                                                                        status =sendAuthStatus(temp);
                                                                        
                                                                                        if(status == 0)
                                                        						{
                                                        								ParodusPrint("sent auth status to reg client\n");
	                        		
	                															}
                                                                                        matchFlag = 1;
                                                                                        break;
                                                                                }
                                                                        }
                                                                        else
                                                                        {
                                                                                ParodusError("Unable to create socket (errno=%d, %s)\n",errno, strerror(errno));
                                                                        }
                                                                }
                                                                ParodusPrint("checking the next item in the list\n");
                                                                temp= temp->next;
                                                        }	
                                                }
                                                ParodusPrint("matchFlag is :%d\n", matchFlag);
                                                if((matchFlag == 0) || (numOfClients == 0))
                                                {
                                                 
                                                        ParodusPrint("Adding nanomsg clients to list\n");
                                                        status = addToList(&msg);
                                                        ParodusPrint("addToList status is :%d\n", status);
                                                        if(status == 0)
                                                        {
                                                        	ParodusPrint("sent auth status to reg client\n");
	                        		
	                        							}
                                                }
                                        }
                                        else
                                        {
                                                //Sending to server for msgTypes 3, 4, 5, 6, 7, 8.
                                                ParodusInfo(" Received upstream data with MsgType: %d\n", msgType);
                                                //Appending metadata with packed msg received from client
                                                if(metaPackSize > 0)
                                                {
                                                        ParodusPrint("Appending received msg with metadata\n");
                                                        encodedSize = appendEncodedData( &appendData, message->msg, message->len, metadataPack, metaPackSize );
                                                        ParodusPrint("encodedSize after appending :%zu\n", encodedSize);
                                                        ParodusPrint("metadata appended upstream msg %s\n", (char *)appendData);
                                                        ParodusInfo("Sending metadata appended upstream msg to server\n");
                                                        handleUpstreamMessage(get_global_conn(),appendData, encodedSize);

                                                        free( appendData);
                                                        appendData =NULL;
                                                }
                                                else
                                                {		
                                                        ParodusError("Failed to send upstream as metadata packing is not successful\n");
                                                }
                                        }
                                }
                                else
                                {
                                        ParodusError("Error in msgpack decoding for upstream\n");
                                }
                                ParodusPrint("Free for upstream decoded msg\n");
                                wrp_free_struct(msg);
                                msg = NULL;
                        }

                        if(nn_freemsg (message->msg) < 0)
                        {
                                ParodusError ("Failed to free msg\n");
                        }
                        free(message);
                        message = NULL;
                }
                else
                {
                        ParodusPrint("Before pthread cond wait in consumer thread\n");   
                        pthread_cond_wait(&nano_con, &nano_mut);
                        pthread_mutex_unlock (&nano_mut);
                        ParodusPrint("mutex unlock in consumer thread after cond wait\n");
                        if (terminated) 
                        {
                                break;
                        }
                }
        }
        return NULL;
}

void sendUpstreamMsgToServer(void **resp_bytes, int resp_size)
{
	void *appendData;
	size_t encodedSize;
	//appending response with metadata 			
	if(metaPackSize > 0)
	{
	   	encodedSize = appendEncodedData( &appendData, *resp_bytes, resp_size, metadataPack, metaPackSize );
	   	ParodusPrint("metadata appended upstream response %s\n", (char *)appendData);
	   	ParodusPrint("encodedSize after appending :%zu\n", encodedSize);
	   		   
		ParodusInfo("Sending response to server\n");
	   	handleUpstreamMessage(get_global_conn(),appendData, encodedSize);
	   	
		free( appendData);
		appendData =NULL;
	}
	else
	{		
		ParodusError("Failed to send upstream as metadata packing is not successful\n");
	}

}

