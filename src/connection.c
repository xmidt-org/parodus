#include "connection.h"


char deviceMAC[32]={'\0'}; 

void listenerOnMessage_queue(noPollCtx * ctx, noPollConn * conn, noPollMsg * msg,noPollPtr user_data);
void listenerOnPingMessage (noPollCtx * ctx, noPollConn * conn, noPollMsg * msg, noPollPtr user_data);
void listenerOnCloseMessage (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data);


#define MAX_SEND_SIZE (60 * 1024)

#define FLUSH_WAIT_TIME (2000000LL)


int sendResponse(noPollConn * conn, void * buffer, size_t length)
{
    char *cp = buffer;
    int final_len_sent = 0;
    noPollOpCode frame_type = NOPOLL_BINARY_FRAME;

    while (length > 0) {
            int bytes_sent, len_to_send;

            len_to_send = length > MAX_SEND_SIZE ? MAX_SEND_SIZE : length;
            length -= len_to_send;
            bytes_sent = __nopoll_conn_send_common(conn, cp, len_to_send,
                            length > 0 ? nopoll_false : nopoll_true,
                            0, frame_type);

            if (bytes_sent != len_to_send) {
                if (-1 == bytes_sent ||
                   (bytes_sent = nopoll_conn_flush_writes(conn, FLUSH_WAIT_TIME, bytes_sent)) != len_to_send)
                {
                        PARODUS_DEBUG("sendResponse() Failed to send all the data\n");
                        cp = NULL;
                        break;
                }
            }
            cp += len_to_send;
	    final_len_sent += len_to_send;
            frame_type = NOPOLL_CONTINUATION_FRAME;
    }

    return final_len_sent;
}


/**
 * @brief createNopollConnection interface to create WebSocket client connections.
 *Loads the WebPA config file and creates the intial connection and manages the connection wait, close mechanisms.
 */
char createNopollConnection(noPollCtx *ctx)
{
	bool initial_retry = false;
	int backoffRetryTime = 0;
	int max_retry_sleep;
	
	char device_id[32]={'\0'};
	char user_agent[512]={'\0'};
	const char * headerNames[HTTP_CUSTOM_HEADER_COUNT] = {"X-WebPA-Device-Name","X-WebPA-Device-Protocols","User-Agent", "X-WebPA-Convey"};
	const char * headerValues[HTTP_CUSTOM_HEADER_COUNT];
        int headerCount = HTTP_CUSTOM_HEADER_COUNT; /* Invalid X-Webpa-Convey header Bug # WEBPA-787 */
	char port[8];
	noPollConnOpts * opts;
	char server_Address[256];
	char redirectURL[128]={'\0'};
	char *temp_ptr;

	cJSON *response = cJSON_CreateObject();
	char *buffer = NULL, *firmwareVersion = NULL, *modelName = NULL, *manufacturer = NULL, *protocol =NULL;	
	char *reboot_reason = NULL;
	char encodedData[1024];
	int  encodedDataSize = 1024;
	int i=0, j=0, connErr=0;
	int bootTime_sec;
	char boot_time[256]={'\0'};
	//Retry Backoff count shall start at c=2 & calculate 2^c - 1.
	int c=2;
       

	FILE *fp;
	fp = fopen("/tmp/parodus_ready", "r");

	if (fp!=NULL)
	{
		unlink("/tmp/parodus_ready");
		PARODUS_DEBUG("Closing Parodus_Ready FIle \n");
		fclose(fp);
	}
		
	get_parodus_cfg()->secureFlag = 1;
	//struct timespec start,end,connErr_start,connErr_end,*startPtr,*endPtr,*connErr_startPtr,*connErr_endPtr;
	struct timespec connErr_start,connErr_end,*connErr_startPtr,*connErr_endPtr;
	//startPtr = &start;
	//endPtr = &end;
	connErr_startPtr = &connErr_start;
	connErr_endPtr = &connErr_end;
	strcpy(deviceMAC, get_parodus_cfg()->hw_mac);
	snprintf(device_id, sizeof(device_id), "mac:%s", deviceMAC);
	PARODUS_INFO("Device_id %s\n",device_id);

	headerValues[0] = device_id;
	headerValues[1] = "wrp-0.11,getset-0.1";    
	
	
	bootTime_sec = get_parodus_cfg()->boot_time;
	PARODUS_DEBUG("BootTime In sec: %d\n", bootTime_sec);
	firmwareVersion = get_parodus_cfg()->fw_name;
    modelName = get_parodus_cfg()->hw_model;
    manufacturer = get_parodus_cfg()->hw_manufacturer;
	protocol = get_parodus_cfg()->webpa_protocol;
    PARODUS_DEBUG("webpa_protocol is %s\n", protocol);
    	snprintf(user_agent, sizeof(user_agent),
             "%s (%s; %s/%s;)",
             ((0 != strlen(protocol)) ? protocol : "unknown"),
             ((0 != strlen(firmwareVersion)) ? firmwareVersion : "unknown"),
             ((0 != strlen(modelName)) ? modelName : "unknown"),
             ((0 != strlen(manufacturer)) ? manufacturer : "unknown"));

	PARODUS_INFO("User-Agent: %s\n",user_agent);
	headerValues[2] = user_agent;
	
		
	PARODUS_INFO("Received reconnect_reason as:%s\n", reconnect_reason);
	reboot_reason = get_parodus_cfg()->hw_last_reboot_reason;
	PARODUS_INFO("Received reboot_reason as:%s\n", reboot_reason);
	
	if(strlen(modelName)!=0)
	{
		cJSON_AddStringToObject(response, HW_MODELNAME, modelName);
	}
	
	if(strlen(get_parodus_cfg()->hw_serial_number)!=0)
	{
		cJSON_AddStringToObject(response, HW_SERIALNUMBER, get_parodus_cfg()->hw_serial_number);
	}
	
	if(strlen(manufacturer)!=0)
	{
		cJSON_AddStringToObject(response, HW_MANUFACTURER, manufacturer);
	}
	
	if(strlen(firmwareVersion)!=0)
	{
		cJSON_AddStringToObject(response, FIRMWARE_NAME, firmwareVersion);
	}
	
	cJSON_AddNumberToObject(response, BOOT_TIME, bootTime_sec);
	cJSON_AddStringToObject(response, WEBPA_PROTOCOL, protocol);
	
	if(strlen(get_parodus_cfg()->webpa_interface_used)!=0)
	{
		cJSON_AddStringToObject(response, WEBPA_INTERFACE, get_parodus_cfg()->webpa_interface_used);
	}	
	
	if(strlen(reboot_reason)!=0)
	{						
		cJSON_AddStringToObject(response, HW_LAST_REBOOT_REASON, reboot_reason);			
	}
	else
	{
		PARODUS_ERROR("Failed to GET Reboot reason value\n");
	}
	
	if(reconnect_reason !=NULL)
	{			
	    cJSON_AddStringToObject(response, LAST_RECONNECT_REASON, reconnect_reason);			
	}
	else
	{
	     	PARODUS_ERROR("Failed to GET Reconnect reason value\n");
	}
		
	buffer = cJSON_PrintUnformatted(response);
	PARODUS_INFO("X-WebPA-Convey Header: [%zd]%s\n", strlen(buffer), buffer);

	if(nopoll_base64_encode (buffer, strlen(buffer), encodedData, &encodedDataSize) != nopoll_true)
	{
		PARODUS_ERROR("Base64 Encoding failed for Connection Header\n");
		headerValues[3] = ""; 
                headerCount -= 1; 
	}
	else
	{
		/* Remove \n characters from the base64 encoded data */
		for(i=0;encodedData[i] != '\0';i++)
		{
			if(encodedData[i] == '\n')
			{
				PARODUS_DEBUG("New line is present in encoded data at position %d\n",i);
			}
			else
			{
				encodedData[j] = encodedData[i];
				j++;
			}
		}
		encodedData[j]='\0';
		headerValues[3] = encodedData;
		PARODUS_DEBUG("Encoded X-WebPA-Convey Header: [%zd]%s\n", strlen(encodedData), encodedData);
	}	
	
	cJSON_Delete(response);
	
	
	snprintf(port,sizeof(port),"%d",8080);
	parStrncpy(server_Address, get_parodus_cfg()->webpa_url, sizeof(server_Address));
	PARODUS_INFO("server_Address %s\n",server_Address);
	
					
	max_retry_sleep = (int) pow(2, get_parodus_cfg()->webpa_backoff_max) -1;
	PARODUS_DEBUG("max_retry_sleep is %d\n", max_retry_sleep );
	
	do
	{
		
		//calculate backoffRetryTime and to perform exponential increment during retry
		
		if(backoffRetryTime < max_retry_sleep)
		{
			backoffRetryTime = (int) pow(2, c) -1;
		}
		PARODUS_DEBUG("New backoffRetryTime value calculated as %d seconds\n", backoffRetryTime);
								
                noPollConn *connection;
		if(get_parodus_cfg()->secureFlag) 
		{                    
		    PARODUS_DEBUG("secure true\n");
			/* disable verification */
			opts = nopoll_conn_opts_new ();
			nopoll_conn_opts_ssl_peer_verify (opts, nopoll_false);
			nopoll_conn_opts_set_ssl_protocol (opts, NOPOLL_METHOD_TLSV1_2); 
			connection = nopoll_conn_tls_new(ctx, opts, server_Address, port, NULL,
                               "/api/v2/device", NULL, NULL, get_parodus_cfg()->webpa_interface_used,
                                headerNames, headerValues, headerCount);// WEBPA-787
		}
		else 
		{
		    PARODUS_DEBUG("secure false\n");
                    connection = nopoll_conn_new(ctx, server_Address, port, NULL,
                               "/api/v2/device", NULL, NULL, get_parodus_cfg()->webpa_interface_used,
                                headerNames, headerValues, headerCount);// WEBPA-787
		}
                set_global_conn(connection);

		if(get_global_conn() != NULL)
		{
			if(!nopoll_conn_is_ok(get_global_conn())) 
			{
				PARODUS_ERROR("Error connecting to server\n");
				PARODUS_ERROR("RDK-10037 - WebPA Connection Lost\n");
				// Copy the server address from config to avoid retrying to the same failing talaria redirected node
				parStrncpy(server_Address, get_parodus_cfg()->webpa_url, sizeof(server_Address));
				close_and_unref_connection(get_global_conn());
				set_global_conn(NULL);
				initial_retry = true;
				
				PARODUS_INFO("Waiting with backoffRetryTime %d seconds\n", backoffRetryTime);
				sleep(backoffRetryTime);
				continue;
			}
			else 
			{
				PARODUS_DEBUG("Connected to Server but not yet ready\n");
				initial_retry = false;
						
				//reset backoffRetryTime back to the starting value, as next reason can be different					
				c = 2;
				backoffRetryTime = (int) pow(2, c) -1;
						
									
			}

			if(!nopoll_conn_wait_until_connection_ready(get_global_conn(), 10, redirectURL)) 
			{
				
				if (strncmp(redirectURL, "Redirect:", 9) == 0) // only when there is a http redirect
				{
					PARODUS_ERROR("Received temporary redirection response message %s\n", redirectURL);
					// Extract server Address and port from the redirectURL
					temp_ptr = strtok(redirectURL , ":"); //skip Redirect 
					temp_ptr = strtok(NULL , ":"); // skip https
					temp_ptr = strtok(NULL , ":");
					parStrncpy(server_Address, temp_ptr+2, sizeof(server_Address));
					parStrncpy(port, strtok(NULL , "/"), sizeof(port));
					PARODUS_INFO("Trying to Connect to new Redirected server : %s with port : %s\n", server_Address, port);
					
					//reset c=2 to start backoffRetryTime as retrying using new redirect server
					c = 2;
					
				}
				else
				{
					PARODUS_ERROR("Client connection timeout\n");	
					PARODUS_ERROR("RDK-10037 - WebPA Connection Lost\n");
					// Copy the server address from config to avoid retrying to the same failing talaria redirected node
					parStrncpy(server_Address, get_parodus_cfg()->webpa_url, sizeof(server_Address));
					PARODUS_INFO("Waiting with backoffRetryTime %d seconds\n", backoffRetryTime);
					sleep(backoffRetryTime);
					c++;
				}
				close_and_unref_connection(get_global_conn());
				set_global_conn(NULL);
				initial_retry = true;
				
			}
			else 
			{
				initial_retry = false;				
				PARODUS_INFO("Connection is ready\n");
			}
		}
		else
		{
			
			/* If the connect error is due to DNS resolving to 10.0.0.1 then start timer.
			 * Timeout after 15 minutes if the error repeats continuously and kill itself. 
			 */
			if((checkHostIp(server_Address) == -2)) 	
			{
				if(connErr == 0)
				{
					getCurrentTime(connErr_startPtr);
					connErr = 1;
					PARODUS_INFO("First connect error occurred, initialized the connect error timer\n");
				}
				else
				{
					getCurrentTime(connErr_endPtr);
					PARODUS_DEBUG("checking timeout difference:%ld\n", timeValDiff(connErr_startPtr, connErr_endPtr));
					if(timeValDiff(connErr_startPtr, connErr_endPtr) >= (15*60*1000))
					{
						PARODUS_ERROR("WebPA unable to connect due to DNS resolving to 10.0.0.1 for over 15 minutes; crashing service.\n");
						reconnect_reason = "Dns_Res_webpa_reconnect";
						LastReasonStatus = true;
						
						kill(getpid(),SIGTERM);						
					}
				}			
			}
			initial_retry = true;
			PARODUS_INFO("Waiting with backoffRetryTime %d seconds\n", backoffRetryTime);
			sleep(backoffRetryTime);
			c++;
			// Copy the server address from config to avoid retrying to the same failing talaria redirected node
			parStrncpy(server_Address, get_parodus_cfg()->webpa_url, sizeof(server_Address));
		}
				
	}while(initial_retry);
	
	if(get_parodus_cfg()->secureFlag) 
	{
		PARODUS_INFO("Connected to server over SSL\n");
	}
	else 
	{
		PARODUS_INFO("Connected to server\n");
	}
	
	//creating tmp file to signal parodus_ready status once connection is successful
	fp = fopen("/tmp/parodus_ready", "w");
	fflush(fp);
	fclose(fp);
	
	// Reset close_retry flag and heartbeatTimer once the connection retry is successful
	PARODUS_DEBUG("createNopollConnection(): close_mut lock\n");
	pthread_mutex_lock (&close_mut);
	close_retry = false;
	pthread_mutex_unlock (&close_mut);
	PARODUS_DEBUG("createNopollConnection(): close_mut unlock\n");
	heartBeatTimer = 0;
	
	
	//Pack the metadata initially to reuse for every upstream msg sending to server
  	PARODUS_DEBUG("-------------- Packing metadata ----------------\n");
  	sprintf(boot_time, "%d", bootTime_sec);
 
 	
  	
	struct data meta_pack[METADATA_COUNT] = {
            {HW_MODELNAME, modelName},
            {HW_SERIALNUMBER, get_parodus_cfg()->hw_serial_number},
            {HW_MANUFACTURER, manufacturer},
            {HW_DEVICEMAC, get_parodus_cfg()->hw_mac},
            {HW_LAST_REBOOT_REASON, reboot_reason},
            {FIRMWARE_NAME , firmwareVersion},
            {BOOT_TIME, boot_time},
            {LAST_RECONNECT_REASON, reconnect_reason},
            {WEBPA_PROTOCOL, protocol},
            {WEBPA_UUID,get_parodus_cfg()->webpa_uuid},
            {WEBPA_INTERFACE, get_parodus_cfg()->webpa_interface_used}
        };
	
	const data_t metapack = {METADATA_COUNT, meta_pack};
	
	metaPackSize = wrp_pack_metadata( &metapack , &metadataPack );

	if (metaPackSize > 0) 
	{
		PARODUS_DEBUG("metadata encoding is successful with size %zu\n", metaPackSize);
	}
	else
	{
		PARODUS_ERROR("Failed to encode metadata\n");

	}
	

	// Reset connErr flag on successful connection
	connErr = 0;
	
	reconnect_reason = "webpa_process_starts";
	LastReasonStatus =false;
	PARODUS_DEBUG("LastReasonStatus reset after successful connection\n");

	nopoll_conn_set_on_msg(get_global_conn(), (noPollOnMessageHandler) listenerOnMessage_queue, NULL);
	nopoll_conn_set_on_ping_msg(get_global_conn(), (noPollOnMessageHandler)listenerOnPingMessage, NULL);
	nopoll_conn_set_on_close(get_global_conn(), (noPollOnCloseHandler)listenerOnCloseMessage, NULL);
	return nopoll_true;
}


/**
 * @brief listenerOnMessage_queue function to add messages to the queue
 *
 * @param[in] ctx The context where the connection happens.
 * @param[in] conn The Websocket connection object
 * @param[in] msg The message received from server for various process requests
 * @param[out] user_data data which is to be sent
 */

void listenerOnMessage_queue(noPollCtx * ctx, noPollConn * conn, noPollMsg * msg,noPollPtr user_data)
{

	UNUSED(ctx);
	UNUSED(conn);
	UNUSED(user_data);

	ParodusMsg *message;

	if (terminated) 
	{
		return;
	}

	message = (ParodusMsg *)malloc(sizeof(ParodusMsg));

	if(message)
	{

		message->msg = msg;
		message->payload = (void *)nopoll_msg_get_payload (msg);
		message->len = nopoll_msg_get_payload_size (msg);
		message->next = NULL;

	
		nopoll_msg_ref(msg);
		
		pthread_mutex_lock (&g_mutex);		
		PARODUS_DEBUG("mutex lock in producer thread\n");
		
		if(ParodusMsgQ == NULL)
		{
			ParodusMsgQ = message;
			PARODUS_DEBUG("Producer added message\n");
		 	pthread_cond_signal(&g_cond);
			pthread_mutex_unlock (&g_mutex);
			PARODUS_DEBUG("mutex unlock in producer thread\n");
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
		PARODUS_ERROR("Memory allocation is failed\n");
	}
	PARODUS_DEBUG("*****Returned from listenerOnMessage_queue*****\n");
}

/**
 * @brief listenerOnPingMessage function to create WebSocket listener to receive heartbeat ping messages
 *
 * @param[in] ctx The context where the connection happens.
 * @param[in] conn Websocket connection object
 * @param[in] msg The ping message received from the server
 * @param[out] user_data data which is to be sent
 */
void listenerOnPingMessage (noPollCtx * ctx, noPollConn * conn, noPollMsg * msg, noPollPtr user_data)
{

	UNUSED(ctx);
	UNUSED(user_data);

    noPollPtr payload = NULL;
	payload = (noPollPtr ) nopoll_msg_get_payload(msg);

	if ((payload!=NULL) && !terminated) 
	{
		PARODUS_INFO("Ping received with payload %s, opcode %d\n",(char *)payload, nopoll_msg_opcode(msg));
		if (nopoll_msg_opcode(msg) == NOPOLL_PING_FRAME) 
		{
			nopoll_conn_send_frame (conn, nopoll_true, nopoll_true, NOPOLL_PONG_FRAME, strlen(payload), payload, 0);
			heartBeatTimer = 0;
			PARODUS_DEBUG("Sent Pong frame and reset HeartBeat Timer\n");
		}
	}
}

void listenerOnCloseMessage (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data)
{
	

	UNUSED(ctx);
	UNUSED(conn);
	
	PARODUS_DEBUG("listenerOnCloseMessage(): mutex lock in producer thread\n");
	
	if((user_data != NULL) && (strstr(user_data, "SSL_Socket_Close") != NULL) && !LastReasonStatus)
	{
		reconnect_reason = "Server_closed_connection";
		LastReasonStatus = true;
		
	}
	else if ((user_data == NULL) && !LastReasonStatus)
	{
		reconnect_reason = "Unknown";
	}

	pthread_mutex_lock (&close_mut);
	close_retry = true;
	pthread_mutex_unlock (&close_mut);
	PARODUS_DEBUG("listenerOnCloseMessage(): mutex unlock in producer thread\n");

}

