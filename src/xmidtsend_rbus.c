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
typedef struct MethodData 
{
    rbusMethodAsyncHandle_t asyncHandle;
    rbusObject_t inParams;
} MethodData;

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
	//contentType
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

	//msg_type
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

	//source
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

	//dest_root
	rbusValue_t dest_root = rbusObject_GetValue(inParams, "dest_root");
	if(dest_root)
	{
		if(rbusValue_GetType(dest_root) == RBUS_STRING)
		{
			char * dest_rootStr = rbusValue_GetString(dest_root, NULL);
			ParodusInfo("dest_root value received is %s\n", dest_rootStr);
		}
	}
	else
	{
		ParodusError("dest_root is empty\n");
	}

	//dest_path
	rbusValue_t dest_path = rbusObject_GetValue(inParams, "dest_path");
	if(dest_path)
	{
		if(rbusValue_GetType(dest_path) == RBUS_STRING)
		{
			char * dest_pathStr = rbusValue_GetString(dest_path, NULL);
			ParodusInfo("dest_path value received is %s\n", dest_pathStr);
		}
	}
	else
	{
		ParodusError("dest_path is empty\n");
	}

	//payload
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

	//payloadlen
	ParodusInfo("check payloadlen\n");
	rbusValue_t payloadlen = rbusObject_GetValue(inParams, "payloadlen");
	ParodusInfo("payloadlen value object\n");
	if(payloadlen)
	{
		ParodusInfo("payloadlen is not empty\n");
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

static rbusError_t sendDataHandler(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
	(void) handle;
	ParodusInfo("methodHandler called: %s\n", methodName);
	//rbusObject_fwrite(inParams, 1, stdout);

	ParodusInfo("displayInputParameters ..\n");
	displayInputParameters(inParams);
	ParodusInfo("displayInputParameters done\n");

	if(strcmp(methodName, XMIDT_SEND_METHOD) == 0)
	{
		pthread_t pid;
		MethodData* data = malloc(sizeof(MethodData));
		data->asyncHandle = asyncHandle;
		data->inParams = inParams;
		rbusObject_Retain(inParams);
		if(pthread_create(&pid, NULL, asyncMethodHandler, data) || pthread_detach(pid)) 
		{
			ParodusError("sendDataHandler failed to create thread\n");
			return RBUS_ERROR_BUS_ERROR;
		}
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
	
	return rc;
}
