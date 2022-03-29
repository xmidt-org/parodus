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
 * @description Parodus to register as a provider for "xmidt_sendData" and provide the functionality to send the telemetry payload via Xmidt using the parameters supplied to the xmidt_sendData function.
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

XmidtMsg * get_global_XmidtMsgQ(void)
{
    return XmidtMsgQ;
}

void set_global_XmidtMsgQ(XmidtMsg * xUpStreamQ)
{
    XmidtMsgQ = xUpStreamQ;
}

pthread_cond_t *get_global_xmidt_con(void)
{
    return &xmidt_con;
}

pthread_mutex_t *get_global_xmidt_mut(void)
{
    return &xmidt_mut;
}

bool highQosValueCheck(int qos)
{
	if(qos > 24)
	{
		ParodusInfo("The qos value is high\n");
		return true;
	}
	else
	{
		ParodusInfo("The qos value is low\n");
	}

	return false;
}
/*
 * @brief To handle xmidt rbus messages received from various components.
 */
void addToXmidtUpstreamQ(wrp_msg_t * msg, rbusMethodAsyncHandle_t asyncHandle)
{
	XmidtMsg *message;

	ParodusInfo("XmidtQsize is %d\n" , XmidtQsize);
	if(XmidtQsize == MAX_QUEUE_SIZE)
	{
		char * errorMsg = strdup("Max Queue Size Exceeded");
		ParodusInfo("Queue Size Exceeded\n");
		createOutParamsandSendAck(msg, asyncHandle, errorMsg , QUEUE_SIZE_EXCEEDED);
		return;
	}

	ParodusInfo ("Add Xmidt Upstream message to queue\n");
	message = (XmidtMsg *)malloc(sizeof(XmidtMsg));

	if(message)
	{
		message->msg = msg;
		message->asyncHandle =asyncHandle;
		//Increment queue size to handle max queue limit
		XmidtQsize++;
		ParodusInfo("XmidtQsize is %d\n" , XmidtQsize);
		message->next=NULL;
		pthread_mutex_lock (&xmidt_mut);
		//Producer adds the rbus msg into queue
		if(XmidtMsgQ == NULL)
		{
			XmidtMsgQ = message;

			ParodusInfo("Producer added xmidt message\n");
			pthread_cond_signal(&xmidt_con);
			pthread_mutex_unlock (&xmidt_mut);
			ParodusInfo("mutex unlock in xmidt producer\n");
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
		ParodusError("failure in allocation for xmidt message\n");
	}
	ParodusInfo ("End of addToXmidtUpstreamQ\n");
	return;
}

//To remove an event from Queue
void xmidtQDequeue()
{
	ParodusInfo("Inside dequeue\n");
	XmidtMsg *temp = NULL;
	pthread_mutex_lock (&xmidt_mut);
	if(XmidtMsgQ != NULL)
	{
		temp = XmidtMsgQ;
		XmidtMsgQ = XmidtMsgQ->next;
		wrp_free_struct(temp->msg);
		XmidtQsize -= 1;
		free(temp);
	}
	pthread_mutex_unlock (&xmidt_mut);
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
	int ret = 0;

	while(FOREVER())
	{
		ParodusInfo("xmidt mutex b4 lock\n");
		pthread_mutex_lock (&xmidt_mut);
		ParodusInfo("mutex lock in xmidt consumer thread\n");
		if(XmidtMsgQ != NULL)
		{
			XmidtMsg *Data = XmidtMsgQ;
			
			pthread_mutex_unlock (&xmidt_mut);
			ParodusInfo("mutex unlock in xmidt consumer thread\n");
			ParodusInfo("Asynchandle pointer %p\n");
			ret = processData(Data->msg, Data->asyncHandle);
			if(ret)
			{
				ParodusInfo("xmidt processData is success\n");
			}
			else
			{
				ParodusError("Failed to process xmidt data\n");
			}
		}
		else
		{
			if (g_shutdown)
			{
				pthread_mutex_unlock (&xmidt_mut);
				break;
			}
			ParodusInfo("Before pthread cond wait in xmidt consumer thread\n");
			pthread_cond_wait(&xmidt_con, &xmidt_mut);
			pthread_mutex_unlock (&xmidt_mut);
			ParodusInfo("mutex unlock in xmidt consumer thread after cond wait\n");
		}
	}
	return NULL;
}

int processData(wrp_msg_t * msg, rbusMethodAsyncHandle_t asyncHandle)
{
	int rv = 0;
	char *errorMsg = "none";
	//rbusObject_t outParams;
	//rbusError_t err;
	int statuscode =0;

	wrp_msg_t * xmidtMsg = msg;
	if (xmidtMsg == NULL)
	{
		ParodusError("xmidtMsg is NULL\n");
		return 0;
	}

	rv = validateXmidtData(xmidtMsg, &errorMsg, &statuscode);
	ParodusInfo("After validateXmidtData, errorMsg %s statuscode %d\n", errorMsg, statuscode);
	if(rv)
	{
		ParodusInfo("validation successful, send event to server\n");
		sendXmidtEventToServer(xmidtMsg, asyncHandle);
		return 1;
	}
	else
	{
		ParodusError("validation failed, send failure ack\n");
		ParodusInfo("errorMsg %s\n", errorMsg);
		createOutParamsandSendAck(xmidtMsg, asyncHandle, errorMsg , statuscode);
		xmidtQDequeue();
		ParodusInfo("ack done\n");
	}
	return 0;
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
		ParodusError("*errorMsg %s *statusCode %d\n", *errorMsg, *statusCode);
		return 0;
	}

	if(eventMsg->u.event.source == NULL)
	{
		*errorMsg = strdup("Missing source");
		*statusCode = MISSING_SOURCE;
		ParodusError("*errorMsg %s *statusCode %d\n", *errorMsg, *statusCode);
		return 0;
	}

	if(eventMsg->u.event.dest == NULL)
	{
		*errorMsg = strdup("Missing dest");
		*statusCode = MISSING_DEST;
		ParodusError("*errorMsg %s *statusCode %d\n", *errorMsg, *statusCode);
		return 0;
	}

	if(eventMsg->u.event.content_type == NULL)
	{
		*errorMsg = strdup("Missing content_type");
		*statusCode = MISSING_CONTENT_TYPE;
		ParodusError("*errorMsg %s *statusCode %d\n", *errorMsg, *statusCode);
		return 0;
	}

	if(eventMsg->u.event.payload == NULL)
	{
		*errorMsg = strdup("Missing payload");
		*statusCode = MISSING_PAYLOAD;
		ParodusError("*errorMsg %s *statusCode %d\n", *errorMsg, *statusCode);
		return 0;
	}

	if(eventMsg->u.event.payload_size == 0)
	{
		*errorMsg = strdup("Missing payloadlen");
		*statusCode = MISSING_PAYLOADLEN;
		ParodusError("*errorMsg %s *statusCode %d\n", *errorMsg, *statusCode);
		return 0;
	}

	ParodusInfo("validateXmidtData success. *errorMsg %s *statusCode %d\n", *errorMsg, *statusCode);
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

	ParodusInfo("In sendXmidtEventToServer\n");
	notif_wrp_msg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
	if(notif_wrp_msg != NULL)
	{
		memset(notif_wrp_msg, 0, sizeof(wrp_msg_t));
		notif_wrp_msg->msg_type = WRP_MSG_TYPE__EVENT;

		ParodusInfo("msg->u.event.source: %s\n",msg->u.event.source);

		if(msg->u.event.source !=NULL)
		{
			//To get device_id in the format "mac:112233445xxx"
			ret = getDeviceId(&device_id, &device_id_len);
			if(ret == 0)
			{
				ParodusInfo("device_id %s device_id_len %lu\n", device_id, device_id_len);
				snprintf(sourceStr, sizeof(sourceStr), "%s/%s", device_id, msg->u.event.source);
				ParodusInfo("sourceStr formed is %s\n" , sourceStr);
				notif_wrp_msg->u.event.source = strdup(sourceStr);
				ParodusInfo("notif_wrp_msg->u.event.source is %s\n", notif_wrp_msg->u.event.source);
			}
			else
			{
				ParodusError("Failed to get device_id\n");
			}
		}
		ParodusInfo("destination: %s\n", msg->u.event.dest);
		notif_wrp_msg->u.event.dest = msg->u.event.dest;
		if(msg->u.event.content_type != NULL)
		{
			if(strcmp(msg->u.event.content_type , "JSON") == 0)
			{
				notif_wrp_msg->u.event.content_type = strdup("application/json");
			}
			ParodusInfo("content_type is %s\n",notif_wrp_msg->u.event.content_type);
		}
		if(msg->u.event.payload != NULL)
		{
			ParodusInfo("Notification payload: %s\n",msg->u.event.payload);
			notif_wrp_msg->u.event.payload = (void *)msg->u.event.payload;
			notif_wrp_msg->u.event.payload_size = msg->u.event.payload_size;
			ParodusInfo("payload size %lu\n", notif_wrp_msg->u.event.payload_size);
		}

		if(msg->u.event.qos != 0)
		{
			notif_wrp_msg->u.event.qos = msg->u.event.qos;
			qos = notif_wrp_msg->u.event.qos;
			ParodusInfo("Notification qos: %d\n",notif_wrp_msg->u.event.qos);
		}
		ParodusInfo("Encode xmidt wrp msg\n");
		msg_len = wrp_struct_to (notif_wrp_msg, WRP_BYTES, &msg_bytes);

		ParodusInfo("Encoded xmidt wrp msg, msg_len %lu\n", msg_len);
		if(msg_len > 0)
		{
			ParodusInfo("sendUpstreamMsgToServer\n");
			sendRetStatus = sendUpstreamMsgToServer(&msg_bytes, msg_len);
		}
		else
		{
			ParodusInfo("The msg_len is zero\n");
			xmidtQDequeue();
			wrp_free_struct(notif_wrp_msg);
			ParodusInfo("After wrp_free_struct\n");
			free(msg_bytes);
			msg_bytes = NULL;
			return;
		}

		while(sendRetStatus)     //If SendMessage is failed condition
		{
			ParodusInfo("sendXmidtEventToServer is Failed\n");
			if(highQosValueCheck(qos))
			{
				errorMsg = strdup("sendXmidtEventToServer Failed , Enqueue event since QOS is High");
				ParodusInfo("The event is having high qos retry again \n");
				createOutParamsandSendAck(msg, asyncHandle, errorMsg, CLIENT_DISCONNECT);
				ParodusInfo("The value of pointer xmidt->msg is %p\n", XmidtMsgQ->msg);
				ParodusInfo("Wait till connection is Up\n");

				pthread_cond_wait(&xmidt_con, &xmidt_mut);
				pthread_mutex_unlock(&xmidt_mut);
				ParodusInfo("Received signal proceed to retry\n");
			}
			else
			{
				errorMsg = strdup("sendXmidtEventToServer Failed , Dequeue event since QOS is Low");
				ParodusInfo("The event is having low qos proceed to dequeue\n");
				createOutParamsandSendAck(msg, asyncHandle, errorMsg, ENQUEUE_FAILURE);
				xmidtQDequeue();
				break;
			}
			sendRetStatus = sendUpstreamMsgToServer(&msg_bytes, msg_len);
			ParodusInfo("After sendRetStatus async pointer is %p for qos %d\n", asyncHandle, qos);
		}

		if(sendRetStatus == 0)
		{
			errorMsg = strdup("sendXmidtEventToServer is Success");
			ParodusInfo("sendXmidtEventToServer done\n");
			createOutParamsandSendAck(msg, asyncHandle, errorMsg, DELIVERED_SUCCESS);
			xmidtQDequeue();
		}

		wrp_free_struct(notif_wrp_msg);
		ParodusInfo("After wrp_free_struct\n");
		free(msg_bytes);
		msg_bytes = NULL;
		ParodusInfo("sendXmidtEventToServer done\n");
	}

}

void createOutParamsandSendAck(wrp_msg_t *msg, rbusMethodAsyncHandle_t asyncHandle, char *errorMsg, int statuscode)
{
	rbusObject_t outParams;
	rbusError_t err;
	rbusValue_t value;
	char qosstring[20] = "";

	rbusMethodAsyncHandle_t tempAsyncHandle = malloc(sizeof(struct rbusMethodAsyncHandle_t*));
	memcpy(tempAsyncHandle, asyncHandle, sizeof(rbusMethodAsyncHandle_t*));

	if(msg == NULL)
	{
		ParodusError("msg is NULL\n");
		return;
	}

	ParodusInfo("createOutParams\n");

	rbusValue_Init(&value);
	rbusValue_SetString(value, "event");
	rbusObject_Init(&outParams, NULL);
	rbusObject_SetValue(outParams, "msg_type", value);
	rbusValue_Release(value);

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
		ParodusInfo("Inside dest\n");
		rbusValue_Init(&value);
		rbusValue_SetString(value, msg->u.event.dest);
		rbusObject_SetValue(outParams, "dest", value);
		rbusValue_Release(value);
	}

	if(msg->u.event.content_type !=NULL)
	{
		ParodusInfo("Inside content_type\n");
		rbusValue_Init(&value);
		rbusValue_SetString(value, msg->u.event.content_type);
		rbusObject_SetValue(outParams, "content_type", value);
		rbusValue_Release(value);
	}

	rbusValue_Init(&value);
	ParodusInfo("msg->u.event.qos int %d\n", msg->u.event.qos);
	snprintf(qosstring, sizeof(qosstring), "%d", msg->u.event.qos);
	ParodusInfo("qosstring is %s\n", qosstring);
	rbusValue_SetString(value, qosstring);
	rbusObject_SetValue(outParams, "qos", value);
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

	rbusValue_Init(&value);
	rbusValue_SetString(value, "transaction_uuid"); //change this to actual transid
	rbusObject_SetValue(outParams, "transaction_uuid", value);
	rbusValue_Release(value);


	if(outParams !=NULL)
	{
		if(tempAsyncHandle == NULL)
		{
			ParodusInfo("tempAsyncHandle is NULL\n");
			return;
		}
		ParodusInfo("B4 rbusMethod_SendAsyncResponse ..\n");
		ParodusInfo("b4 rbusMethod_SendAsyncResponse async pointer is %p for qos %s\n", tempAsyncHandle, qosstring);
		err = rbusMethod_SendAsyncResponse(tempAsyncHandle, RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION, outParams);
		ParodusInfo("After rbusMethod_SendAsyncResponse\n");
		ParodusInfo("err is %d RBUS_ERROR_SUCCESS %d\n", err, RBUS_ERROR_SUCCESS);
		if(err != RBUS_ERROR_SUCCESS)
		{
			ParodusError("rbusMethod_SendAsyncResponse failed err:%d\n", err);
		}
		else
		{
			ParodusInfo("rbusMethod_SendAsyncResponse success:%d\n", err);
		}
		ParodusInfo("Release outParams\n");
		rbusObject_Release(outParams);
		ParodusInfo("outParams released\n");
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
		ParodusInfo("Rbus check method. Not proceeding to process this inparam\n");
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

void parseRbusInparamsToWrp(rbusObject_t inParams, wrp_msg_t **eventMsg)
{
	const char *msg_typeStr = NULL;
	const char *sourceVal = NULL;
	const char *destStr = NULL, *contenttypeStr = NULL;
	const char *payloadStr = NULL, *qosVal = NULL;
	unsigned int payloadlength = 0;

	wrp_msg_t *msg = NULL;

	msg = ( wrp_msg_t * ) malloc( sizeof( wrp_msg_t ) );
	if(msg != NULL)
	{
		memset( msg, 0, sizeof( wrp_msg_t ) );
	}

	rbusValue_t msg_type = rbusObject_GetValue(inParams, "msg_type");
	if(msg_type)
	{
		if(rbusValue_GetType(msg_type) == RBUS_STRING)
		{
			msg_typeStr = rbusValue_GetString(msg_type, NULL);
			ParodusInfo("msg_type value received is %s\n", msg_typeStr);
			if((msg_typeStr !=NULL) && (strcmp(msg_typeStr, "event") ==0))
			{
				msg->msg_type = WRP_MSG_TYPE__EVENT;
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
			sourceVal = rbusValue_GetString(source, NULL);
			ParodusInfo("source value received is %s\n", sourceVal);
			msg->u.event.source = strdup(sourceVal); //free
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
			destStr = rbusValue_GetString(dest, NULL);
			ParodusInfo("dest value received is %s\n", destStr);
			msg->u.event.dest = strdup(destStr); //free
			ParodusPrint("msg->u.event.dest is %s\n", msg->u.event.dest);
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
			contenttypeStr = rbusValue_GetString(contenttype, NULL);
			ParodusInfo("contenttype value received is %s\n", contenttypeStr);
			msg->u.event.content_type = strdup(contenttypeStr); //free
			ParodusPrint("msg->u.event.content_type is %s\n", msg->u.event.content_type);
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
			payloadStr = rbusValue_GetString(payload, NULL);
			ParodusInfo("payload received is %s\n", payloadStr);
			msg->u.event.payload = strdup(payloadStr); //free
			ParodusPrint("msg->u.event.payload is %s\n", msg->u.event.payload);
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

	ParodusInfo("check qos\n");
	rbusValue_t qos = rbusObject_GetValue(inParams, "qos");
	if(qos)
	{
		if(rbusValue_GetType(qos) == RBUS_STRING)
		{
			ParodusPrint("qos type %d RBUS_STRING %d\n", rbusValue_GetType(qos), RBUS_STRING);
			qosVal = rbusValue_GetString(qos, NULL);
			ParodusInfo("qos received is %s\n", qosVal);
			if(qosVal !=NULL)
			{
				msg->u.event.qos = atoi(qosVal);
				ParodusPrint("msg->u.event.qos is %d\n", msg->u.event.qos);
			}
		}
	}
	 *eventMsg = msg;
	ParodusInfo("parseRbusInparamsToWrp End\n");
}

static rbusError_t sendDataHandler(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
	(void) handle;
	int inStatus = 0;
	char *transaction_uuid = NULL;
	wrp_msg_t *wrpMsg= NULL;
	ParodusInfo("methodHandler called: %s\n", methodName);
	//rbusObject_fwrite(inParams, 1, stdout);

	if((methodName !=NULL) && (strcmp(methodName, XMIDT_SEND_METHOD) == 0))
	{
		inStatus = checkInputParameters(inParams);
		if(inStatus)
		{
			ParodusInfo("InParam Retain\n");
			rbusObject_Retain(inParams);
			parseRbusInparamsToWrp(inParams, &wrpMsg);
			//generate transaction id to create outParams and send ack
			transaction_uuid = generate_transaction_uuid();
			ParodusInfo("xmidt transaction_uuid generated is %s\n", transaction_uuid);

			//xmidt send producer
			addToXmidtUpstreamQ(wrpMsg, asyncHandle);
			ParodusInfo("sendDataHandler returned %d\n", RBUS_ERROR_ASYNC_RESPONSE);
			//return RBUS_ERROR_ASYNC_RESPONSE;
		}
		else
		{
			ParodusInfo("check method call received, ignoring input\n");
		}
	}
	else
	{
		ParodusError("Method %s received is not supported\n", methodName);
		return RBUS_ERROR_BUS_ERROR;
	}
	return RBUS_ERROR_SUCCESS;
}


int regXmidtSendDataMethod()
{
	int rc = RBUS_ERROR_SUCCESS;
	rbusDataElement_t dataElements[1] = { { XMIDT_SEND_METHOD, RBUS_ELEMENT_TYPE_METHOD, { NULL, NULL, NULL, NULL, NULL, sendDataHandler } } };

	rbusHandle_t rbus_handle = get_parodus_rbus_Handle();

	ParodusInfo("Registering xmidt_sendData method %s\n", XMIDT_SEND_METHOD);
	if(!rbus_handle)
	{
		ParodusError("regXmidtSendDataMethod failed in getting bus handles\n");
		return -1;
	}

	rc = rbus_regDataElements(rbus_handle, 1, dataElements);
	
	if(rc != RBUS_ERROR_SUCCESS)
	{
		ParodusError("SendData provider: rbus_regDataElements failed: %d\n", rc);
	}
	else
	{
		ParodusInfo("SendData method provider register success\n");
	}
	
	//start xmidt consumer thread .(should we start from conn_interface.c?)
	processXmidtData();
	return rc;
}
