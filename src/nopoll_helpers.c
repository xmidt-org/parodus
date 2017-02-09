/**
 * @file internal.c
 *
 * @description This file is used to manage internal functions of parodus
 *
 * Copyright (c) 2015  Comcast
 */

#include "ParodusInternal.h"
#include "wss_mgr.h"
#include "connection.h"

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/



/** To send upstream msgs to server ***/

void handleUpstreamMessage(noPollConn *conn, void *msg, size_t len)
{
	int bytesWritten = 0;
	
	PARODUS_INFO("handleUpstreamMessage length %zu\n", len);
	if(nopoll_conn_is_ok(conn) && nopoll_conn_is_ready(conn))
	{
		//bytesWritten = nopoll_conn_send_binary(conn, msg, len);
		bytesWritten = sendResponse(conn, msg, len);
		PARODUS_DEBUG("Number of bytes written: %d\n", bytesWritten);
		if (bytesWritten != (int) len) 
		{
			PARODUS_ERROR("Failed to send bytes %zu, bytes written were=%d (errno=%d, %s)..\n", len, bytesWritten, errno, strerror(errno));
		}
	}
	else
	{
		PARODUS_ERROR("Failed to send msg upstream as connection is not OK\n");
	}
	
}



/**
 * @brief listenerOnMessage function to create WebSocket listener to receive connections
 *
 * @param[in] ctx The context where the connection happens.
 * @param[in] conn The Websocket connection object
 * @param[in] msg The message received from server for various process requests
 * @param[out] user_data data which is to be sent
 */
void listenerOnMessage(void * msg, size_t msgSize, int *numOfClients, reg_list_item_t **head )
{
	
	int rv =0;
	wrp_msg_t *message;
	char* destVal = NULL;
	char dest[32] = {'\0'};
	
	int msgType;
	int bytes =0;
	int destFlag =0;			
	int resp_size =0;
	const char *recivedMsg = NULL;
	char *str= NULL;
	wrp_msg_t *resp_msg = NULL;
	void *resp_bytes;
	reg_list_item_t *temp = NULL;
	
	recivedMsg =  (const char *) msg;
	
	PARODUS_INFO("Received msg from server:%s\n", recivedMsg);	
	if(recivedMsg!=NULL) 
	{
	
		/*** Decoding downstream recivedMsg to check destination ***/
		
		rv = wrp_to_struct(recivedMsg, msgSize, WRP_BYTES, &message);
				
		if(rv > 0)
		{
			PARODUS_DEBUG("\nDecoded recivedMsg of size:%d\n", rv);
			msgType = message->msg_type;
			PARODUS_INFO("msgType received:%d\n", msgType);
			PARODUS_DEBUG("numOfClients registered is %d\n", *numOfClients);
		
			if((message->u.req.dest !=NULL))
			{
				destVal = message->u.req.dest;
				strtok(destVal , "/");
				strcpy(dest,strtok(NULL , "/"));
				PARODUS_INFO("Received downstream dest as :%s\n", dest);
				temp = *head;
				//Checking for individual clients & Sending to each client
				
				while (NULL != temp)
				{
					PARODUS_DEBUG("node is pointing to temp->service_name %s \n",temp->service_name);
					// Sending message to registered clients
					if( strcmp(dest, temp->service_name) == 0) 
					{
						PARODUS_DEBUG("sending to nanomsg client %s\n", dest);     
						bytes = nn_send(temp->sock, recivedMsg, msgSize, 0);
						PARODUS_INFO("sent downstream message '%s' to reg_client '%s'\n",recivedMsg,temp->url);
						PARODUS_DEBUG("downstream bytes sent:%d\n", bytes);
						destFlag =1;
						break;
				
					}
					PARODUS_DEBUG("checking the next item in the list\n");
					temp= temp->next;
					
				}
				
				
				//if any unknown dest received sending error response to server
				if(destFlag ==0)
				{
					PARODUS_ERROR("Unknown dest:%s\n", dest);
																	
					cJSON *response = cJSON_CreateObject();
					cJSON_AddNumberToObject(response, "statusCode", 531);	
					cJSON_AddStringToObject(response, "message", "Service Unavailable");
					str = cJSON_PrintUnformatted(response);
					PARODUS_INFO("Payload Response: %s\n", str);
					resp_msg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
					memset(resp_msg, 0, sizeof(wrp_msg_t));

					resp_msg ->msg_type = msgType;
					resp_msg ->u.req.source = message->u.req.dest;
					resp_msg ->u.req.dest = message->u.req.source;
					resp_msg ->u.req.transaction_uuid=message->u.req.transaction_uuid;
					resp_msg ->u.req.payload = (void *)str;
					resp_msg ->u.req.payload_size = strlen(str);
					
					PARODUS_DEBUG("msgpack encode\n");
					resp_size = wrp_struct_to( resp_msg, WRP_BYTES, &resp_bytes );
					
				    sendUpstreamMsgToServer(&resp_bytes, resp_size);				
	
				}
				
			
		  	 }
	  	}
	  	
	  	else
	  	{
	  		PARODUS_ERROR( "Failure in msgpack decoding for receivdMsg: rv is %d\n", rv );
	  	}
	  	
	  	PARODUS_DEBUG("free for downstream decoded msg\n");
	  	wrp_free_struct(message);
	  

        }
                
     
}
