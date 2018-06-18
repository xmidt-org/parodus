#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cJSON.h>
#include <wrp-c.h>
#include "crud_tasks.h"
#include "crud_internal.h"


int processCrudRequest( wrp_msg_t *reqMsg, wrp_msg_t **responseMsg)
{
	wrp_msg_t *resp_msg = NULL;
    char *str= NULL;
    int ret = -1;

    resp_msg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
    memset(resp_msg, 0, sizeof(wrp_msg_t));

    resp_msg->msg_type = reqMsg->msg_type;
    resp_msg->u.crud.transaction_uuid = strdup(reqMsg->u.crud.transaction_uuid);
    resp_msg->u.crud.source = strdup(reqMsg->u.crud.dest);
    resp_msg->u.crud.dest = strdup(reqMsg->u.crud.source);

    switch( reqMsg->msg_type ) 
    {

	case WRP_MSG_TYPE__CREATE:
	ParodusInfo( "CREATE request\n" );

	ret = createObject( reqMsg, &resp_msg );

	if(ret == 0)
	{
		cJSON *payloadObj = cJSON_Parse( (resp_msg)->u.crud.payload );
		str = cJSON_PrintUnformatted(payloadObj);

		resp_msg ->u.crud.payload = (void *)str;
		if(str !=NULL)
		{
			ParodusInfo("Payload Response: %s\n", str);
			resp_msg ->u.crud.payload_size = strlen(str);
		}
		cJSON_Delete( payloadObj );
	}
	else
	{
		ParodusError("Failed to create object in config JSON\n");

		//WRP payload is NULL for failure cases
		resp_msg ->u.crud.payload = NULL;
		resp_msg ->u.crud.payload_size = 0;
		*responseMsg = resp_msg;
		return -1;
	}

	*responseMsg = resp_msg;
	break;

	case WRP_MSG_TYPE__RETREIVE:
	ParodusInfo( "RETREIVE request\n" );

	ret = retrieveObject( reqMsg, &resp_msg );
	if(ret == 0)
	{
	    cJSON *payloadObj = cJSON_Parse( (resp_msg)->u.crud.payload );
	    str = cJSON_PrintUnformatted(payloadObj);

	    resp_msg ->u.crud.payload = (void *)str;
	    if((resp_msg)->u.crud.payload !=NULL)
	    {
	    	ParodusInfo("Payload Response: %s\n", str);
			resp_msg ->u.crud.payload_size = strlen((resp_msg)->u.crud.payload);
	    }
	    cJSON_Delete( payloadObj );
	}
	else
	{
	    ParodusError("Failed to retrieve object \n");

	    //WRP payload is NULL for failure cases
	    resp_msg ->u.crud.payload = NULL;
	    resp_msg ->u.crud.payload_size = 0;
	    *responseMsg = resp_msg;
	    return -1;
	}

	*responseMsg = resp_msg;
	break;

	case WRP_MSG_TYPE__UPDATE:
	ParodusInfo( "UPDATE request\n" );

	ret = updateObject( reqMsg, &resp_msg );
	if(ret ==0)
	{
		//WRP payload is NULL for update requests
		resp_msg ->u.crud.payload = NULL;
		resp_msg ->u.crud.payload_size = 0;
	}
	else
	{
		ParodusError("Failed to update object \n");
		*responseMsg = resp_msg;
		return -1;
	}
	*responseMsg = resp_msg;
	break;

	case WRP_MSG_TYPE__DELETE:
	ParodusInfo( "DELETE request\n" );

	ret = deleteObject(reqMsg, &resp_msg );
	if(ret == 0)
	{
		//WRP payload is NULL for delete requests
		resp_msg ->u.crud.payload = NULL;
		resp_msg ->u.crud.payload_size = 0;
	}
	else
	{
		ParodusError("Failed to delete object \n");
		*responseMsg = resp_msg;
		return -1;
	}
	*responseMsg = resp_msg;
	break;

	default:
	    ParodusInfo( "Unknown msgType for CRUD request\n" );
	    *responseMsg = resp_msg;
	    break;
     }

    return  0;
}

