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

static pthread_t processThreadId = 0;
static int XmidtQsize = 0;

XmidtMsg *XmidtMsgQ = NULL;

pthread_mutex_t xmidt_mut=PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t xmidt_con=PTHREAD_COND_INITIALIZER;

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
/*
 * @brief To handle xmidt rbus messages received from various components.
 */
void addToXmidtUpstreamQ(wrp_msg_t * msg, rbusMethodAsyncHandle_t asyncHandle)
{
	XmidtMsg *message;

	ParodusPrint("XmidtQsize is %d\n" , XmidtQsize);
	if(XmidtQsize == MAX_QUEUE_SIZE)
	{
		char * errorMsg = strdup("Max Queue Size Exceeded");
		ParodusError("Queue Size Exceeded\n");
		createOutParamsandSendAck(msg, asyncHandle, errorMsg , QUEUE_SIZE_EXCEEDED);
		wrp_free_struct(msg);
		return;
	}

	ParodusPrint ("Add Xmidt Upstream message to queue\n");
	message = (XmidtMsg *)malloc(sizeof(XmidtMsg));

	if(message)
	{
		message->msg = msg;
		message->asyncHandle =asyncHandle;
		//Increment queue size to handle max queue limit
		XmidtQsize++;
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
		char * errorMsg = strdup("Unable to enqueue");
		ParodusError("failure in allocation for xmidt message\n");
		createOutParamsandSendAck(msg, asyncHandle, errorMsg , ENQUEUE_FAILURE);
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
	while(FOREVER())
	{
		pthread_mutex_lock (&xmidt_mut);
		ParodusPrint("mutex lock in xmidt consumer thread\n");
		if(XmidtMsgQ != NULL)
		{
			XmidtMsg *Data = XmidtMsgQ;
			
			pthread_mutex_unlock (&xmidt_mut);
			ParodusPrint("mutex unlock in xmidt consumer thread\n");
			rv = processData(Data->msg, Data->asyncHandle);
			if(!rv)
			{
				ParodusPrint("Data->msg wrp free\n");
				wrp_free_struct(Data->msg);
			}
			else
			{
				free(Data->msg);
			}
			free(Data);
			Data = NULL;
		}
		else
		{
			if (g_shutdown)
			{
				pthread_mutex_unlock (&xmidt_mut);
				break;
			}
			ParodusPrint("Before cond wait in xmidt consumer thread\n");
			pthread_cond_wait(&xmidt_con, &xmidt_mut);
			pthread_mutex_unlock (&xmidt_mut);
			ParodusPrint("mutex unlock in xmidt thread after cond wait\n");
		}
	}
	return NULL;
}

//To validate and send events upstream
int processData(wrp_msg_t * msg, rbusMethodAsyncHandle_t asyncHandle)
{
	int rv = 0;
	char *errorMsg = "none";
	int statuscode =0;

	wrp_msg_t * xmidtMsg = msg;
	if (xmidtMsg == NULL)
	{
		ParodusError("xmidtMsg is NULL\n");
		errorMsg = strdup("Unable to enqueue");
		createOutParamsandSendAck(xmidtMsg, asyncHandle, errorMsg, ENQUEUE_FAILURE);
		xmidtQDequeue();
		return rv;
	}

	rv = validateXmidtData(xmidtMsg, &errorMsg, &statuscode);
	ParodusPrint("validateXmidtData, errorMsg %s statuscode %d\n", errorMsg, statuscode);
	if(rv)
	{
		ParodusPrint("validation successful, send event to server\n");
		sendXmidtEventToServer(xmidtMsg, asyncHandle);
		return rv;
	}
	else
	{
		ParodusError("validation failed, send failure ack\n");
		createOutParamsandSendAck(xmidtMsg, asyncHandle, errorMsg , statuscode);
		xmidtQDequeue();
	}
	return rv;
}

//To remove an event from Queue
void xmidtQDequeue()
{
	pthread_mutex_lock (&xmidt_mut);
	if(XmidtMsgQ != NULL)
	{
		XmidtMsgQ = XmidtMsgQ->next;
		XmidtQsize -= 1;
	}
	else
	{
		ParodusError("XmidtMsgQ is NULL\n");
	}
	pthread_mutex_unlock (&xmidt_mut);
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

void sendXmidtEventToServer(wrp_msg_t * msg, rbusMethodAsyncHandle_t asyncHandle)
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
			if(strcmp(msg->u.event.content_type , "JSON") == 0)
			{
				notif_wrp_msg->u.event.content_type = strdup("application/json");
			}
			ParodusPrint("content_type is %s\n",notif_wrp_msg->u.event.content_type);
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
			errorMsg = strdup("Wrp message encoding failed");
			createOutParamsandSendAck(msg, asyncHandle, errorMsg, WRP_ENCODE_FAILURE);
			xmidtQDequeue();

			ParodusPrint("wrp_free_struct\n");
			if(notif_wrp_msg != NULL)
			{
				wrp_free_struct(notif_wrp_msg);
			}

			if(msg_bytes != NULL)
			{
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
				errorMsg = strdup("send failed due to client disconnect");
				ParodusInfo("The event is having low qos proceed to dequeue\n");
				createOutParamsandSendAck(msg, asyncHandle, errorMsg, CLIENT_DISCONNECT);
				xmidtQDequeue();
				break;
			}
			sendRetStatus = sendUpstreamMsgToServer(&msg_bytes, msg_len);
		}

		if(sendRetStatus == 0)
		{
			errorMsg = strdup("send to server success");
			createOutParamsandSendAck(msg, asyncHandle, errorMsg, DELIVERED_SUCCESS);
			xmidtQDequeue();
		}

		ParodusPrint("B4 notif wrp_free_struct\n");
		if(notif_wrp_msg != NULL)
		{
			wrp_free_struct(notif_wrp_msg);
		}

		if(msg_bytes != NULL)
		{
			free(msg_bytes);
			msg_bytes = NULL;
		}

	}
	else
	{
		errorMsg = strdup("Memory allocation failed");
		ParodusError("Memory allocation failed\n");
		createOutParamsandSendAck(msg, asyncHandle, errorMsg, MSG_PROCESSING_FAILED);
		xmidtQDequeue();
	}

	if(msg->u.event.source !=NULL)
	{
		free(msg->u.event.source);
		msg->u.event.source = NULL;
	}
	if(msg->u.event.content_type !=NULL)
	{
		free(msg->u.event.content_type);
		msg->u.event.content_type = NULL;
	}
}

void createOutParamsandSendAck(wrp_msg_t *msg, rbusMethodAsyncHandle_t asyncHandle, char *errorMsg, int statuscode)
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

	ParodusPrint("statuscode %d errorMsg %s\n", statuscode, errorMsg);
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
			ParodusPrint("msg->u.event.source is %s\n", msg->u.event.source);
			rbusValue_Init(&value);
			rbusValue_SetString(value, msg->u.event.source);
			rbusObject_SetValue(outParams, "source", value);
			rbusValue_Release(value);
		}

		if(msg->u.event.dest !=NULL)
		{
			rbusValue_Init(&value);
			rbusValue_SetString(value, msg->u.event.dest);
			rbusObject_SetValue(outParams, "dest", value);
			rbusValue_Release(value);
		}

		if(msg->u.event.content_type !=NULL)
		{
			rbusValue_Init(&value);
			rbusValue_SetString(value, msg->u.event.content_type);
			rbusObject_SetValue(outParams, "content_type", value);
			rbusValue_Release(value);
		}

		rbusValue_Init(&value);
		snprintf(qosstring, sizeof(qosstring), "%d", msg->u.event.qos);
		ParodusPrint("qosstring is %s\n", qosstring);
		rbusValue_SetString(value, qosstring);
		rbusObject_SetValue(outParams, "qos", value);
		rbusValue_Release(value);

		if(msg->u.event.transaction_uuid !=NULL)
		{
			rbusValue_Init(&value);
			rbusValue_SetString(value, msg->u.event.transaction_uuid);
			rbusObject_SetValue(outParams, "transaction_uuid", value);
			rbusValue_Release(value);
			ParodusPrint("outParams msg->u.event.transaction_uuid %s\n", msg->u.event.transaction_uuid);
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

		err = rbusMethod_SendAsyncResponse(asyncHandle, RBUS_ERROR_SUCCESS, outParams);
		//err = rbusMethod_SendAsyncResponse(asyncHandle, RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION, outParams); //for negative case
		
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
			transaction_uuid = generate_transaction_uuid();
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
