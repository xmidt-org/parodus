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
 * @file downstream.c
 *
 * @description This describes functions required to manage downstream messages.
 *
 */

#include "downstream.h"
#include "upstream.h" 
#include "connection.h"
#include "partners_check.h"
#include "ParodusInternal.h"
#include "crud_interface.h"
#include "xmidtsend_rbus.h"
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void createNewMsgForCRUD(wrp_msg_t *message, wrp_msg_t **crudMessage );
static void createNewMsgForCloudACK(wrp_msg_t *message, wrp_msg_t **eventMessage ); //Test purpose.
static int test = 0;
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
    wrp_msg_t *crudMessage= NULL;
    char* destVal = NULL;
    char dest[32] = {'\0'};
    int msgType;
    int bytes =0;
    int destFlag =0;
    size_t size = 0;
    int resp_size = -1 ;
    const char *recivedMsg = NULL;
    char *str= NULL;
    wrp_msg_t *resp_msg = NULL;
    void *resp_bytes;
    cJSON *response = NULL;
    reg_list_item_t *temp = NULL;

    recivedMsg =  (const char *) msg;

    ParodusInfo("Received msg from server\n");
    if(recivedMsg!=NULL) 
    {
        /*** Decoding downstream recivedMsg to check destination ***/
        rv = wrp_to_struct(recivedMsg, msgSize, WRP_BYTES, &message);

        if(rv > 0)
        {
            ParodusPrint("\nDecoded recivedMsg of size:%d\n", rv);
            msgType = message->msg_type;
            ParodusInfo("msgType received:%d\n", msgType);
            
            switch( message->msg_type )
            {
                case WRP_MSG_TYPE__AUTH:
                {
                    ParodusInfo("Authorization Status received with Status code :%d\n", message->u.auth.status);
                    break;
                }

                case WRP_MSG_TYPE__EVENT:
                case WRP_MSG_TYPE__REQ:
                case WRP_MSG_TYPE__CREATE:
                case WRP_MSG_TYPE__UPDATE:
                case WRP_MSG_TYPE__RETREIVE:
                case WRP_MSG_TYPE__DELETE:
                {
                    ParodusPrint("numOfClients registered is %d\n", get_numOfClients());
                    int ret = validate_partner_id(message, NULL);
                    if(ret < 0)
                    {
                        response = cJSON_CreateObject();
                        cJSON_AddNumberToObject(response, "statusCode", 403);
                        cJSON_AddStringToObject(response, "message", "Invalid partner_id");
                    } 

                      
                    destVal = strdup(((WRP_MSG_TYPE__EVENT == msgType) ? message->u.event.dest : 
                              ((WRP_MSG_TYPE__REQ   == msgType) ? message->u.req.dest : message->u.crud.dest)));

                    if( (destVal != NULL) && (ret >= 0) )
                    {
						char *newDest = NULL;
						char *tempDest = strtok(destVal , "/");
						if(tempDest != NULL)
						{
							newDest = strtok(NULL , "/");
						}
						if(newDest != NULL)
						{
							parStrncpy(dest,newDest, sizeof(dest));
						}
						else
						{
							parStrncpy(dest,destVal, sizeof(dest));
						}
                        ParodusInfo("Received downstream dest as :%s and transaction_uuid :%s\n", dest, 
                            ((WRP_MSG_TYPE__REQ   == msgType) ? message->u.req.transaction_uuid : 
                            ((WRP_MSG_TYPE__EVENT == msgType) ? "NA" : message->u.crud.transaction_uuid)));
			OnboardLog("%s\n",
                            ((WRP_MSG_TYPE__REQ   == msgType) ? message->u.req.transaction_uuid :
                            ((WRP_MSG_TYPE__EVENT == msgType) ? "NA" : message->u.crud.transaction_uuid)));
                        
                        free(destVal);

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
                                ParodusInfo("sent downstream message to reg_client '%s'\n",temp->url);
                                ParodusPrint("downstream bytes sent:%d\n", bytes);
                                destFlag =1;
				test++;
                                break;
                            }
                            ParodusPrint("checking the next item in the list\n");
                            temp= temp->next;
                        }
                        release_global_node ();

			/* check Downstream dest for CRUD requests */
			if(destFlag ==0 && strcmp("parodus", dest)==0)
			{
							ParodusPrint("Received CRUD request : dest : %s\n", dest);
							if ((message->u.crud.source == NULL) || (message->u.crud.transaction_uuid == NULL))
							{
								ParodusError("Invalid request\n");
								response = cJSON_CreateObject();
								cJSON_AddNumberToObject(response, "statusCode", 400);
								cJSON_AddStringToObject(response, "message", "Invalid Request");
								ret = -1;
							}
							else
							{
							    createNewMsgForCRUD(message, &crudMessage);
							    addCRUDmsgToQueue(crudMessage);
							}
							destFlag =1;
			}
			//if any unknown dest received sending error response to server
                        if(destFlag ==0)
                        {
                            ParodusError("Unknown dest:%s\n", dest);
                            response = cJSON_CreateObject();
                            cJSON_AddNumberToObject(response, "statusCode", 531);
                            cJSON_AddStringToObject(response, "message", "Service Unavailable");
                        }
                    }

                    if( (WRP_MSG_TYPE__EVENT != msgType) &&
                        ((destFlag == 0) || (ret < 0)) )
                    {
                        resp_msg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
                        memset(resp_msg, 0, sizeof(wrp_msg_t));

                        resp_msg ->msg_type = msgType;
                        if( WRP_MSG_TYPE__REQ == msgType )
                        {
                            resp_msg ->u.req.source = message->u.req.dest;
                            resp_msg ->u.req.dest = message->u.req.source;
                            resp_msg ->u.req.transaction_uuid=message->u.req.transaction_uuid;
                        }
                        else
                        {
                            resp_msg ->u.crud.source = message->u.crud.dest;
				if(message->u.crud.source !=NULL)
				{
					resp_msg ->u.crud.dest = message->u.crud.source;
				}
				else
				{
					resp_msg ->u.crud.dest = "unknown";
				}
                            resp_msg ->u.crud.transaction_uuid = message->u.crud.transaction_uuid;
                            resp_msg ->u.crud.path =             message->u.crud.path;
                        }

                        if(response != NULL)
                        {
                            str = cJSON_PrintUnformatted(response);
                            ParodusInfo("Payload Response: %s\n", str);

                            if( WRP_MSG_TYPE__REQ == msgType )
                            {
                                resp_msg ->u.req.payload = (void *)str;
                                resp_msg ->u.req.payload_size = strlen(str);
                            }
                            else
                            {
                                resp_msg ->u.crud.payload = (void *)str;
                                resp_msg ->u.crud.payload_size = strlen(str);
                            }

                            ParodusPrint("msgpack encode\n");
                            resp_size = wrp_struct_to( resp_msg, WRP_BYTES, &resp_bytes );
                            if(resp_size > 0)
                            {
                                size = (size_t) resp_size;
                                sendUpstreamMsgToServer(&resp_bytes, size);
                            }
                            free(str);
                            cJSON_Delete(response);
                            free(resp_bytes);
                            resp_bytes = NULL;
                        }
                        free(resp_msg);
                    }
		    //To handle cloud ack events received from server for the xmidt sent messages.
		    if(test == 1 || test == 3)
		    {
			wrp_msg_t *eventMsg= NULL;
			ParodusInfo("Create downstream event Msg with cloud ack\n");
			createNewMsgForCloudACK(message, &eventMsg);
			msgType = WRP_MSG_TYPE__EVENT;
			ParodusPrint("check cloud ack\n");
		    if((WRP_MSG_TYPE__EVENT == msgType) && (ret >= 0))
		    {
			//Process cloud ack only when qos > 24
			/*if(highQosValueCheck(message->u.event.qos))
			{
				if(message->u.event.transaction_uuid !=NULL)
				{
					ParodusInfo("Received cloud ack from server: transaction_uuid %s qos %d, rdr %d\n", message->u.event.transaction_uuid, message->u.event.qos, message->u.event.rdr);
					addToCloudAckQ(message->u.event.transaction_uuid, message->u.event.qos, message->u.event.rdr);
					ParodusInfo("Added to cloud ack Q\n");
				}
				else
				{
					ParodusError("cloud ack transaction id is NULL\n");
				}
			}
			else
			{
				ParodusInfo("cloud ack received with low qos %d, ignoring it\n", message->u.event.qos);
			}*/
			//Remove this. TEST purpose.
			if(highQosValueCheck(eventMsg->u.event.qos))
			{
				if(eventMsg->u.event.transaction_uuid !=NULL)
				{
					ParodusInfo("Received cloud ack from server: transaction_uuid %s qos %d, rdr %d\n", eventMsg->u.event.transaction_uuid, eventMsg->u.event.qos, eventMsg->u.event.rdr);
					addToCloudAckQ(eventMsg->u.event.transaction_uuid, eventMsg->u.event.qos, eventMsg->u.event.rdr);
					ParodusPrint("Added to cloud ack Q\n");
				}
				else
				{
					ParodusError("cloud ack transaction id is NULL\n");
				}
			}
			else
			{
				ParodusInfo("cloud ack received with low qos %d, ignoring it\n", eventMsg->u.event.qos);
			}
			ParodusInfo("test is %d\n", test);
		    }
		    }
                    break;
                }

                case WRP_MSG_TYPE__SVC_REGISTRATION:
                case WRP_MSG_TYPE__SVC_ALIVE:
                case WRP_MSG_TYPE__UNKNOWN:
                default:
                    break;
            }
            ParodusPrint("free for downstream decoded msg\n");
            wrp_free_struct(message);
            message = NULL;
        }
        else
        {
            ParodusError( "Failure in msgpack decoding for receivdMsg: rv is %d\n", rv );
        }
    }
}
/*----------------------------------------------------------------------------*/
/*                             Internal Functions                             */
/*----------------------------------------------------------------------------*/
/**
 * @brief createNewMsgForCRUD function to create new message for processing CRUD requests
 *
 * @param[in] message The message received from server
 * @param[out] crudMessage New message for processing CRUD requests
 */
static void createNewMsgForCRUD(wrp_msg_t *message, wrp_msg_t **crudMessage )
{
    wrp_msg_t *msg;
    msg = ( wrp_msg_t * ) malloc( sizeof( wrp_msg_t ) );
    size_t i;
    if(msg != NULL)
    {
        memset( msg, 0, sizeof( wrp_msg_t ) );
        msg->msg_type = message->msg_type;
        if(message->u.crud.source != NULL)
        {
            ParodusPrint("message->u.crud.source = %s\n",message->u.crud.source);
            msg->u.crud.source = strdup(message->u.crud.source);
        }

        if(message->u.crud.dest!= NULL)
        {
            ParodusPrint("message->u.crud.dest = %s\n",message->u.crud.dest);
            msg->u.crud.dest = strdup(message->u.crud.dest);
        }

        if(message->u.crud.transaction_uuid != NULL)
        {
            ParodusPrint("message->u.crud.transaction_uuid = %s\n",message->u.crud.transaction_uuid);
            msg->u.crud.transaction_uuid = strdup(message->u.crud.transaction_uuid);
        }

        if(message->u.crud.partner_ids!= NULL && message->u.crud.partner_ids->count >0)
        {
            msg->u.crud.partner_ids = ( partners_t * ) malloc( sizeof( partners_t ) +
                                                              sizeof( char * ) * message->u.crud.partner_ids->count );
            if(msg->u.crud.partner_ids != NULL)
            {
                msg->u.crud.partner_ids->count = message->u.crud.partner_ids->count;
                for(i = 0; i<message->u.crud.partner_ids->count; i++)
                {
                    ParodusPrint("message->u.crud.partner_ids->partner_ids[%d] = %s\n",i,message->u.crud.partner_ids->partner_ids[i]);
                    msg->u.crud.partner_ids->partner_ids[i] = strdup(message->u.crud.partner_ids->partner_ids[i]);
                }
            }
        }

        if(message->u.crud.headers!= NULL && message->u.crud.headers->count >0)
        {
            msg->u.crud.headers = ( headers_t * ) malloc( sizeof( headers_t ) +
                                                                sizeof( char * ) * message->u.crud.headers->count );
            if(msg->u.crud.headers != NULL)
            {
                msg->u.crud.headers->count = message->u.crud.headers->count;
                for(i = 0; i<message->u.crud.headers->count; i++)
                {
                    ParodusPrint("message->u.crud.headers->headers[%d] = %s\n",i,message->u.crud.headers->headers[i]);
                    msg->u.crud.headers->headers[i] = strdup(message->u.crud.headers->headers[i]);
                }
            }
        }

        if(message->u.crud.metadata != NULL && message->u.crud.metadata->count > 0)
        {
            msg->u.crud.metadata = (data_t *) malloc( sizeof( data_t ) );
            if(msg->u.crud.metadata != NULL)
            {
                memset( msg->u.crud.metadata, 0, sizeof( data_t ) );
                msg->u.crud.metadata->count = message->u.crud.metadata->count;
                msg->u.crud.metadata->data_items = ( struct data* )malloc( sizeof( struct data ) * ( message->u.crud.metadata->count ) );
                for(i=0; i<message->u.crud.metadata->count; i++)
                {
                    if(message->u.crud.metadata->data_items[i].name != NULL)
                    {
                        ParodusPrint("message->u.crud.metadata->data_items[%d].name : %s\n",i,message->u.crud.metadata->data_items[i].name);
                        msg->u.crud.metadata->data_items[i].name = strdup(message->u.crud.metadata->data_items[i].name);
                    }
                    if(message->u.crud.metadata->data_items[i].value != NULL)
                    {
                        ParodusPrint("message->u.crud.metadata->data_items[%d].value : %s\n",i,message->u.crud.metadata->data_items[i].value);
                        msg->u.crud.metadata->data_items[i].value = strdup(message->u.crud.metadata->data_items[i].value);
                    }
                }
            }
        }
        msg->u.crud.include_spans = message->u.crud.include_spans;
        if(message->u.crud.content_type != NULL)
        {
            ParodusPrint("message->u.crud.content_type : %s\n",message->u.crud.content_type);
            msg->u.crud.content_type = strdup(message->u.crud.content_type);
        }
        msg->u.crud.spans.spans = NULL;   /* not supported */
        msg->u.crud.spans.count = 0;     /* not supported */
        msg->u.crud.status = message->u.crud.status;
        msg->u.crud.rdr = message->u.crud.rdr;
        if(message->u.crud.payload != NULL)
        {
            ParodusPrint("message->u.crud.payload = %s\n", (char *)message->u.crud.payload);
            msg->u.crud.payload = strdup((char *)message->u.crud.payload);
        }
        msg->u.crud.payload_size = message->u.crud.payload_size;
        if(message->u.crud.path != NULL)
        {
            ParodusPrint("message->u.crud.path = %s\n", message->u.crud.path);
            msg->u.crud.path = strdup(message->u.crud.path);
        }
        *crudMessage = msg;
    }
}
//Test purpose. to create new message for processing cloud ACK .
static void createNewMsgForCloudACK(wrp_msg_t *message, wrp_msg_t **eventMessage )
{
    wrp_msg_t *msg;
    msg = ( wrp_msg_t * ) malloc( sizeof( wrp_msg_t ) );
    if(msg != NULL)
    {
        memset( msg, 0, sizeof( wrp_msg_t ) );
        msg->msg_type = WRP_MSG_TYPE__EVENT;
        if(message->u.event.source != NULL)
        {
            msg->u.event.source = strdup("event:/profile-notify/MyProfile1");
        }

        if(message->u.event.dest != NULL)
        {
            msg->u.event.dest = strdup("mac:889e6863239e/telemetry2");
        }

        if(message->u.event.transaction_uuid != NULL)
        {
            msg->u.event.transaction_uuid = strdup("8d72d4c2-1f59-4420-a736-3946083d529a");
        }

        if(message->u.event.content_type != NULL)
        {
            msg->u.event.content_type = strdup("application/json");
        }
        msg->u.event.rdr = 0;
	msg->u.event.qos = 50;
	ParodusInfo("msg->u.event.rdr = %d msg->u.event.qos = %d\n",msg->u.event.rdr, msg->u.event.qos);
        *eventMessage = msg;
    }
    ParodusInfo("createNewMsgForCloudACK done\n");
}
