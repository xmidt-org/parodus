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

#define XMIDT_SEND_METHOD "Device.X_RDK_Xmidt.SendData"
static bool returnStatus = true;
static pthread_t processThreadId = 0;

typedef struct MethodData 
{
    rbusMethodAsyncHandle_t asyncHandle;
    rbusObject_t inParams;
} MethodData;

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

static void* asyncMethodHandler(void *p)
{
	MethodData* data;
	rbusObject_t outParams;
	rbusValue_t value;
	rbusError_t err;
	ParodusInfo("Enter asyncMethodHandler. sending aync response back\n");

	//This should be a consumer that awake when xpc ack is received.
	//ParodusInfo("sleep of 10s for sending aync response back\n");
	//sleep(10);
	//ParodusInfo("sleep done\n");
	data = p;
	rbusValue_Init(&value);
	rbusValue_SetString(value, "Async method response from parodus");
	rbusObject_Init(&outParams, NULL);
	rbusObject_SetValue(outParams, "parodus_ack_response", value);
	rbusValue_Release(value);
	if(returnStatus)
	{
		ParodusInfo("asyncMethodHandler sending response as success\n");
		err = rbusMethod_SendAsyncResponse(data->asyncHandle, RBUS_ERROR_SUCCESS, outParams);
	}
	else
	{
		ParodusInfo("asyncMethodHandler sending response as failure\n");
		err = rbusMethod_SendAsyncResponse(data->asyncHandle, RBUS_ERROR_INVALID_RESPONSE_FROM_DESTINATION, outParams);
	}
	if(err != RBUS_ERROR_SUCCESS)
	{
		ParodusInfo("asyncMethodHandler rbusMethod_SendAsyncResponse failed err:%d\n", err);
	}
	else
	{
		ParodusInfo("asyncMethodHandler sent response to t2:%d\n", err);
	}
	rbusObject_Release(data->inParams);
	rbusObject_Release(outParams);
	if(data !=NULL)
	{
		free(data);
		data = NULL;
	}
	ParodusInfo("Exit asyncMethodHandler\n");
	return NULL;
}

void displayInputParameters(rbusObject_t inParams)
{
	rbusValue_t contenttype = rbusObject_GetValue(inParams, "contentType");
	if(contenttype)
	{
		if(rbusValue_GetType(contenttype) == RBUS_STRING)
		{
			char * contenttypeStr = rbusValue_GetString(contenttype, NULL);
			ParodusInfo("contenttype value received is %s\n", contenttypeStr);
		}
	}
	else
	{
		ParodusError("contenttype is empty\n");
	}

	rbusValue_t msg_type = rbusObject_GetValue(inParams, "msg_type");
	if(msg_type)
	{
		if(rbusValue_GetType(msg_type) == RBUS_STRING)
		{
			char *msg_typeStr = rbusValue_GetString(msg_type, NULL);
			ParodusInfo("msg_type value received is %s\n", msg_typeStr);
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
			char * sourceStr = rbusValue_GetString(source, NULL);
			ParodusInfo("source value received is %s\n", sourceStr);
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
			char * dest_Str = rbusValue_GetString(dest, NULL);
			ParodusInfo("dest value received is %s\n", dest_Str);
		}
	}
	else
	{
		ParodusError("dest is empty\n");
	}

	rbusValue_t payload = rbusObject_GetValue(inParams, "payload");
	if(payload)
	{
		if((rbusValue_GetType(payload) == RBUS_STRING))
		{
			char * payloadStr = rbusValue_GetString(payload, NULL);
			ParodusInfo("payload received is %s\n", payloadStr);
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
			ParodusInfo("payloadlen type %d RBUS_INT32 %d\n", rbusValue_GetType(payloadlen), RBUS_INT32);
			int payloadlength = rbusValue_GetInt32(payloadlen);
			ParodusInfo("payloadlen received is %d\n", payloadlength);
		}
	}
	else
	{
		ParodusError("payloadlen is empty\n");
	}
}

/*
 * @brief To handle xmidt rbus messages received from various components.
 */

void addToXmidtUpstreamQ(rbusObject_t* inParams)
{
	XmidtMsg *message;

	ParodusInfo ("Add Xmidt Upstream message to queue\n");
	message = (XmidtMsg *)malloc(sizeof(XmidtMsg));

	if(message)
	{
		message->msg =inParams;
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
	while(FOREVER())
	{
		pthread_mutex_lock (&xmidt_mut);
		ParodusPrint("mutex lock in xmidt consumer thread\n");
		if(XmidtMsgQ != NULL)
		{
			XmidtMsg *Data = XmidtMsgQ;
			XmidtMsgQ = XmidtMsgQ->next;
			pthread_mutex_unlock (&xmidt_mut);
			ParodusInfo("mutex unlock in xmidt consumer thread\n");

			ParodusInfo("B4 parseData\n");
			parseData(Data->msg);
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

void parseData(rbusObject_t * msg)
{
	rbusObject_t inParamObj = NULL;

	inParamObj = *msg;

	ParodusInfo("print in params from consumer..\n");
	displayInputParameters(inParamObj);
	ParodusInfo("print inparams done\n");

	processXmidtEvent(inParamObj);
	ParodusInfo("processXmidtEvent done\n");

	//Need to send failure status when received input params are empty/invalid.
}

int processXmidtEvent(rbusObject_t inParams)
{
	char *contenttypeStr = NULL;
	char *msg_typeStr = NULL;
	char * sourceStr = NULL;
	char *source_string = NULL;
	char * dest_Str = NULL;
	char * payloadStr = NULL;
	unsigned int payloadlength = 0;
	int qosVal = 0;
	char *device_id = NULL;
	size_t device_id_len = 0;
	int ret = -1;
	int rv = 1;

	//contentType
	rbusValue_t contenttype = rbusObject_GetValue(inParams, "contentType");
	if(contenttype)
	{
		if(rbusValue_GetType(contenttype) == RBUS_STRING)
		{
			contenttypeStr = rbusValue_GetString(contenttype, NULL);
			ParodusInfo("contenttype value received is %s\n", contenttypeStr);
		}
	}
	else
	{
		ParodusError("contenttype is empty\n");
		return rv;
	}

	//msg_type
	rbusValue_t msg_type = rbusObject_GetValue(inParams, "msg_type");
	if(msg_type)
	{
		if(rbusValue_GetType(msg_type) == RBUS_STRING)
		{
			msg_typeStr = rbusValue_GetString(msg_type, NULL);
			ParodusInfo("msg_type value received is %s\n", msg_typeStr);
			if((msg_typeStr !=NULL) && (strcmp(msg_typeStr, "event") !=0))
			{
				ParodusError("msg_type is not event\n");
				return rv;
			}
		}
	}
	else
	{
		ParodusError("msg_type is empty\n");
		return rv;
	}

	//source
	rbusValue_t source = rbusObject_GetValue(inParams, "source");
	if(source)
	{
		if(rbusValue_GetType(source) == RBUS_STRING)
		{
			sourceStr = rbusValue_GetString(source, NULL);
			ParodusInfo("source value received is %s\n", sourceStr);
			if(sourceStr !=NULL)
			{
				//To get device_id in the format "mac:112233445xxx"
				ret = getDeviceId(&device_id, &device_id_len);
				if(ret == 0)
				{
					ParodusInfo("device_id %s device_id_len %lu\n", device_id, device_id_len);
					source_string = (char *) malloc(sizeof(char)*64);
					if(source_string  !=NULL)
					{
						snprintf(source_string, 64, "%s/%s", device_id, sourceStr);
						ParodusInfo("source_string formed is %s\n" , source_string);
					}
				}
				else
				{
					ParodusError("Failed to get device_id\n");
					return rv;
				}
			}

		}
	}
	else
	{
		ParodusError("source is empty\n");
		return rv;
	}

	//dest
	rbusValue_t dest = rbusObject_GetValue(inParams, "dest");
	if(dest)
	{
		if(rbusValue_GetType(dest) == RBUS_STRING)
		{
			dest_Str = rbusValue_GetString(dest, NULL);
			ParodusInfo("dest value received is %s\n", dest_Str);
		}
	}
	else
	{
		ParodusError("dest is empty\n");
		return rv;
	}

	//payload
	rbusValue_t payload = rbusObject_GetValue(inParams, "payload");
	if(payload)
	{
		if((rbusValue_GetType(payload) == RBUS_STRING))
		{
			payloadStr = rbusValue_GetString(payload, NULL);
			ParodusInfo("payload received is %s\n", payloadStr);
		}
	}
	else
	{
		ParodusError("payload is empty\n");
		return rv;
	}

	//payloadlen
	ParodusInfo("check payloadLen\n");
	rbusValue_t payloadlen = rbusObject_GetValue(inParams, "payloadLen");
	if(payloadlen)
	{
		ParodusPrint("payloadlen is not empty\n");
		if(rbusValue_GetType(payloadlen) == RBUS_INT32)
		{
			ParodusInfo("payloadlen type %d RBUS_INT32 %d\n", rbusValue_GetType(payloadlen), RBUS_INT32);
			payloadlength = rbusValue_GetInt32(payloadlen);
			ParodusInfo("payloadlen received is %lu\n", payloadlength);
		}
	}
	else
	{
		ParodusError("payloadlen is empty\n");
		return rv;
	}

	//qos optional
	ParodusInfo("check qos\n");
	rbusValue_t qos = rbusObject_GetValue(inParams, "qos");
	ParodusInfo("qos value object\n");
	if(qos)
	{
		ParodusInfo("qos is not empty\n");
		if(rbusValue_GetType(qos) == RBUS_INT32)
		{
			ParodusInfo("qos type %d RBUS_INT32 %d\n", rbusValue_GetType(qos), RBUS_INT32);
			qosVal = rbusValue_GetInt32(qos);
			ParodusInfo("qos received is %d\n", qosVal);
		}
	}

	ParodusInfo("B4 sendXmidtEventToServer\n");
	sendXmidtEventToServer(source_string, dest_Str, contenttypeStr, qosVal, payloadStr, payloadlength);
	ParodusInfo("processXmidtEvent is completed\n");
	rv = 0;
	return rv;
}

void sendXmidtEventToServer(char *source, char *destination, char* contenttype, int qos, void *payload, unsigned int payload_len)
{
	wrp_msg_t *notif_wrp_msg = NULL;
	int rc = RBUS_ERROR_SUCCESS;
	ssize_t msg_len;
	void *msg_bytes;

	if(source != NULL && destination != NULL && contenttype != NULL && payload != NULL)
	{
		notif_wrp_msg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
		if(notif_wrp_msg != NULL)
		{
			memset(notif_wrp_msg, 0, sizeof(wrp_msg_t));
			notif_wrp_msg->msg_type = WRP_MSG_TYPE__EVENT;
			ParodusInfo("source: %s\n",source);
			notif_wrp_msg->u.event.source = strdup(source); //free
			ParodusInfo("destination: %s\n", destination);
			notif_wrp_msg->u.event.dest = strdup(destination); //free
			if(contenttype != NULL)
			{
				if(strcmp(contenttype , "JSON") == 0)
				{
					notif_wrp_msg->u.event.content_type = strdup("application/json");
				}
				ParodusInfo("content_type is %s\n",notif_wrp_msg->u.event.content_type);
			}
			if(payload != NULL)
			{
				ParodusInfo("Notification payload: %s\n",payload);
				notif_wrp_msg->u.event.payload = (void *)payload;
				notif_wrp_msg->u.event.payload_size = payload_len;
				ParodusInfo("payload len %lu\n", payload_len);
			}

			ParodusInfo("Encode xmidt wrp msg\n");
			msg_len = wrp_struct_to (notif_wrp_msg, WRP_BYTES, &msg_bytes);

			ParodusInfo("Encoded xmidt wrp msg, msg_len %lu\n", msg_len);
			if(msg_len > 0)
			{
				ParodusInfo("sendUpstreamMsgToServer\n");
				sendUpstreamMsgToServer(&msg_bytes, msg_len);
			}
			ParodusInfo("B4 wrp_free_struct.\n");
			wrp_free_struct(notif_wrp_msg);
			ParodusInfo("After wrp_free_struct\n");
			free(msg_bytes);
			msg_bytes = NULL;
			ParodusInfo("sendXmidtEventToServer done\n");
		}
	}
}

static rbusError_t sendDataHandler(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
	(void) handle;
	ParodusInfo("methodHandler called: %s\n", methodName);
	//rbusObject_fwrite(inParams, 1, stdout);

	//ParodusInfo("displayInputParameters ..\n");
	//displayInputParameters(inParams);
	//ParodusInfo("displayInputParameters done\n");

	if(strcmp(methodName, XMIDT_SEND_METHOD) == 0)
	{
		/*pthread_t pid;
		MethodData* data = malloc(sizeof(MethodData));
		data->asyncHandle = asyncHandle;
		data->inParams = inParams;
		rbusObject_Retain(inParams);*/
		//xmidt send producer
		addToXmidtUpstreamQ(&inParams);
		/**** For now, disable async ack send to t2 nack ***/
		/*if(pthread_create(&pid, NULL, asyncMethodHandler, data) || pthread_detach(pid)) 
		{
			ParodusError("sendDataHandler failed to create thread\n");
			return RBUS_ERROR_BUS_ERROR;
		}*/
		ParodusInfo("sendDataHandler created async thread, returned %d\n", RBUS_ERROR_ASYNC_RESPONSE);
		return RBUS_ERROR_ASYNC_RESPONSE;
	}
	else
	{
		ParodusError("Method %s received is not supported\n", methodName);
		return RBUS_ERROR_BUS_ERROR;
	}
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
