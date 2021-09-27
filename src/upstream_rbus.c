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

void processWebconfigUpstreamMessage(rbusHandle_t handle, rbusMessage_t* msg, void * userData);

/* API to register RBUS listener to receive messages from webconfig */
void registerRBUSlistener()
{
	rbusError_t err;
	rbusHandle_t rbus_Handle;

	err = rbus_open(&rbus_Handle, "parodus");
	if (err)
	{
		ParodusError("rbus_open:%s\n", rbusError_ToString(err));
		return;
	}
	ParodusInfo("B4 rbusMessage_AddListener\n");
	rbusMessage_AddListener(rbus_Handle, "webconfig.upstream", &processWebconfigUpstreamMessage, NULL);
	ParodusInfo("After rbusMessage_AddListener\n");
}

void processWebconfigUpstreamMessage(rbusHandle_t handle, rbusMessage_t* msg, void * userData)
{
	int rv=-1;
	wrp_msg_t *event_msg;
	void *bytes;

	rv = wrp_to_struct( msg->data, msg->length, WRP_BYTES, &event_msg );
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
		   sendUpstreamMsgToServer((void **)(&msg->data), msg->length);
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


