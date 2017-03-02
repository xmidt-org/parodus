#include "ParodusInternal.h"
#include "config.h"

char parodus_url[32] ={'\0'};

char* getWebpaConveyHeader()
{
    cJSON *response = cJSON_CreateObject();
    char *buffer = NULL;
    static char encodedData[1024];
    int  encodedDataSize = 1024;
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

void packMetaData()
{
    char boot_time[256]={'\0'};
    //Pack the metadata initially to reuse for every upstream msg sending to server
    ParodusPrint("-------------- Packing metadata ----------------\n");
    sprintf(boot_time, "%d", get_parodus_cfg()->boot_time);

    struct data meta_pack[METADATA_COUNT] = {
            {HW_MODELNAME, get_parodus_cfg()->hw_model},
            {HW_SERIALNUMBER, get_parodus_cfg()->hw_serial_number},
            {HW_MANUFACTURER, get_parodus_cfg()->hw_manufacturer},
            {HW_DEVICEMAC, get_parodus_cfg()->hw_mac},
            {HW_LAST_REBOOT_REASON, get_parodus_cfg()->hw_last_reboot_reason},
            {FIRMWARE_NAME , get_parodus_cfg()->fw_name},
            {BOOT_TIME, boot_time},
            {LAST_RECONNECT_REASON, reconnect_reason},
            {WEBPA_PROTOCOL, get_parodus_cfg()->webpa_protocol},
            {WEBPA_UUID,get_parodus_cfg()->webpa_uuid},
            {WEBPA_INTERFACE, get_parodus_cfg()->webpa_interface_used}
        };

    const data_t metapack = {METADATA_COUNT, meta_pack};

    metaPackSize = wrp_pack_metadata( &metapack , &metadataPack );

    if (metaPackSize > 0) 
    {
	    ParodusPrint("metadata encoding is successful with size %zu\n", metaPackSize);
    }
    else
    {
	    ParodusError("Failed to encode metadata\n");
    }
}

void getParodusUrl()
{
    const char *parodusIp = NULL;
    const char * envParodus = getenv ("PARODUS_SERVICE_URL");
    
    if( envParodus != NULL)
    {
      parodusIp = envParodus;
    }
    else
    {
      parodusIp = PARODUS_UPSTREAM ;
    }
    
    snprintf(parodus_url,sizeof(parodus_url),"%s", parodusIp);
    ParodusInfo("formatted parodus Url %s\n",parodus_url);
	
}
