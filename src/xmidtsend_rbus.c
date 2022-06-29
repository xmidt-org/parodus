/**
 * Copyright 2022 Comcast Cable Communications Management, LLC
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
 * @file xmidtsend_rbus.c
 *
 * @ To provide Xmidt send RBUS method to send events upstream.
 *
 */

#include <stdlib.h>
#include <rbus.h>
#include "upstream.h"
#include "ParodusInternal.h"
#include "partners_check.h"
#include "xmidtsend_rbus.h"
#include "config.h"
#include "time.h"
#include "heartBeat.h"

static pthread_t processThreadId = 0;
static int XmidtQsize = 0;

XmidtMsg *XmidtMsgQ = NULL;

CloudAck *g_cloudackHead = NULL;

pthread_mutex_t xmidt_mut=PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t xmidt_con=PTHREAD_COND_INITIALIZER;

pthread_mutex_t cloudack_mut=PTHREAD_MUTEX_INITIALIZER;

const char * contentTypeList[]={
"application/json",
"avro/binary",
"application/msgpack",
"application/binary"
};

bool highQosValueCheck(int qos)
{
	if(qos > 24)
	{
		ParodusInfo("The qos value is high\n");
		return true;
	}
	else
	{
		ParodusPrint("The qos value is low\n");
	}

	return false;
}

XmidtMsg * get_global_xmidthead(void)
{
    XmidtMsg *tmp = NULL;
    pthread_mutex_lock (&xmidt_mut);
    tmp = XmidtMsgQ;
    pthread_mutex_unlock (&xmidt_mut);
    return tmp;
}

void set_global_xmidthead(XmidtMsg *new)
{
	pthread_mutex_lock (&xmidt_mut);
	XmidtMsgQ = new;
	pthread_mutex_unlock (&xmidt_mut);
}

CloudAck * get_global_cloud_node(void)
{
    CloudAck * tmp = NULL;
    pthread_mutex_lock (&cloudack_mut);
    tmp = g_cloudackHead;
    pthread_mutex_unlock (&cloudack_mut);
    return tmp;
}

int get_XmidtQsize()
{
	int tmp = 0;
	pthread_mutex_lock (&xmidt_mut);
	tmp = XmidtQsize;
	pthread_mutex_unlock (&xmidt_mut);
	return tmp;
}

void increment_XmidtQsize()
{
	pthread_mutex_lock (&xmidt_mut);
	XmidtQsize++;
	pthread_mutex_unlock (&xmidt_mut);
}

void decrement_XmidtQsize()
{
	pthread_mutex_lock (&xmidt_mut);
	XmidtQsize--;
	pthread_mutex_unlock (&xmidt_mut);
}

int checkCloudConn()
{
	if (!cloud_status_is_online ())
	{
		ParodusInfo("cloud status is not online, wait till connection up\n");
		pthread_mutex_lock(get_global_cloud_status_mut());
		pthread_cond_wait(get_global_cloud_status_cond(), get_global_cloud_status_mut());
		pthread_mutex_unlock(get_global_cloud_status_mut());
		ParodusInfo("Received cloud status signal, proceed to event processing\n");
	}
	return 1;
}
/*
 * @brief To handle xmidt rbus messages received from various components.
 */
void addToXmidtUpstreamQ(wrp_msg_t * msg, rbusMethodAsyncHandle_t asyncHandle)
{
	XmidtMsg *message;
	struct timespec ts;
	char * errorMsg = NULL;

	ParodusPrint("XmidtQsize is %d\n" , get_XmidtQsize());
	if(get_XmidtQsize() == get_parodus_cfg()->max_queue_size)
	{
		mapXmidtStatusToStatusMessage(QUEUE_SIZE_EXCEEDED, &errorMsg);
		ParodusInfo("errorMsg is %s\n",errorMsg);
		createOutParamsandSendAck(msg, asyncHandle, errorMsg , QUEUE_SIZE_EXCEEDED, RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION);
		wrp_free_struct(msg);
		return;
	}

	ParodusPrint ("Add Xmidt Upstream message to queue\n");
	message = (XmidtMsg *)malloc(sizeof(XmidtMsg));

	if(message)
	{
		message->msg = msg;
		message->asyncHandle =asyncHandle;
		message->state = PENDING;
		getCurrentTime(&ts);
		message->enqueueTime = (long long)ts.tv_sec;
		message->sentTime = 0;
		//Increment queue size to handle max queue limit
		increment_XmidtQsize();
		message->next=NULL;
		pthread_mutex_lock (&xmidt_mut);
		//Producer adds the rbus msg into queue
		if(XmidtMsgQ == NULL)
		{
			XmidtMsgQ = message;

			ParodusPrint("Producer added xmidt message\n");
			pthread_cond_signal(&xmidt_con);
			pthread_mutex_unlock (&xmidt_mut);
			ParodusPrint("mutex unlock in xmidt producer\n");
		}
		else
		{
			XmidtMsg *temp = XmidtMsgQ;
			while(temp->next)
			{
				temp = temp->next;
			}
			temp->next = message;
			pthread_mutex_unlock (&xmidt_mut);
		}
	}
	else
	{
		mapXmidtStatusToStatusMessage(ENQUEUE_FAILURE, &errorMsg);
		ParodusInfo("errorMsg is %s\n",errorMsg);
		createOutParamsandSendAck(msg, asyncHandle, errorMsg , ENQUEUE_FAILURE, RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION);
		wrp_free_struct(msg);
	}
	return;
}

//Xmidt consumer thread to process the rbus events.
void processXmidtData()
{
	int err = 0;
	err = pthread_create(&processThreadId, NULL, processXmidtUpstreamMsg, NULL);
	if (err != 0)
	{
		ParodusError("Error creating processXmidtData thread :[%s]\n", strerror(err));
	}
	else
	{
		ParodusInfo("processXmidtData thread created Successfully\n");
	}

}

//Consumer to Parse and process rbus data.
void* processXmidtUpstreamMsg()
{
	int rv = 0;
	long long currTime = 0;
	int ret = 0, status = 0;
	struct timespec ts;
	XmidtMsg *xmidtQ = NULL;
	XmidtMsg *next_node = NULL;

	xmidtQ = get_global_xmidthead();

	while(FOREVER())
	{
		if(get_parodus_init())
		{
			ParodusInfo("Initial cloud connection is not established, Xmidt wait till connection up\n");
			pthread_mutex_lock(get_global_cloud_status_mut());
			pthread_cond_wait(get_global_cloud_status_cond(), get_global_cloud_status_mut());
			pthread_mutex_unlock(get_global_cloud_status_mut());
			ParodusInfo("Received cloud status signal proceed to event processing\n");
		}
		pthread_mutex_lock (&xmidt_mut);
		ParodusInfo("mutex lock in xmidt consumer thread\n");
		if (xmidtQ != NULL)
		{
			XmidtMsg *Data = xmidtQ;
			pthread_mutex_unlock (&xmidt_mut);
			ParodusInfo("mutex unlock in xmidt consumer thread\n");

			checkMsgExpiry();
			checkMaxQandOptimize();

			ParodusInfo("check state\n");
			switch(Data->state)
			{
				case PENDING:
					ParodusInfo("state : PENDING\n");
					//send msg to server only when cloud connection is online.
					if(checkCloudConn())
					{
						ParodusInfo("cloud status is online, processData\n");
						rv = processData(Data, Data->msg, Data->asyncHandle);
						if(!rv)
						{
							ParodusError("processData failed\n");
						}
						else
						{
							ParodusInfo("processData success\n");
						}
					}
					break;

				case SENT:
					ParodusInfo("state : SENT\n");
					getCurrentTime(&ts);
					currTime = (long long)ts.tv_sec;
					ParodusInfo("currTime %d sentTime %lu timeout sec %lu\n",currTime, Data->sentTime, Data->sentTime + CLOUD_ACK_TIMEOUT_SEC);
					if (currTime > (Data->sentTime + CLOUD_ACK_TIMEOUT_SEC))
					{
						ParodusInfo("Check cloud ack for matching transaction id\n");
						ret = checkCloudACK(Data, Data->asyncHandle);
						ParodusInfo("ret is %d\n", ret);
						if (ret)
						{
							ParodusInfo("cloud ack processed successfully\n");
						}
						else //ack timeout case
						{
							ParodusInfo("transaction id match not found, cloud ack timed out. Need to retry\n");
							if (checkCloudConn())
							{
								ParodusInfo("cloud status is online, processData retry\n");
								if(get_pingTimeStamp() > (Data->sentTime + CLOUD_ACK_TIMEOUT_SEC))
								{
									rv = processData(Data, Data->msg, Data->asyncHandle);
									if(!rv)
									{
										ParodusInfo("Data->msg wrp free\n");
										wrp_free_struct(Data->msg);
									}
									else
									{
										ParodusInfo("processData success\n");
									}
								}
							}
						}
					}
					break;
				case DELETE:
					ParodusInfo("state : DELETE\n");
					status = deleteFromXmidtQ(Data->msg, &next_node);
					if(status)
					{
						ParodusInfo("deleteFromXmidtQ done, continue\n");
						xmidtQ = next_node;
						continue;
					}
					else
					{
						ParodusError("deleteFromXmidtQ failed\n");
						break;
					}
			}

			if(xmidtQ !=NULL)
			{
				xmidtQ = xmidtQ->next;
			}

			// circling back to 1st node
			if(xmidtQ == NULL && get_global_xmidthead() != NULL)
			{
				ParodusInfo("xmidtQ is NULL, circling back to 1st node\n");
				xmidtQ = get_global_xmidthead();
			}
			sleep(1);
		}
		else
		{
			if (g_shutdown)
			{
				pthread_mutex_unlock (&xmidt_mut);
				break;
			}
			ParodusInfo("Before cond wait in xmidt consumer thread\n");
			pthread_cond_wait(&xmidt_con, &xmidt_mut);
			pthread_mutex_unlock (&xmidt_mut);
			ParodusInfo("mutex unlock in xmidt thread after cond wait\n");
			xmidtQ = get_global_xmidthead();
		}
	}
	return NULL;
}

//To validate and send events upstream
int processData(XmidtMsg *Datanode, wrp_msg_t * msg, rbusMethodAsyncHandle_t asyncHandle)
{
	int rv = 0;
	char *errorMsg = "none";
	int statuscode =0;

	wrp_msg_t * xmidtMsg = msg;
	if (xmidtMsg == NULL)
	{
		ParodusError("xmidtMsg is NULL\n");
		mapXmidtStatusToStatusMessage(ENQUEUE_FAILURE, &errorMsg);
		ParodusInfo("errorMsg is %s\n",errorMsg);
		createOutParamsandSendAck(xmidtMsg, asyncHandle, errorMsg, ENQUEUE_FAILURE, RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION);
		updateXmidtState(Datanode, DELETE);
		return rv;
	}

	rv = validateXmidtData(xmidtMsg, &errorMsg, &statuscode);
	ParodusPrint("validateXmidtData, errorMsg %s statuscode %d\n", errorMsg, statuscode);
	if(rv)
	{
		ParodusPrint("validation successful, send event to server\n");
		sendXmidtEventToServer(Datanode, xmidtMsg, asyncHandle);
		return rv;
	}
	else
	{
		ParodusError("validation failed, send failure ack\n");
		createOutParamsandSendAck(xmidtMsg, asyncHandle, errorMsg , statuscode, RBUS_ERROR_INVALID_INPUT);
		updateXmidtState(Datanode, DELETE);
	}
	return rv;
}

int validateXmidtData(wrp_msg_t * eventMsg, char **errorMsg, int *statusCode)
{
	if(eventMsg == NULL)
	{
		ParodusError("eventMsg is NULL\n");
		return 0;
	}

	if(eventMsg->msg_type != WRP_MSG_TYPE__EVENT)
	{
		*errorMsg = strdup("Message format is invalid");
		*statusCode = INVALID_MSG_TYPE;
		ParodusError("errorMsg: %s, statusCode: %d\n", *errorMsg, *statusCode);
		return 0;
	}

	if(eventMsg->u.event.source == NULL)
	{
		*errorMsg = strdup("Missing source");
		*statusCode = MISSING_SOURCE;
		ParodusError("errorMsg: %s, statusCode: %d\n", *errorMsg, *statusCode);
		return 0;
	}

	if(eventMsg->u.event.dest == NULL)
	{
		*errorMsg = strdup("Missing dest");
		*statusCode = MISSING_DEST;
		ParodusError("errorMsg: %s, statusCode: %d\n", *errorMsg, *statusCode);
		return 0;
	}

	if(eventMsg->u.event.content_type == NULL)
	{
		*errorMsg = strdup("Missing content_type");
		*statusCode = MISSING_CONTENT_TYPE;
		ParodusError("errorMsg: %s, statusCode: %d\n", *errorMsg, *statusCode);
		return 0;
	}
	else
	{
		int i =0, count = 0, valid = 0;
		count = sizeof(contentTypeList)/sizeof(contentTypeList[0]);
		for(i = 0; i<count; i++)
		{
			if (strcmp(eventMsg->u.event.content_type, contentTypeList[i]) == 0)
			{
				ParodusPrint("content_type is valid %s\n", contentTypeList[i]);
				valid = 1;
				break;
			}
		}
		if (!valid)
		{
			ParodusError("content_type is not valid, %s\n", eventMsg->u.event.content_type);
			*errorMsg = strdup("Invalid content_type");
			*statusCode = INVALID_CONTENT_TYPE;
			ParodusError("errorMsg: %s, statusCode: %d\n", *errorMsg, *statusCode);
			return 0;
		}
	}

	if(eventMsg->u.event.payload == NULL)
	{
		*errorMsg = strdup("Missing payload");
		*statusCode = MISSING_PAYLOAD;
		ParodusError("errorMsg: %s, statusCode: %d\n", *errorMsg, *statusCode);
		return 0;
	}

	if(eventMsg->u.event.payload_size == 0)
	{
		*errorMsg = strdup("Missing payloadlen");
		*statusCode = MISSING_PAYLOADLEN;
		ParodusError("errorMsg: %s, statusCode: %d\n", *errorMsg, *statusCode);
		return 0;
	}

	ParodusPrint("validateXmidtData success. errorMsg %s statusCode %d\n", *errorMsg, *statusCode);
	return 1;
}

void sendXmidtEventToServer(XmidtMsg *msgnode, wrp_msg_t * msg, rbusMethodAsyncHandle_t asyncHandle)
{
	wrp_msg_t *notif_wrp_msg = NULL;
	ssize_t msg_len;
	void *msg_bytes;
	int ret = -1;
	char sourceStr[64] = {'\0'};
	char *device_id = NULL;
	size_t device_id_len = 0;
	int sendRetStatus = 1;
	char *errorMsg = NULL;
	int qos = 0;

        ParodusInfo("MAX_QUEUE_SIZE: %d\n", get_parodus_cfg()->max_queue_size); 
	notif_wrp_msg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
	if(notif_wrp_msg != NULL)
	{
		memset(notif_wrp_msg, 0, sizeof(wrp_msg_t));
		notif_wrp_msg->msg_type = WRP_MSG_TYPE__EVENT;

		ParodusPrint("msg->u.event.source: %s\n",msg->u.event.source);

		if(msg->u.event.source !=NULL)
		{
			//To get device_id in the format "mac:112233445xxx"
			ret = getDeviceId(&device_id, &device_id_len);
			if(ret == 0)
			{
				ParodusPrint("device_id %s device_id_len %lu\n", device_id, device_id_len);
				snprintf(sourceStr, sizeof(sourceStr), "%s/%s", device_id, msg->u.event.source);
				ParodusPrint("sourceStr formed is %s\n" , sourceStr);
				notif_wrp_msg->u.event.source = strdup(sourceStr);
				ParodusInfo("source:%s\n", notif_wrp_msg->u.event.source);
			}
			else
			{
				ParodusError("Failed to get device_id\n");
			}
			if(device_id != NULL)
			{
				free(device_id);
				device_id = NULL;
			}
		}

		if(msg->u.event.dest != NULL)
		{
			notif_wrp_msg->u.event.dest = msg->u.event.dest;
			ParodusInfo("destination: %s\n", notif_wrp_msg->u.event.dest);
		}

		if(msg->u.event.transaction_uuid != NULL)
		{
			notif_wrp_msg->u.event.transaction_uuid = msg->u.event.transaction_uuid;
			ParodusPrint("Notification transaction_uuid %s\n", notif_wrp_msg->u.event.transaction_uuid);
		}

		if(msg->u.event.content_type != NULL)
		{
			notif_wrp_msg->u.event.content_type = msg->u.event.content_type;
			ParodusInfo("content_type is %s\n",notif_wrp_msg->u.event.content_type);
		}

		if(msg->u.event.payload != NULL)
		{
			ParodusInfo("Notification payload: %s\n",msg->u.event.payload);
			notif_wrp_msg->u.event.payload = (void *)msg->u.event.payload;
			notif_wrp_msg->u.event.payload_size = msg->u.event.payload_size;
			ParodusPrint("payload size %lu\n", notif_wrp_msg->u.event.payload_size);
		}

		if(msg->u.event.qos != 0)
		{
			notif_wrp_msg->u.event.qos = msg->u.event.qos;
			qos = notif_wrp_msg->u.event.qos;
			ParodusInfo("Notification qos: %d\n",notif_wrp_msg->u.event.qos);
		}
		msg_len = wrp_struct_to (notif_wrp_msg, WRP_BYTES, &msg_bytes);

		ParodusPrint("Encoded xmidt wrp msg, msg_len %lu\n", msg_len);
		if(msg_len > 0)
		{
			ParodusPrint("sendUpstreamMsgToServer\n");
			sendRetStatus = sendUpstreamMsgToServer(&msg_bytes, msg_len);
		}
		else
		{
			ParodusError("wrp msg_len is zero\n");
			mapXmidtStatusToStatusMessage(WRP_ENCODE_FAILURE, &errorMsg);
			ParodusInfo("errorMsg is %s\n",errorMsg);
			createOutParamsandSendAck(msg, asyncHandle, errorMsg, WRP_ENCODE_FAILURE, RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION);
			updateXmidtState(msgnode, DELETE);
			/*ParodusPrint("wrp_free_struct\n");
			if(notif_wrp_msg != NULL)
			{
				wrp_free_struct(notif_wrp_msg);
			}*/
			if(notif_wrp_msg !=NULL)
			{
				ParodusInfo("notif_wrp_msg->u.event.source free\n");
				if(notif_wrp_msg->u.event.source !=NULL)
				{
					free(notif_wrp_msg->u.event.source);
					notif_wrp_msg->u.event.source = NULL;
				}
				ParodusInfo("notif_wrp_msg free\n");
				free(notif_wrp_msg);
				notif_wrp_msg = NULL;
			}
			if(msg_bytes != NULL)
			{
				ParodusInfo("msg_bytes free\n");
				free(msg_bytes);
				msg_bytes = NULL;
			}
			return;
		}

		while(sendRetStatus)     //If SendMessage is failed condition
		{
			ParodusError("sendXmidtEventToServer is Failed\n");
			if(highQosValueCheck(qos))
			{
				ParodusPrint("The event is having high qos retry again\n");
				ParodusInfo("Wait till connection is Up\n");

				pthread_mutex_lock(get_global_cloud_status_mut());
				pthread_cond_wait(get_global_cloud_status_cond(), get_global_cloud_status_mut());
				pthread_mutex_unlock(get_global_cloud_status_mut());
				ParodusInfo("Received cloud status signal proceed to retry\n");
			}
			else
			{
				mapXmidtStatusToStatusMessage(CLIENT_DISCONNECT, &errorMsg);
				ParodusInfo("errorMsg is %s\n",errorMsg);
				ParodusInfo("The event is having low qos proceed to delete\n");
				createOutParamsandSendAck(msg, asyncHandle, errorMsg, CLIENT_DISCONNECT, RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION);
				updateXmidtState(msgnode, DELETE);
				break;
			}
			sendRetStatus = sendUpstreamMsgToServer(&msg_bytes, msg_len);
		}

		if(sendRetStatus == 0)
		{
			if(highQosValueCheck(qos))
			{
				ParodusInfo("High qos event send success, update state to SENT\n");
				//Update msg status from PENDING to SENT
				updateXmidtState(msgnode, SENT);
				print_xmidMsg_list();
			}
			else
			{
				ParodusInfo("Low qos event, send success callback and delete\n");
				mapXmidtStatusToStatusMessage(DELIVERED_SUCCESS, &errorMsg);
				ParodusInfo("errorMsg is %s\n",errorMsg);
				createOutParamsandSendAck(msg, asyncHandle, errorMsg, DELIVERED_SUCCESS, RBUS_ERROR_SUCCESS);
				updateXmidtState(msgnode, DELETE);
			}
		}

		/*ParodusInfo("B4 notif wrp_free_struct\n");
		if(notif_wrp_msg != NULL)
		{
			wrp_free_struct(notif_wrp_msg);
		}*/

		if(msg_bytes != NULL)
		{
			free(msg_bytes);
			msg_bytes = NULL;
		}
		ParodusInfo("sendXmidtEventToServer done\n");
	}
	else
	{
		mapXmidtStatusToStatusMessage(MSG_PROCESSING_FAILED, &errorMsg);
		ParodusInfo("errorMsg is %s\n",errorMsg);
		ParodusError("Memory allocation failed\n");
		createOutParamsandSendAck(msg, asyncHandle, errorMsg, MSG_PROCESSING_FAILED, RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION);
		updateXmidtState(msgnode, DELETE);
	}

	/*if(msg->u.event.source !=NULL)
	{
		free(msg->u.event.source);
		msg->u.event.source = NULL;
	}*/
	if(notif_wrp_msg !=NULL)
	{
		ParodusInfo("notif_wrp_msg->u.event.source free\n");
		if(notif_wrp_msg->u.event.source !=NULL)
		{
			free(notif_wrp_msg->u.event.source);
			notif_wrp_msg->u.event.source = NULL;
		}
		ParodusInfo("notif_wrp_msg free\n");
		free(notif_wrp_msg);
		notif_wrp_msg = NULL;
	}
	ParodusInfo("sendXmidtEventToServer done\n");
}

void createOutParamsandSendAck(wrp_msg_t *msg, rbusMethodAsyncHandle_t asyncHandle, char *errorMsg, int statuscode, rbusError_t error)
{
	rbusObject_t outParams;
	rbusError_t err;
	rbusValue_t value;
	char qosstring[20] = "";

	rbusValue_Init(&value);
	rbusValue_SetString(value, "event");
	rbusObject_Init(&outParams, NULL);
	rbusObject_SetValue(outParams, "msg_type", value);
	rbusValue_Release(value);

	ParodusInfo("statuscode %d errorMsg %s\n", statuscode, errorMsg);
	rbusValue_Init(&value);
	rbusValue_SetInt32(value, statuscode);
	rbusObject_SetValue(outParams, "status", value);
	rbusValue_Release(value);

	if(errorMsg !=NULL)
	{
		rbusValue_Init(&value);
		rbusValue_SetString(value, errorMsg);
		rbusObject_SetValue(outParams, "error_message", value);
		rbusValue_Release(value);
		free(errorMsg);
	}

	if(msg != NULL)
	{
		if(msg->u.event.source !=NULL)
		{
			ParodusInfo("msg->u.event.source is %s\n", msg->u.event.source);
			rbusValue_Init(&value);
			rbusValue_SetString(value, msg->u.event.source);
			rbusObject_SetValue(outParams, "source", value);
			rbusValue_Release(value);
		}

		if(msg->u.event.dest !=NULL)
		{
			ParodusInfo("msg->u.event.dest is %s\n", msg->u.event.dest);
			rbusValue_Init(&value);
			rbusValue_SetString(value, msg->u.event.dest);
			rbusObject_SetValue(outParams, "dest", value);
			rbusValue_Release(value);
		}

		if(msg->u.event.content_type !=NULL)
		{
			ParodusInfo("msg->u.event.content_type is %s\n", msg->u.event.content_type);
			rbusValue_Init(&value);
			rbusValue_SetString(value, msg->u.event.content_type);
			rbusObject_SetValue(outParams, "content_type", value);
			rbusValue_Release(value);
		}

		rbusValue_Init(&value);
		snprintf(qosstring, sizeof(qosstring), "%d", msg->u.event.qos);
		ParodusInfo("qosstring is %s\n", qosstring);
		rbusValue_SetString(value, qosstring);
		rbusObject_SetValue(outParams, "qos", value);
		rbusValue_Release(value);

		if(msg->u.event.transaction_uuid !=NULL)
		{
			rbusValue_Init(&value);
			rbusValue_SetString(value, msg->u.event.transaction_uuid);
			rbusObject_SetValue(outParams, "transaction_uuid", value);
			rbusValue_Release(value);
			ParodusInfo("outParams msg->u.event.transaction_uuid %s\n", msg->u.event.transaction_uuid);
		}
	}

	if(outParams !=NULL)
	{
		//rbusObject_fwrite(outParams, 1, stdout);
		if(asyncHandle == NULL)
		{
			ParodusError("asyncHandle is NULL\n");
			return;
		}

		ParodusInfo("B4 async send callback\n");
		err = rbusMethod_SendAsyncResponse(asyncHandle, error, outParams);
		
		if(err != RBUS_ERROR_SUCCESS)
		{
			ParodusError("rbusMethod_SendAsyncResponse failed err: %d\n", err);
		}
		else
		{
			ParodusInfo("rbusMethod_SendAsyncResponse success: %d\n", err);
		}

		rbusObject_Release(outParams);
	}
	else
	{
		ParodusError("Failed to create outParams\n");
	}

	return;
}

/*
* @brief This function handles check method call from t2 before the actual inParams SET and to not proceed with inParams processing. 
*/
int checkInputParameters(rbusObject_t inParams)
{
	rbusValue_t check = rbusObject_GetValue(inParams, "check");
	if(check)
	{
		ParodusPrint("Rbus check method. Not proceeding to process this inparam\n");
		return 0;
	}
	return 1;
}

//To generate unique transaction id for xmidt send requests
char* generate_transaction_uuid()
{
	char *transID = NULL;
	uuid_t transaction_Id;
	char *trans_id = NULL;
	trans_id = (char *)malloc(37);
	uuid_generate_random(transaction_Id);
	uuid_unparse(transaction_Id, trans_id);

	if(trans_id !=NULL)
	{
		transID = trans_id;
	}
	return transID;
}

void parseRbusInparamsToWrp(rbusObject_t inParams, char *trans_id, wrp_msg_t **eventMsg)
{
	char *msg_typeStr = NULL;
	char *sourceVal = NULL;
	char *destStr = NULL, *contenttypeStr = NULL;
	char *payloadStr = NULL, *qosVal = NULL;
	unsigned int payloadlength = 0;
	wrp_msg_t *msg = NULL;

	msg = ( wrp_msg_t * ) malloc( sizeof( wrp_msg_t ) );
	if(msg == NULL)
	{
		ParodusError("Wrp msg allocation failed\n");
		return;
	}

	memset( msg, 0, sizeof( wrp_msg_t ) );

	rbusValue_t msg_type = rbusObject_GetValue(inParams, "msg_type");
	if(msg_type)
	{
		if(rbusValue_GetType(msg_type) == RBUS_STRING)
		{
			msg_typeStr = (char *) rbusValue_GetString(msg_type, NULL);
			ParodusPrint("msg_type value received is %s\n", msg_typeStr);
			if(msg_typeStr !=NULL)
			{
				if(strcmp(msg_typeStr, "event") ==0)
				{
					msg->msg_type = WRP_MSG_TYPE__EVENT;
				}
				else
				{
					ParodusError("msg_type received is not event : %s\n", msg_typeStr);
				}
			}
		}
	}
	else
	{
		ParodusError("msg_type is empty\n");
	}

	rbusValue_t source = rbusObject_GetValue(inParams, "source");
	if(source)
	{
		if(rbusValue_GetType(source) == RBUS_STRING)
		{
			sourceVal = (char *)rbusValue_GetString(source, NULL);
			if(sourceVal !=NULL)
			{
				ParodusInfo("source value received is %s\n", sourceVal);
				msg->u.event.source = strdup(sourceVal);
			}
			ParodusPrint("msg->u.event.source is %s\n", msg->u.event.source);
		}
	}
	else
	{
		ParodusError("source is empty\n");
	}

	rbusValue_t dest = rbusObject_GetValue(inParams, "dest");
	if(dest)
	{
		if(rbusValue_GetType(dest) == RBUS_STRING)
		{
			destStr = (char *)rbusValue_GetString(dest, NULL);
			if(destStr !=NULL)
			{
				ParodusPrint("dest value received is %s\n", destStr);
				msg->u.event.dest = strdup(destStr);
				ParodusPrint("msg->u.event.dest is %s\n", msg->u.event.dest);
			}
		}
	}
	else
	{
		ParodusError("dest is empty\n");
	}

	rbusValue_t contenttype = rbusObject_GetValue(inParams, "content_type");
	if(contenttype)
	{
		if(rbusValue_GetType(contenttype) == RBUS_STRING)
		{
			contenttypeStr = (char *)rbusValue_GetString(contenttype, NULL);
			if(contenttypeStr !=NULL)
			{
				ParodusPrint("contenttype value received is %s\n", contenttypeStr);
				msg->u.event.content_type = strdup(contenttypeStr);
				ParodusPrint("msg->u.event.content_type is %s\n", msg->u.event.content_type);
			}
		}
	}
	else
	{
		ParodusError("contenttype is empty\n");
	}

	rbusValue_t payload = rbusObject_GetValue(inParams, "payload");
	if(payload)
	{
		if((rbusValue_GetType(payload) == RBUS_STRING))
		{
			payloadStr = (char *)rbusValue_GetString(payload, NULL);
			if(payloadStr !=NULL)
			{
				ParodusPrint("payload received is %s\n", payloadStr);
				msg->u.event.payload = strdup(payloadStr);
				ParodusPrint("msg->u.event.payload is %s\n", msg->u.event.payload);
			}
			else
			{
				ParodusError("payloadStr is empty\n");
			}
		}
	}
	else
	{
		ParodusError("payload is empty\n");
	}

	rbusValue_t payloadlen = rbusObject_GetValue(inParams, "payloadlen");
	if(payloadlen)
	{
		if(rbusValue_GetType(payloadlen) == RBUS_INT32)
		{
			ParodusPrint("payloadlen type %d RBUS_INT32 %d\n", rbusValue_GetType(payloadlen), RBUS_INT32);
			payloadlength = rbusValue_GetInt32(payloadlen);
			ParodusPrint("payloadlen received is %lu\n", payloadlength);
			msg->u.event.payload_size = (size_t) payloadlength;
			ParodusPrint("msg->u.event.payload_size is %lu\n", msg->u.event.payload_size);
		}
	}
	else
	{
		ParodusError("payloadlen is empty\n");
	}

	ParodusPrint("check qos\n");
	rbusValue_t qos = rbusObject_GetValue(inParams, "qos");
	if(qos)
	{
		if(rbusValue_GetType(qos) == RBUS_STRING)
		{
			ParodusPrint("qos type %d RBUS_STRING %d\n", rbusValue_GetType(qos), RBUS_STRING);
			qosVal = (char *)rbusValue_GetString(qos, NULL);
			ParodusPrint("qos received is %s\n", qosVal);
			if(qosVal !=NULL)
			{
				msg->u.event.qos = atoi(qosVal);
				ParodusPrint("msg->u.event.qos is %d\n", msg->u.event.qos);
			}
		}
	}

	if(trans_id !=NULL)
	{
			ParodusPrint("Add trans_id %s to wrp message\n", trans_id);
			msg->u.event.transaction_uuid = strdup(trans_id);
			free(trans_id);
			trans_id = NULL;
			ParodusPrint("msg->u.event.transaction_uuid is %s\n", msg->u.event.transaction_uuid);
	}
	else
	{
		ParodusError("transaction_uuid is empty\n");
	}

	*eventMsg = msg;
	ParodusPrint("parseRbusInparamsToWrp End\n");
}

static rbusError_t sendDataHandler(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
	(void) handle;
	(void) outParams;
	int inStatus = 0;
	char *transaction_uuid = NULL;
	wrp_msg_t *wrpMsg= NULL;
	ParodusInfo("methodHandler called: %s\n", methodName);
	//printRBUSParams(inParams, INPARAMS_PATH);

	if((methodName !=NULL) && (strcmp(methodName, XMIDT_SEND_METHOD) == 0))
	{
		inStatus = checkInputParameters(inParams);
		if(inStatus)
		{
			//generate transaction id to create outParams and send ack
			//transaction_uuid = generate_transaction_uuid();
			transaction_uuid = strdup("8d72d4c2-1f59-4420-a736-3946083d529a"); //Testing
			ParodusInfo("xmidt transaction_uuid generated is %s\n", transaction_uuid);
			parseRbusInparamsToWrp(inParams, transaction_uuid, &wrpMsg);

			//xmidt send producer
			addToXmidtUpstreamQ(wrpMsg, asyncHandle);
			ParodusInfo("sendDataHandler returned %d\n", RBUS_ERROR_ASYNC_RESPONSE);
			return RBUS_ERROR_ASYNC_RESPONSE;
		}
		else
		{
			ParodusPrint("check method call received, ignoring input\n");
		}
	}
	else
	{
		ParodusError("Method %s received is not supported\n", methodName);
		return RBUS_ERROR_BUS_ERROR;
	}
	ParodusPrint("send RBUS_ERROR_SUCCESS\n");
	return RBUS_ERROR_SUCCESS;
}


int regXmidtSendDataMethod()
{
	int rc = RBUS_ERROR_SUCCESS;
	rbusDataElement_t dataElements[1] = { { XMIDT_SEND_METHOD, RBUS_ELEMENT_TYPE_METHOD, { NULL, NULL, NULL, NULL, NULL, sendDataHandler } } };

	rbusHandle_t rbus_handle = get_parodus_rbus_Handle();

	ParodusPrint("Registering xmidt sendData method %s\n", XMIDT_SEND_METHOD);
	if(!rbus_handle)
	{
		ParodusError("regXmidtSendDataMethod failed as rbus_handle is empty\n");
		return -1;
	}

	rc = rbus_regDataElements(rbus_handle, 1, dataElements);
	
	if(rc != RBUS_ERROR_SUCCESS)
	{
		ParodusError("Register xmidt sendData method failed: %d\n", rc);
	}
	else
	{
		ParodusInfo("Register xmidt sendData method %s success\n", XMIDT_SEND_METHOD);

		//start xmidt queue consumer thread .
		processXmidtData();
	}
	return rc;
}

//To print and store params output to a file
void printRBUSParams(rbusObject_t params, char* file_path)
{
      if( NULL != params )
      {
          FILE *fd = fopen(file_path, "w+");
          rbusObject_fwrite(params, 1, fd);
          fclose(fd);
      }
      else
      {
           ParodusError("Params is NULL\n");
      }
}

/*
 * @brief To store downstream cloud ack messages in a list for further processing.
 */
void addToCloudAckQ(char *trans_id, int qos, int rdr)
{
	CloudAck *ackmsg;

	ParodusInfo ("Add Xmidt downstream message to CloudAck list\n");
	ackmsg = (CloudAck *)malloc(sizeof(CloudAck));

	if(ackmsg)
	{
		ackmsg->transaction_id = strdup(trans_id);
		ackmsg->qos = qos;
		ackmsg->rdr =rdr;
		ParodusInfo("ackmsg->transaction_id %s ackmsg->qos %d ackmsg->rdr %d\n", ackmsg->transaction_id,ackmsg->qos,ackmsg->rdr);
		ackmsg->next=NULL;
		pthread_mutex_lock (&cloudack_mut);
		if(g_cloudackHead == NULL)
		{
			g_cloudackHead = ackmsg;

			ParodusInfo("Producer added cloud ack msg to Q\n");
			pthread_mutex_unlock (&cloudack_mut);
			ParodusInfo("mutex unlock in cloud ack producer\n");
		}
		else
		{
			CloudAck *temp = g_cloudackHead;
			while(temp->next)
			{
				temp = temp->next;
			}
			temp->next = ackmsg;
			pthread_mutex_unlock (&cloudack_mut);
		}
	}
	else
	{
		ParodusError("failure in allocation for cloud ack\n");
	}
	return;
}

//Check cloud ack and send rbus callback to caller based on transaction id.
int checkCloudACK(XmidtMsg *xmdnode, rbusMethodAsyncHandle_t asyncHandle)
{
	wrp_msg_t *xmdMsg;
	char *xmdMsgTransID = NULL;
	char * errorMsg = NULL;
	CloudAck *cloudnode = NULL;

	if(xmdnode != NULL)
	{
		xmdMsg = xmdnode->msg;
		if(xmdMsg != NULL)
		{
			xmdMsgTransID = xmdMsg->u.event.transaction_uuid;
			ParodusInfo("xmdMsgTransID %s\n", xmdMsgTransID);
		}
		else
		{
			ParodusError("xmdMsg is NULL\n");
			return 0;
		}
	}

	cloudnode = get_global_cloud_node();

	while (NULL != cloudnode)
	{
		ParodusInfo("cloudnode->transaction_id %s cloudnode->qos %d cloudnode->rdr %d\n", cloudnode->transaction_id,cloudnode->qos,cloudnode->rdr);
		if(xmdMsgTransID != NULL && cloudnode->transaction_id != NULL)
		{
			if( strcmp(xmdMsgTransID, cloudnode->transaction_id) == 0)
			{
				ParodusInfo("transaction_id %s is matching, send callback\n", xmdMsgTransID);
				mapXmidtStatusToStatusMessage(DELIVERED_SUCCESS, &errorMsg);
				ParodusInfo("errorMsg is %s\n",errorMsg);
				createOutParamsandSendAck(xmdMsg, asyncHandle, errorMsg, DELIVERED_SUCCESS, cloudnode->rdr);
				ParodusInfo("set to DELETE state as cloud ack is processed\n");
				updateXmidtState(xmdnode, DELETE);
				print_xmidMsg_list();
				ParodusInfo("delete cloudACK cloudnode\n");
				deleteCloudACKNode(cloudnode->transaction_id);
				ParodusInfo("checkCloudACK returns success\n");
				return 1;
			}
			else
			{
				ParodusError("transaction_id %s match not found\n", xmdMsgTransID);
			}
		}
		cloudnode= cloudnode->next;
	}
	ParodusError("checkCloudACK returns failure\n");
	return 0;
}

//To update state of the msg node that is currently being processed.
int updateXmidtState(XmidtMsg * temp, int state)
{
	struct timespec ts;
	if(temp == NULL)
	{
		ParodusError("XmidtMsg is NULL, updateXmidtState failed\n");
		return 0;
	}
	else
	{
		ParodusInfo("state to be updated %d\n", state);
		ParodusInfo("node is pointing to temp->state %d\n",temp->state);
		pthread_mutex_lock (&xmidt_mut);
		temp->state = state;
		getCurrentTime(&ts);
		temp->sentTime = (long long)ts.tv_sec;
		ParodusInfo("msgnode is updated with state %d sentTime %lu\n", temp->state, temp->sentTime);
		pthread_mutex_unlock (&xmidt_mut);
		return 1;
	}
}

void print_xmidMsg_list()
{
	XmidtMsg *temp = NULL;
	temp = get_global_xmidthead();
	while (NULL != temp)
	{
		wrp_msg_t *xmdMsg = temp->msg;
		ParodusInfo("node is pointing to xmdMsg transid %s temp->state %d temp->enqueueTime %lu temp->sentTime %lu\n", xmdMsg->u.event.transaction_uuid, temp->state, temp->enqueueTime, temp->sentTime);
		temp= temp->next;
	}
	ParodusInfo("print_xmidMsg_list done\n");
	return;
}

//delete matching cloud ack entry
int deleteCloudACKNode(char* trans_id)
{
	CloudAck *prev_node = NULL, *curr_node = NULL;

	if( NULL == trans_id )
	{
		ParodusError("Invalid value for trans_id\n");
		return 0;
	}
	ParodusInfo("cloud ack to be deleted with trans_id: %s\n", trans_id);

	prev_node = NULL;
	pthread_mutex_lock (&cloudack_mut);
	curr_node = g_cloudackHead ;
	// Traverse to get the msg to be deleted
	while( NULL != curr_node )
	{
		if(strcmp(curr_node->transaction_id, trans_id) == 0)
		{
			ParodusInfo("Found the node to delete\n");
			if( NULL == prev_node )
			{
				ParodusInfo("need to delete first doc\n");
				g_cloudackHead = curr_node->next;
			}
			else
			{
				ParodusInfo("Traversing to find node\n");
				prev_node->next = curr_node->next;

			}

			ParodusInfo("Deleting the node entries\n");
			if(curr_node->transaction_id !=NULL)
			{
				free( curr_node->transaction_id );
				curr_node->transaction_id = NULL;
			}
			if(curr_node != NULL)
			{
				free( curr_node );
			}
			curr_node = NULL;
			ParodusInfo("Deleted successfully and returning..\n");
			pthread_mutex_unlock (&cloudack_mut);
			return 1;
		}

		prev_node = curr_node;
		curr_node = curr_node->next;
	}
	pthread_mutex_unlock (&cloudack_mut);
	ParodusError("Could not find the entry to delete from list\n");
	return 0;
}

//Delete msg from XmidtQ
int deleteFromXmidtQ(wrp_msg_t *msg, XmidtMsg **next_node)
{
	XmidtMsg *prev_node = NULL, *curr_node = NULL;
	char *transid = NULL;

	if( NULL == msg )
	{
		ParodusError("Invalid xmidt msg\n");
		return 0;
	}

	transid = msg->u.event.transaction_uuid;
	if( NULL == transid )
	{
		ParodusError("Invalid xmidt transid\n");
		return 0;
	}
	ParodusInfo("msg to be deleted with transid %s\n", transid);

	prev_node = NULL;
	pthread_mutex_lock (&xmidt_mut);
	curr_node = XmidtMsgQ ;
	// Traverse to get the msg to be deleted
	while( NULL != curr_node )
	{
		wrp_msg_t * curr_node_msg = curr_node->msg;
		if(curr_node_msg !=NULL && strcmp(curr_node_msg->u.event.transaction_uuid, transid) == 0)
		{
			ParodusInfo("Found the node to delete\n");
			if( NULL == prev_node )
			{
				ParodusInfo("need to delete first doc\n");
				XmidtMsgQ = curr_node->next;
			}
			else
			{
				ParodusInfo("Traversing to find node\n");
				prev_node->next = curr_node->next;
				*next_node = curr_node->next;

			}

			ParodusInfo("Deleting the node entries\n");
			wrp_free_struct( curr_node->msg);
			curr_node->msg = NULL;
			if(curr_node !=NULL)
			{
				free( curr_node );
				curr_node = NULL;
			}
			ParodusInfo("Deleted successfully and returning..\n");
			decrement_XmidtQsize();
			ParodusInfo("XmidtQsize after delete is %d\n", get_XmidtQsize());
			pthread_mutex_unlock (&xmidt_mut);
			return 1;
		}

		prev_node = curr_node;
		curr_node = curr_node->next;
	}
	pthread_mutex_unlock (&xmidt_mut);
	ParodusError("Could not find the entry to delete from list\n");
	return 0;
}

//check if message is expired based on each qos and set to delete state.
void checkMsgExpiry()
{
	long long currTime = 0;
	struct timespec ts;

	XmidtMsg *temp = NULL;
	temp = get_global_xmidthead();

	while(temp != NULL)
	{
		getCurrentTime(&ts);
		currTime= (long long)ts.tv_sec;
		wrp_msg_t * tempMsg = temp->msg;
		ParodusInfo("qos %d currTime %lu enqueueTime %lu\n", tempMsg->u.event.qos, currTime, temp->enqueueTime);

		if(tempMsg->u.event.qos > 74)
		{
			ParodusInfo("Critical Qos, check if expiry of 30 mins reached\n");
			if((currTime - temp->enqueueTime) > CRITICAL_QOS_EXPIRE_TIME)
			{
				ParodusInfo("Critical qos msg, set to DELETE state\n");
				updateXmidtState(temp, DELETE);
			}
		}
		else if (tempMsg->u.event.qos > 49)
		{
			ParodusInfo("High Qos, check if expiry of 25 mins reached\n");
			if((currTime - temp->enqueueTime) > HIGH_QOS_EXPIRE_TIME)
			{
				ParodusInfo("High qos msg, set to DELETE state\n");
				updateXmidtState(temp, DELETE);
			}
		}
		else if (tempMsg->u.event.qos > 24)
		{
			ParodusInfo("Medium Qos, check if expiry of 20 mins reached\n");
			if((currTime - temp->enqueueTime) > MEDIUM_QOS_EXPIRE_TIME)
			{
				ParodusInfo("Medium qos msg, set to DELETE state\n");
				updateXmidtState(temp, DELETE);
			}
		}
		else if (tempMsg->u.event.qos >= 0)
		{
			ParodusInfo("Low Qos, check if expiry of 15 mins reached\n");
			if((currTime - temp->enqueueTime) > LOW_QOS_EXPIRE_TIME)
			{
				ParodusInfo("Low qos msg, set to DELETE state\n");
				updateXmidtState(temp, DELETE);
			}
		}
		else
		{
			ParodusError("Invalid qos\n");
		}
		temp = temp->next;
	}

}

//To delete low qos messages from queue when max queue limit is reached.
void checkMaxQandOptimize()
{
	int qos = 0;

	ParodusInfo("checkMaxQandOptimize . XmidtQsize is %d\n" , get_XmidtQsize());
	if(get_XmidtQsize() == MAX_QUEUE_SIZE)
	{
		ParodusInfo("Max Queue size reached, check and optimize\n");

		//Traverse through XmidtMsgQ list and set low qos msgs to DELETE
		XmidtMsg *temp = NULL;
		temp = get_global_xmidthead();

		while(temp != NULL)
		{
			wrp_msg_t * tempMsg = temp->msg;
			qos = tempMsg->u.event.qos;
			ParodusInfo("qos is %d\n", qos);
			if(highQosValueCheck(qos))
			{
				ParodusInfo("High qos msg, skip delete\n");
			}
			else
			{
				ParodusInfo("Low qos msg, set to DELETE state\n");
				updateXmidtState(temp, DELETE);
			}
			temp = temp->next;
		}
	}
}

//map xmidt status and rdr response to status message
void mapXmidtStatusToStatusMessage(int status, char **message)
{
	char *result = NULL;

	if (status == DELIVERED_SUCCESS)
	{
		result = strdup("Delivered (success)");
	}
	else if (status == INVALID_MSG_TYPE)
	{
		result = strdup("Message format is invalid");
	}
	else if (status == MISSING_SOURCE)
	{
		result = strdup("Missing source");
	}
	else if (status == MISSING_DEST)
	{
		result = strdup("Missing dest");
	}
	else if (status == MISSING_CONTENT_TYPE)
	{
		result = strdup("Missing content_type");
	}
	else if (status == MISSING_PAYLOAD)
	{
		result = strdup("Missing payload");
	}
	else if (status == MISSING_PAYLOADLEN)
	{
		result = strdup("Missing payloadlen");
	}
	else if (status == INVALID_CONTENT_TYPE)
	{
		result = strdup("Invalid content_type");
	}
	else if (status == ENQUEUE_FAILURE)
	{
		result = strdup("Unable to enqueue");
	}
	else if (status == CLIENT_DISCONNECT)
	{
		result = strdup("send failed due to client disconnect");
	}
	else if (status == QUEUE_SIZE_EXCEEDED)
	{
		result = strdup("Max Queue Size Exceeded");
	}
	else if (status == WRP_ENCODE_FAILURE)
	{
		result = strdup("Wrp message encoding failed");
	}
	else if (status == MSG_PROCESSING_FAILED)
	{
		result = strdup("Memory allocation failed");
	}
	else
	{
		result = strdup("Unknown Error");
	}
	*message = result;
}
