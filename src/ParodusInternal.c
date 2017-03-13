/**
 * @file ParodusInternal.c
 *
 * @description This file is used to manage internal functions of parodus
 *
 * Copyright (c) 2015  Comcast
 */
 
#include "ParodusInternal.h"
#include "config.h"
#include "connection.h"

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
char* getWebpaConveyHeader()
{
    cJSON *response = cJSON_CreateObject();
    char *buffer = NULL;
    static char encodedData[1024];
    int  encodedDataSize = 1024;
    char * reconnect_reason = get_global_reconnect_reason();
    int i =0, j=0;

    if(strlen(get_parodus_cfg()->hw_model)!=0)
    {
	    cJSON_AddStringToObject(response, HW_MODELNAME, get_parodus_cfg()->hw_model);
    }

    if(strlen(get_parodus_cfg()->hw_serial_number)!=0)
    {
	    cJSON_AddStringToObject(response, HW_SERIALNUMBER, get_parodus_cfg()->hw_serial_number);
    }

    if(strlen(get_parodus_cfg()->hw_manufacturer)!=0)
    {
	    cJSON_AddStringToObject(response, HW_MANUFACTURER, get_parodus_cfg()->hw_manufacturer);
    }

    if(strlen(get_parodus_cfg()->fw_name)!=0)
    {
	    cJSON_AddStringToObject(response, FIRMWARE_NAME, get_parodus_cfg()->fw_name);
    }

    cJSON_AddNumberToObject(response, BOOT_TIME, get_parodus_cfg()->boot_time);

    if(strlen(get_parodus_cfg()->webpa_protocol)!=0)
    {
        cJSON_AddStringToObject(response, WEBPA_PROTOCOL, get_parodus_cfg()->webpa_protocol);
    }

    if(strlen(get_parodus_cfg()->webpa_interface_used)!=0)
    {
	    cJSON_AddStringToObject(response, WEBPA_INTERFACE, get_parodus_cfg()->webpa_interface_used);
    }	

    if(strlen(get_parodus_cfg()->hw_last_reboot_reason)!=0)
    {
        cJSON_AddStringToObject(response, HW_LAST_REBOOT_REASON, get_parodus_cfg()->hw_last_reboot_reason);
    }
    else
    {
	    ParodusError("Failed to GET Reboot reason value\n");
    }

    if(reconnect_reason !=NULL)
    {			
        cJSON_AddStringToObject(response, LAST_RECONNECT_REASON, reconnect_reason);			
    }
    else
    {
     	ParodusError("Failed to GET Reconnect reason value\n");
    }
	
    buffer = cJSON_PrintUnformatted(response);
    ParodusInfo("X-WebPA-Convey Header: [%zd]%s\n", strlen(buffer), buffer);

    if(nopoll_base64_encode (buffer, strlen(buffer), encodedData, &encodedDataSize) != nopoll_true)
    {
	    ParodusError("Base64 Encoding failed for Connection Header\n");
    }
    else
    {
	    // Remove \n characters from the base64 encoded data 
	    for(i=0;encodedData[i] != '\0';i++)
	    {
		    if(encodedData[i] == '\n')
		    {
			    ParodusPrint("New line is present in encoded data at position %d\n",i);
		    }
		    else
		    {
			    encodedData[j] = encodedData[i];
			    j++;
		    }
	    }
	    encodedData[j]='\0';
	    ParodusPrint("Encoded X-WebPA-Convey Header: [%zd]%s\n", strlen(encodedData), encodedData);
    }

    return encodedData;
}
