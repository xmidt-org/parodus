/**
 * @file downstream.c
 *
 * @description This describes functions required to manage downstream messages.
 *
 * Copyright (c) 2015  Comcast
 */

#include "downstream.h"
#include "upstream.h" 
#include "connection.h"
#include "ParodusInternal.h"

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/**
 * @brief listenerOnMessage function to create WebSocket listener to receive connections
 *
 * @param[in] msg The message received from server for various process requests
 * @param[in] msgSize message size
 */
void listenerOnMessage(void * msg, size_t msgSize)
{
    int rv =0;
    wrp_msg_t *message;
    char* destVal = NULL;
    char dest[32] = {'\0'};
    int msgType;
    int bytes =0;
    int destFlag =0, size = 0;
    ssize_t resp_size = 0;
    const char *recivedMsg = NULL;
    char *str= NULL;
    wrp_msg_t *resp_msg = NULL;
    void *resp_bytes;
    reg_list_item_t *temp = NULL;

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
            ParodusPrint("numOfClients registered is %d\n", get_numOfClients());

            if((message->u.req.dest !=NULL))
            {
                destVal = message->u.req.dest;
                strtok(destVal , "/");
                parStrncpy(dest,strtok(NULL , "/"), sizeof(dest));
                ParodusInfo("Received downstream dest as :%s\n", dest);
                temp = get_global_node();
                //Checking for individual clients & Sending to each client

                while (NULL != temp)
                {
                    ParodusPrint("node is pointing to temp->service_name %s \n",temp->service_name);
                    // Sending message to registered clients
                    if( strcmp(dest, temp->service_name) == 0) 
                    {
                        ParodusPrint("sending to nanomsg client %s\n", dest);     
                        bytes = nn_send(temp->sock, recivedMsg, msgSize, 0);
                        ParodusInfo("sent downstream message '%s' to reg_client '%s'\n",recivedMsg,temp->url);
                        ParodusPrint("downstream bytes sent:%d\n", bytes);
                        destFlag =1;
                        break;
                    }
                    ParodusPrint("checking the next item in the list\n");
                    temp= temp->next;
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
                    size = (int) resp_size;
                    sendUpstreamMsgToServer(&resp_bytes, size);
                    free(str);
                    free(resp_msg);
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
