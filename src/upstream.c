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
 * @file upstream.c
 *
 * @description This describes functions required to manage upstream messages.
 *
 */

#include "ParodusInternal.h"
#include "upstream.h"
#include "config.h"
#include "partners_check.h"
#include "connection.h"
#include "client_list.h"
#include "nopoll_helpers.h"
#include "close_retry.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define METADATA_COUNT 					12
#define PARODUS_SERVICE_NAME			"parodus"
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

void *metadataPack;
size_t metaPackSize=-1;


UpStreamMsg *UpStreamMsgQ = NULL;

pthread_mutex_t nano_mut=PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t nano_con=PTHREAD_COND_INITIALIZER;

UpStreamMsg * get_global_UpStreamMsgQ(void)
{
    return UpStreamMsgQ;
}

void set_global_UpStreamMsgQ(UpStreamMsg * UpStreamQ)
{
    UpStreamMsgQ = UpStreamQ;
}

pthread_cond_t *get_global_nano_con(void)
{
    return &nano_con;
}

pthread_mutex_t *get_global_nano_mut(void)
{
    return &nano_mut;
}

/*----------------------------------------------------------------------------*/
/*                             Internal Functions                             */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             External functions                             */
/*----------------------------------------------------------------------------*/

void packMetaData()
{
    char boot_time[256]={'\0'};
    //Pack the metadata initially to reuse for every upstream msg sending to server
    ParodusPrint("-------------- Packing metadata ----------------\n");
    sprintf(boot_time, "%d", get_parodus_cfg()->boot_time);

    struct data meta_pack[METADATA_COUNT] = {
            {HW_MODELNAME, get_parodus_cfg()->hw_model},
            {HW_SERIALNUMBER, get_parodus_cfg()->hw_serial_number},
            {HW_MANUFACTURER, get_parodus_cfg()->hw_manufacturer},
            {HW_DEVICEMAC, get_parodus_cfg()->hw_mac},
            {HW_LAST_REBOOT_REASON, get_parodus_cfg()->hw_last_reboot_reason},
            {FIRMWARE_NAME , get_parodus_cfg()->fw_name},
            {BOOT_TIME, boot_time},
            {LAST_RECONNECT_REASON, get_global_reconnect_reason()},
            {WEBPA_PROTOCOL, get_parodus_cfg()->webpa_protocol},
            {WEBPA_UUID,get_parodus_cfg()->webpa_uuid},
            {WEBPA_INTERFACE, get_parodus_cfg()->webpa_interface_used},
            {PARTNER_ID, get_parodus_cfg()->partner_id}
        };

    const data_t metapack = {METADATA_COUNT, meta_pack};

    metaPackSize = wrp_pack_metadata( &metapack , &metadataPack );

    if (metaPackSize > 0) 
    {
	    ParodusPrint("metadata encoding is successful with size %zu\n", metaPackSize);
    }
    else
    {
	    ParodusError("Failed to encode metadata\n");
    }
}
       
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
        int t = NANO_SOCKET_RCV_TIMEOUT_MS;
        int rc = nn_setsockopt(sock, NN_SOL_SOCKET, NN_RCVTIMEO, &t, sizeof(t));
        if (rc < 0)
        {
            ParodusError ("Unable to set socket receive timeout (errno=%d, %s)\n",errno, strerror(errno));
        }
        ParodusPrint("Nanomsg bind with get_parodus_cfg()->local_url  %s\n", get_parodus_cfg()->local_url);
        bind = nn_bind(sock, get_parodus_cfg()->local_url);
        if(bind < 0)
        {
            ParodusError("Unable to bind socket (errno=%d, %s)\n",errno, strerror(errno));
        }
        else
        {
            ParodusInfo("nanomsg server gone into the listening mode...\n");
            while( FOREVER() ) 
            {
                buf = NULL;
                bytes = nn_recv (sock, &buf, NN_MSG, 0);
                if (g_shutdown)
                  break;
                if (bytes < 0) {
                  if ((errno != EAGAIN) && (errno != ETIMEDOUT))
                    ParodusInfo ("Error (%d) receiving message from nanomsg client\n", errno);
                  continue;
                }
                ParodusInfo ("Upstream message received from nanomsg client\n");
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
	    if(nn_shutdown(sock, bind) < 0)
	    {
	        ParodusError ("nn_shutdown bind socket=%d endpt=%d, err=%d\n", 
		    sock, bind, errno);
	    }
	    if (nn_close (sock) < 0)
	    {
	        ParodusError ("nn_close bind socket=%d err=%d\n", 
		    sock, errno);
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


void *processUpstreamMessage()
{		
    int rv=-1, rc = -1;	
    int msgType;
    wrp_msg_t *msg,*retrieve_msg = NULL;
    void *bytes;
    reg_list_item_t *temp = NULL;
    int matchFlag = 0;
    int status = -1;
	char *device_id = NULL;
	size_t device_id_len = 0;
	size_t parodus_len;
	int ret = -1;

    while(FOREVER())
    {
        pthread_mutex_lock (&nano_mut);
        ParodusPrint("mutex lock in consumer thread\n");
        if(UpStreamMsgQ != NULL)
        {
            UpStreamMsg *message = UpStreamMsgQ;
            UpStreamMsgQ = UpStreamMsgQ->next;
            pthread_mutex_unlock (&nano_mut);
            ParodusPrint("mutex unlock in consumer thread\n");

            /*** Decoding Upstream Msg to check msgType ***/
            /*** For MsgType 9 Perform Nanomsg client Registration else Send to server ***/	
            ParodusPrint("---- Decoding Upstream Msg ----\n");

            rv = wrp_to_struct( message->msg, message->len, WRP_BYTES, &msg );
            if(rv > 0)
            {
                msgType = msg->msg_type;				   
                if(msgType == WRP_MSG_TYPE__SVC_REGISTRATION)
                {
                    ParodusInfo("\n Nanomsg client Registration for Upstream\n");
                    //Extract serviceName and url & store it in a linked list for reg_clients
                    temp = get_global_node();
                    if(get_numOfClients() !=0)
                    {
                        matchFlag = 0;
                        ParodusPrint("matchFlag reset to %d\n", matchFlag);
                        while(temp!=NULL)
                        {
                            if(strcmp(temp->service_name, msg->u.reg.service_name)==0)
                            {
                                ParodusInfo("match found, client is already registered\n");
                                parStrncpy(temp->url,msg->u.reg.url, sizeof(temp->url));
                                if(nn_shutdown(temp->sock, temp->endpoint) < 0)
                                {
                                    ParodusError ("nn_shutdown socket=%d endpt=%d, err=%d\n", 
					temp->sock, temp->endpoint, errno);
                                }
				if (nn_close (temp->sock) < 0)
                                {
                                    ParodusError ("nn_close socket=%d err=%d\n", 
					temp->sock, errno);
                                }

                                temp->sock = nn_socket(AF_SP,NN_PUSH );
                                if(temp->sock >= 0)
                                {					
                                    int t = NANO_SOCKET_SEND_TIMEOUT_MS;
                                    rc = nn_setsockopt(temp->sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
                                    if(rc < 0)
                                    {
                                        ParodusError ("Unable to set socket send timeout (errno=%d, %s)\n",errno, strerror(errno));
                                    }
                                    rc = nn_connect(temp->sock, msg->u.reg.url); 
                                    if(rc < 0)
                                    {
                                        ParodusError ("Unable to connect socket (errno=%d, %s)\n",errno, strerror(errno));
                                    }
                                    else
                                    {
                                        temp->endpoint = rc;
                                        ParodusInfo("Client registered before. Sending ack on socket %d\n", temp->sock); 
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
                    if((matchFlag == 0) || (get_numOfClients() == 0))
                    {
                        ParodusPrint("Adding nanomsg clients to list\n");
                        status = addToList(&msg);
                        ParodusPrint("addToList status is :%d\n", status);
                        if(status == 0)
                        {
                            ParodusPrint("sent auth status to reg client\n");
                        }
                    }
                    release_global_node ();
                }
                else if(msgType == WRP_MSG_TYPE__EVENT)
                {
                    ParodusInfo(" Received upstream event data: dest '%s'\n", msg->u.event.dest);
                    partners_t *partnersList = NULL;
                    int j = 0;

                    int ret = validate_partner_id(msg, &partnersList);
                    if(ret == 1)
                    {
                        wrp_msg_t *eventMsg = (wrp_msg_t *) malloc(sizeof(wrp_msg_t));
                        eventMsg->msg_type = msgType;
                        eventMsg->u.event.content_type=msg->u.event.content_type;
                        eventMsg->u.event.source=msg->u.event.source;
                        eventMsg->u.event.dest=msg->u.event.dest;
                        eventMsg->u.event.payload=msg->u.event.payload;
                        eventMsg->u.event.payload_size=msg->u.event.payload_size;
                        eventMsg->u.event.headers=msg->u.event.headers;
                        eventMsg->u.event.metadata=msg->u.event.metadata;
                        eventMsg->u.event.partner_ids = partnersList;

                        int size = wrp_struct_to( eventMsg, WRP_BYTES, &bytes );
                        if(size > 0)
                        {
                            sendUpstreamMsgToServer(&bytes, size);
                        }
                        free(eventMsg);
                        free(bytes);
                        bytes = NULL;
                    }
                    else
                    {
                        sendUpstreamMsgToServer(&message->msg, message->len);
                    }
                    if(partnersList != NULL)
                    {
                        for(j=0; j<(int)partnersList->count; j++)
                        {
                            if(NULL != partnersList->partner_ids[j])
                            {
                                free(partnersList->partner_ids[j]);
                            }
                        }
                        free(partnersList);
                    }
                    partnersList = NULL;
                }
                else
                {
					//Sending to server for msgTypes 3, 5, 6, 7, 8.
					if( WRP_MSG_TYPE__REQ == msgType )
					{
						ParodusInfo(" Received upstream data with MsgType: %d dest: '%s' transaction_uuid: %s\n", msgType, msg->u.req.dest, msg->u.req.transaction_uuid );
						sendUpstreamMsgToServer(&message->msg, message->len);
					}
					else
					{
						ParodusInfo(" Received upstream data with MsgType: %d dest: '%s' transaction_uuid: %s status: %d\n",msgType, msg->u.crud.dest, msg->u.crud.transaction_uuid, msg->u.crud.status );
						if(WRP_MSG_TYPE__RETREIVE == msgType)
						{
								ret = getDeviceId(&device_id, &device_id_len);
								if(ret == 0)
								{
									ParodusPrint("device_id %s device_id_len %lu\n", device_id, device_id_len);
									/* Match dest based on device_id. Check dest start with: "mac:112233445xxx" ? */
									if( 0 == strncasecmp(device_id, msg->u.crud.dest, device_id_len-1) )
									{
										 /* For this device. */
										parodus_len = strlen( PARODUS_SERVICE_NAME );
										if( 0 == strncmp(PARODUS_SERVICE_NAME, &msg->u.crud.dest[device_id_len], parodus_len-1) ) 
										{
											/* For Parodus CRUD queue. */
											ParodusInfo("Create RetrieveMsg and add to parodus CRUD queue\n");
											createUpstreamRetrieveMsg(msg, &retrieve_msg);
											addCRUDmsgToQueue(retrieve_msg);
										}
										else
										{
											/* For nanomsg clients. */
											getServiceNameAndSendResponse(msg, &message->msg, message->len);
										}
									}
									else
									{
										/* Not for this device. Send upstream */
										ParodusInfo("sendUpstreamMsgToServer \n");
										sendUpstreamMsgToServer(&message->msg, message->len);
									}
									if(device_id != NULL)
									{
										free(device_id);
										device_id = NULL;
									}
								}
								else
								{
									ParodusError("Failed to get device_id\n");
								}
						} else if (WRP_MSG_TYPE__SVC_ALIVE != msgType) {
							sendUpstreamMsgToServer(&message->msg, message->len);
						}
					}
            	}
           	}
            else
            {
                ParodusError("Error in msgpack decoding for upstream\n");
            }

			//nn_freemsg should not be done for parodus/tags/ CRUD requests as it is not received through nanomsg.
			if ((msg && (msg->u.crud.source !=NULL) && wrp_does_service_match("parodus", msg->u.crud.source) == 0))
			{
				free(message->msg);
			}
			else
			{
				if(nn_freemsg (message->msg) < 0)
				{
					ParodusError ("Failed to free msg\n");
				}
			}
			ParodusPrint("Free for upstream decoded msg\n");
			if (msg) {
                wrp_free_struct(msg);
            }
			msg = NULL;
			free(message);
			message = NULL;
        }
        else
        {
            if (g_shutdown) {
                pthread_mutex_unlock (&nano_mut);
                break;
            }
            ParodusPrint("Before pthread cond wait in consumer thread\n");   
            pthread_cond_wait(&nano_con, &nano_mut);
            pthread_mutex_unlock (&nano_mut);
            ParodusPrint("mutex unlock in consumer thread after cond wait\n");
        }
    }
    return NULL;
}

/**
 * @brief getDeviceId function to create deviceId in the format "mac:112233445xxx"
 *
 * @param[out] device_id in the format "mac:112233445xxx"
 * @param[out] total size of device_id
 */
int getDeviceId(char **device_id, size_t *device_id_len)
{
	char *deviceID = NULL;
	size_t len;

	if((get_parodus_cfg()->hw_mac !=NULL) && (strlen(get_parodus_cfg()->hw_mac)!=0))
	{
		len = strlen(get_parodus_cfg()->hw_mac) + 5;

		deviceID = (char *) malloc(sizeof(char)*64);
		if(deviceID != NULL)
		{
			snprintf(deviceID, len, "mac:%s", get_parodus_cfg()->hw_mac);
			*device_id = deviceID;
			*device_id_len = len;
		}
		else
		{
			ParodusError("device_id allocation failed\n");
			return -1;
		}
	}
	else
	{
		ParodusError("device mac is NULL\n");
		return -1;
	}
	return 0;
}


/**
 * @brief createUpstreamRetrieveMsg function to create new message for processing Retrieve requests
 *
 * @param[in] message The upstream message received from cloud or internal clients
 * @param[out] retrieve_msg New message for processing Retrieve requests
 */
void createUpstreamRetrieveMsg(wrp_msg_t *message, wrp_msg_t **retrieve_msg)
{
	wrp_msg_t *msg;
	msg = ( wrp_msg_t * ) malloc( sizeof( wrp_msg_t ) );
	if(msg != NULL)
    {
		memset(msg, 0, sizeof(wrp_msg_t));

		msg->msg_type = message->msg_type;
		if(message->u.crud.transaction_uuid != NULL)
		{
			msg->u.crud.transaction_uuid = strdup(message->u.crud.transaction_uuid);
		}
		if(message->u.crud.source !=NULL)
		{
			msg->u.crud.source = strdup(message->u.crud.source);
		}
		if(message->u.crud.dest != NULL)
		{
			msg->u.crud.dest = strdup(message->u.crud.dest);
		}
		*retrieve_msg = msg;
	}
}

/**
 * @brief getServiceNameAndSendResponse function to fetch client service name and to send msg to it.
 *
 * @param[in] msg The decoded message to fetch client service name from its dest field
 * @param[in] msg_bytes The encoded upstream msg to be sent to client
 * @param[in] msg_size Total size of the msg to send to client
 */
void getServiceNameAndSendResponse(wrp_msg_t *msg, void **msg_bytes, size_t msg_size)
{
	char *serviceName = NULL;
	int sendStatus =-1;

	serviceName = wrp_get_msg_element(WRP_ID_ELEMENT__SERVICE, msg, DEST);
	if ( serviceName != NULL)
	{
		sendStatus=sendMsgtoRegisteredClients(serviceName,(const char **)msg_bytes, msg_size);
		if(sendStatus ==1)
		{
			ParodusInfo("Send upstreamMsg successfully to registered client %s\n", serviceName);
		}
		else
		{
			ParodusError("Failed to send upstreamMsg to registered client %s\n", serviceName);
		}
		free(serviceName);
		serviceName = NULL;
	}
	else
	{
		ParodusError("serviceName is NULL,not sending retrieve response to client\n");
	}
}

void sendUpstreamMsgToServer(void **resp_bytes, size_t resp_size)
{
	void *appendData;
	size_t encodedSize;
	bool close_retry = false;
	//appending response with metadata 			
	if(metaPackSize > 0)
	{
	   	encodedSize = appendEncodedData( &appendData, *resp_bytes, resp_size, metadataPack, metaPackSize );
	   	ParodusPrint("metadata appended upstream response %s\n", (char *)appendData);
	   	ParodusPrint("encodedSize after appending :%zu\n", encodedSize);
	   		   
		ParodusInfo("Sending response to server\n");
		close_retry = get_close_retry();

		/* send response when connection retry is not in progress. Also during cloud_disconnect UPDATE request. Here, close_retry becomes 1 hence check is added to send disconnect response to server. */
		//TODO: Upstream and downstream messages in queue should be handled and queue should be empty before parodus forcefully disconnect from cloud.
		if(!close_retry || (get_parodus_cfg()->cloud_disconnect !=NULL))
		{
			sendMessage(get_global_conn(),appendData, encodedSize);
		}
		else
		{
			ParodusInfo("close_retry is %d, unable to send response as connection retry is in progress\n", close_retry);
		}
		free(appendData);
		appendData =NULL;
	}
	else
	{		
		ParodusError("Failed to send upstream as metadata packing is not successful\n");
	}

}
