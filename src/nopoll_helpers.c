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
	
	ParodusInfo("handleUpstreamMessage length %zu\n", len);
	if(nopoll_conn_is_ok(conn) && nopoll_conn_is_ready(conn))
	{
		//bytesWritten = nopoll_conn_send_binary(conn, msg, len);
		bytesWritten = sendResponse(conn, msg, len);
		ParodusPrint("Number of bytes written: %d\n", bytesWritten);
		if (bytesWritten != (int) len) 
		{
			ParodusError("Failed to send bytes %zu, bytes written were=%d (errno=%d, %s)..\n", len, bytesWritten, errno, strerror(errno));
		}
	}
	else
	{
		ParodusError("Failed to send msg upstream as connection is not OK\n");
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
void listenerOnMessage(void * msg, size_t msgSize, int *numOfClients, reg_client **clients)
{
	
	int rv =0;
	wrp_msg_t *message;
	char* destVal = NULL;
	char dest[32] = {'\0'};
	
	int msgType;
	int p =0;
	int bytes =0;
	int destFlag =0;			
	int resp_size =0;
	const char *recivedMsg = NULL;
	char *str= NULL;
	wrp_msg_t *resp_msg = NULL;
	void *resp_bytes;
	
	recivedMsg =  (const char *) msg;
	
	ParodusInfo("Received msg from server:%s\n", recivedMsg);	
	if(recivedMsg!=NULL) 
	{
	
		/*** Decoding downstream recivedMsg to check destination ***/
		
		rv = wrp_to_struct(recivedMsg, msgSize, WRP_BYTES, &message);
				
		if(rv > 0)
		{
			ParodusPrint("\nDecoded recivedMsg of size:%d\n", rv);
			msgType = message->msg_type;
			ParodusInfo("msgType received:%d\n", msgType);
		
			if((message->u.req.dest !=NULL))
			{
				destVal = message->u.req.dest;
				strtok(destVal , "/");
				strcpy(dest,strtok(NULL , "/"));
				ParodusInfo("Received downstream dest as :%s\n", dest);
			
				//Checking for individual clients & Sending to each client
				
				for( p = 0; p < *numOfClients; p++ ) 
				{
					ParodusPrint("clients[%d].service_name is %s \n",p, clients[p]->service_name);
				    // Sending message to registered clients
				    if( strcmp(dest, clients[p]->service_name) == 0) 
				    {  
				    	ParodusPrint("sending to nanomsg client %s\n", dest);     
					bytes = nn_send(clients[p]->sock, recivedMsg, msgSize, 0);
					ParodusInfo("sent downstream message '%s' to reg_client '%s'\n",recivedMsg,clients[p]->url);
					ParodusPrint("downstream bytes sent:%d\n", bytes);
					destFlag =1;
			
				    } 				    				  
				    
				}
				
				//if any unknown dest received sending error response to server
				if(destFlag ==0)
				{
					ParodusError("Unknown dest:%s\n", dest);
																	
					cJSON *response = cJSON_CreateObject();
					cJSON_AddNumberToObject(response, "statusCode", 531);	
					cJSON_AddStringToObject(response, "message", "Service Unavailable");
					str = cJSON_PrintUnformatted(response);
					ParodusInfo("Payload Response: %s\n", str);
					resp_msg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
					memset(resp_msg, 0, sizeof(wrp_msg_t));

					resp_msg ->msg_type = msgType;
					resp_msg ->u.req.source = message->u.req.dest;
					resp_msg ->u.req.dest = message->u.req.source;
					resp_msg ->u.req.transaction_uuid=message->u.req.transaction_uuid;
					resp_msg ->u.req.payload = (void *)str;
					resp_msg ->u.req.payload_size = strlen(str);
					
					ParodusPrint("msgpack encode\n");
					resp_size = wrp_struct_to( resp_msg, WRP_BYTES, &resp_bytes );
					
				    sendUpstreamMsgToServer(&resp_bytes, resp_size);				
	
				}
				
			
		  	 }
	  	}
	  	
	  	else
	  	{
	  		ParodusError( "Failure in msgpack decoding for receivdMsg: rv is %d\n", rv );
	  	}
	  	
	  	ParodusPrint("free for downstream decoded msg\n");
	  	wrp_free_struct(message);
	  

        }
                
     
}
