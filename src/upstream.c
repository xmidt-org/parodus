/**
 * @file upstream.c
 *
 * @description This describes functions required to manage upstream messages.
 *
 * Copyright (c) 2015  Comcast
 */

#include "ParodusInternal.h"
#include "upstream.h"
#include "config.h"
#include "connection.h"
#include "client_list.h"
#include "nopoll_helpers.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define METADATA_COUNT 					11
#define PARODUS_UPSTREAM                "tcp://127.0.0.1:6666"
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

void *metadataPack;
size_t metaPackSize=-1;
char parodus_url[32] ={'\0'};

UpStreamMsg *UpStreamMsgQ = NULL;

pthread_mutex_t nano_mut=PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t nano_con=PTHREAD_COND_INITIALIZER;

/*----------------------------------------------------------------------------*/
/*                             Internal Functions                             */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             External functions                             */
/*----------------------------------------------------------------------------*/

void getParodusUrl()
{
    const char *parodusIp = NULL;
    const char * envParodus = getenv ("PARODUS_SERVICE_URL");

    if( envParodus != NULL)
    {
      parodusIp = envParodus;
    }
    else
    {
      parodusIp = PARODUS_UPSTREAM ;
    }

    snprintf(parodus_url,sizeof(parodus_url),"%s", parodusIp);
    ParodusInfo("formatted parodus Url %s\n",parodus_url);

}

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
            {WEBPA_INTERFACE, get_parodus_cfg()->webpa_interface_used}
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
        bind = nn_bind(sock, parodus_url);
        if(bind < 0)
        {
            ParodusError("Unable to bind socket (errno=%d, %s)\n",errno, strerror(errno));
        }
        else
        {
            while( FOREVER() ) 
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


void *processUpstreamMessage()
{		
    int rv=-1, rc = -1;	
    int msgType;
    wrp_msg_t *msg;	
    void *appendData;
    size_t encodedSize;
    reg_list_item_t *temp = NULL;
    int matchFlag = 0;
    int status = -1;

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
                if(msgType == 9)
                {
                    ParodusInfo("\n Nanomsg client Registration for Upstream\n");
                    //Extract serviceName and url & store it in a linked list for reg_clients
                    if(get_numOfClients() !=0)
                    {
                        temp = get_global_node();
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
                        sendMessage(get_global_conn(),appendData, encodedSize);

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
	   	sendMessage(get_global_conn(),appendData, encodedSize);
	   	
		free(appendData);
		appendData =NULL;
	}
	else
	{		
		ParodusError("Failed to send upstream as metadata packing is not successful\n");
	}

}

