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


static rbusError_t sendDataHandler(rbusHandle_t handle, char const* methodName, rbusObject_t inParams, rbusObject_t outParams, rbusMethodAsyncHandle_t asyncHandle)
{
	(void) handle;
	ParodusInfo("methodHandler called: %s\n", methodName);
	rbusObject_fwrite(inParams, 1, stdout);

	ParodusInfo("Print InParams..\n");
	rbusValue_t name = rbusObject_GetValue(inParams, "name");

        //if(name && (rbusValue_GetType(name) == RBUS_STRING))
        //{
		ParodusInfo("name type: %d\n", rbusValue_GetType(name));
		char * nameStr = rbusValue_ToString(name, NULL, 0);
		//char * nameStr = rbusValue_GetString(name, NULL);
		ParodusInfo("name received is %s\n", nameStr);

		ParodusInfo("payload..\n");
		rbusValue_t payload = rbusObject_GetValue(inParams, "payload");
		ParodusInfo("payload type: %d\n", rbusValue_GetType(payload));
		//char * payloadStr = rbusValue_GetString(payload, NULL);
		char * payloadStr = rbusValue_ToString(payload, NULL, 0);
		ParodusInfo("payloadStr received is %s\n", payloadStr);
	//}
	ParodusInfo("Print InParams done\n");
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
