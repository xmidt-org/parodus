/*
 * upstream_rbus.c
 *
 *  Created on: 09-Sep-2021
 *      Author: infyuser
 */
#include <stdlib.h>
#include <rbus.h>
#include "upstream.h"
#include "ParodusInternal.h"
#include "partners_check.h"

#define WEBCFG_UPSTREAM_EVENT "Webconfig.Upstream"

void processWebconfigUpstreamEvent(rbusHandle_t handle, rbusEvent_t const* event, rbusEventSubscription_t* subscription);

void subscribeAsyncHandler( rbusHandle_t handle, rbusEventSubscription_t* subscription, rbusError_t error);

/* API to register RBUS listener to receive messages from webconfig */
void subscribeRBUSevent()
{
	rbusError_t err;
    int rc = RBUS_ERROR_SUCCESS;
	rbusHandle_t rbus_Handle;

	err = rbus_open(&rbus_Handle, "parodus");
	if (err)
	{
		ParodusError("rbus_open failed :%s\n", rbusError_ToString(err));
		return;
	}
    rc = rbusEvent_SubscribeAsync(rbus_Handle,WEBCFG_UPSTREAM_EVENT,processWebconfigUpstreamEvent,subscribeAsyncHandler,"parodus",10*60);
    if(rc != RBUS_ERROR_SUCCESS)
        ParodusError("rbusEvent_Subscribe failed: %d, %s\n", rc, rbusError_ToString(rc));
    else
        ParodusInfo("rbusEvent_Subscribe was successful\n");
}

void processWebconfigUpstreamEvent(rbusHandle_t handle, rbusEvent_t const* event, rbusEventSubscription_t* subscription)
{
    (void)handle;
    (void)subscription;
    
    int rv=-1;
	wrp_msg_t *event_msg;
	void *bytes;
    int len;
    rbusValue_t value = NULL;

    value = rbusObject_GetValue(event->data, "value");
    bytes = rbusValue_GetBytes(value, &len);

	rv = wrp_to_struct( bytes, len, WRP_BYTES, &event_msg );
	if(rv > 0)
	{
		ParodusInfo(" Received upstream event data: dest '%s'\n", event_msg->u.event.dest);
		partners_t *partnersList = NULL;
		int j = 0;

		int ret = validate_partner_id(event_msg, &partnersList);
		if(ret == 1)
		{
		    wrp_msg_t *eventMsg = (wrp_msg_t *) malloc(sizeof(wrp_msg_t));
		    eventMsg->msg_type = event_msg->msg_type;
		    eventMsg->u.event.content_type=event_msg->u.event.content_type;
		    eventMsg->u.event.source=event_msg->u.event.source;
		    eventMsg->u.event.dest=event_msg->u.event.dest;
		    eventMsg->u.event.payload=event_msg->u.event.payload;
		    eventMsg->u.event.payload_size=event_msg->u.event.payload_size;
		    eventMsg->u.event.headers=event_msg->u.event.headers;
		    eventMsg->u.event.metadata=event_msg->u.event.metadata;
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
		   sendUpstreamMsgToServer((void **)(&bytes), len);
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
}

void subscribeAsyncHandler( rbusHandle_t handle, rbusEventSubscription_t* subscription, rbusError_t error)
{
	ParodusInfo("subscribeAsyncHandler event %s, error %d - %s\n",subscription->eventName, error, rbusError_ToString(error));
}
