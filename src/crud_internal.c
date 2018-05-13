/**
 * Copyright 2016 Comcast Cable Communications Management, LLC
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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cJSON.h>
#include <wrp-c.h>
#include "crud_tasks.h"
#include "crud_internal.h"
#include "config.h"

#define CRUD_CONFIG_FILE  "parodus_cfg.json"


static int writeToJSON(char *data)
{
    FILE *fp;
    fp = fopen(CRUD_CONFIG_FILE , "w+");
    if (fp == NULL) 
    {
        ParodusError("Failed to open file %s\n", CRUD_CONFIG_FILE );
        return 0;
    }
    fwrite(data, strlen(data), 1, fp);
    fclose(fp);
    return 1;
}

static int readFromJSON(char **data)
{
    FILE *fp;
    int ch_count = 0;
    fp = fopen(CRUD_CONFIG_FILE, "r+");
    if (fp == NULL) 
    {
        ParodusError("Failed to open file %s\n", CRUD_CONFIG_FILE);
        return 0;
    }
    fseek(fp, 0, SEEK_END);
	ch_count = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	*data = (char *) malloc(sizeof(char) * (ch_count + 1));
	fread(*data, 1, ch_count,fp);
	(*data)[ch_count] ='\0';
	fclose(fp);
	return 1;
}


int createObject( wrp_msg_t *reqMsg , wrp_msg_t **response)
{
	char *destVal = NULL;
	char *out = NULL;
	cJSON *parameters = NULL;
	cJSON *json, *jsonPayload = NULL;
	char *child_ptr,*obj[5];
	int objlevel = 1, i = 1, j=0;
	char *jsonData = NULL;
	cJSON *testObj1 = NULL;
	char *resPayload = NULL;

	ParodusInfo("Processing createObject\n");
	ParodusInfo("resp_msg->u.crud.dest is %s\n", reqMsg->u.crud.dest);
	ParodusInfo("reqMsg->u.crud.payload is %s\n", (char *)reqMsg->u.crud.payload);
	
	int status = readFromJSON(&jsonData);
	ParodusInfo("read status %d\n", status);
	json = cJSON_Parse( jsonData );

	if(reqMsg->u.crud.dest !=NULL)
	{
	   destVal = strdup(reqMsg->u.crud.dest);
	   ParodusInfo("destVal is %s\n", destVal);

	    if( (destVal != NULL))
	    {
	    	child_ptr = strtok(destVal , "/");
	    	
	    	/* Get the 1st object */
		obj[0] = strdup( child_ptr );
		ParodusPrint( "parent is %s\n", obj[0] );

		free(destVal);
	
		while( child_ptr != NULL ) 
		{
		    child_ptr = strtok( NULL, "/" );
		    if( child_ptr != NULL ) {
			obj[i] = strdup( child_ptr );
			ParodusPrint( "child obj[%d]:%s\n", i, obj[i] );
			i++;
		    }
		}

		objlevel = i;
		ParodusInfo( " Number of object level %d\n", objlevel );

		/* Valid request will be mac:14cfexxxx/parodus/tags/${name} which is objlevel 4 */
		if(objlevel == 4)
		{
			jsonPayload = cJSON_Parse( reqMsg->u.crud.payload );		
	
			cJSON* res_obj = cJSON_CreateObject();
			cJSON *payloadObj = cJSON_CreateObject();
	
			char *key = cJSON_GetArrayItem( jsonPayload, 0 )->string;

			int value = cJSON_GetArrayItem( jsonPayload, 0 )->valueint;
			ParodusInfo("key:%s value:%d\n", key, value);
	
			//check tags object exists
			cJSON *tagObj = cJSON_GetObjectItem( json, "tags" );
			if(tagObj !=NULL)
			{
				ParodusInfo("tag obj exists in json\n");
				//check requested test object exists under tags
				cJSON *testObj = cJSON_GetObjectItem( tagObj, obj[objlevel -1] );
		
				if(testObj !=NULL)
				{
					int jsontagitemSize = cJSON_GetArraySize( tagObj );
			                ParodusPrint( "jsontagitemSize is %d\n", jsontagitemSize );
			            
			            	//traverse through each test objects to find match
					for( i = 0 ; j < jsontagitemSize ; j++ ) 
					{
						char *testkey = cJSON_GetArrayItem( tagObj, j )->string;
						ParodusPrint("testkey is %s\n", testkey);
			
			
						if( strcmp( testkey, obj[objlevel -1] ) == 0 ) 
						{
							ParodusInfo( "testObj already exists in json. Update it\n" );
						    	cJSON_ReplaceItemInObject(testObj,key,cJSON_CreateNumber(value));
							cJSON_AddItemToObject(payloadObj, obj[objlevel -1] , testObj);
							resPayload = cJSON_PrintUnformatted(payloadObj);
							(*response)->u.crud.payload = resPayload;
							(*response)->u.crud.status = 201;
						    	break;
					    	}
						else
						{
							ParodusPrint("testObj not found, iterating..\n");
						}
					}
			
				}
				else
				{
					ParodusInfo("testObj not exists in json, adding it\n");
					cJSON_AddItemToObject(tagObj, obj[objlevel -1], testObj1 = cJSON_CreateObject());
					cJSON_AddNumberToObject(testObj1, key, value);

					cJSON_AddItemToObject(payloadObj, obj[objlevel -1], testObj1);
					resPayload = cJSON_PrintUnformatted(payloadObj);
					(*response)->u.crud.payload = resPayload;
			    		(*response)->u.crud.status = 201;
			
			
				}
			}
			else
			{
				ParodusInfo("tagObj doesnot exists in json, adding it\n");
				cJSON_AddItemToObject(res_obj , "tags", parameters = cJSON_CreateObject());
				cJSON_AddItemToObject(parameters, obj[objlevel -1], testObj1 = cJSON_CreateObject());
				cJSON_AddNumberToObject(testObj1, key, value);

				resPayload = cJSON_PrintUnformatted(parameters);
				(*response)->u.crud.payload = resPayload;
				(*response)->u.crud.status = 201;
						    
			}
	
			cJSON_AddItemToObject(res_obj , "tags", tagObj);
			out = cJSON_PrintUnformatted(res_obj );

			ParodusInfo("out : %s\n",out);
			int create_status = writeToJSON(out);
			if(create_status == 1)
			{
				ParodusInfo("Data is successfully added to JSON\n");
			}
			else
			{
				ParodusError("Failed to add data to JSON\n");
				(*response)->u.crud.status = 500;
				return -1;
			}
		}
		else
		{
			//  Return error for request format other than /tag/${name}
			ParodusError("Invalid CREATE request\n");
			(*response)->u.crud.status = 400;
			return -1;
		}

	    } 
	    else
	    {
		ParodusError("Unable to parse object details from CREATE request\n");
		(*response)->u.crud.status = 400;
		return -1;
	    } 
	}
	else
	{
		ParodusError("Requested dest path is NULL\n");
		(*response)->u.crud.status = 400;
		return -1;
	}
	

	return 0;
}

int retrieveFromMemory(char *keyName, cJSON **jsonresponse)
{
	*jsonresponse = cJSON_CreateObject();
	
	if(strcmp(HW_MODELNAME, keyName)==0)
	{
		 if(get_parodus_cfg()->hw_model ==NULL)
		 {
		 	ParodusError("retrieveFromMemory: hw_model value is NULL\n");
		 	return -1;
		 }
		 ParodusInfo("retrieveFromMemory: keyName: %s value: %s\n",keyName,get_parodus_cfg()->hw_model);
		 cJSON_AddItemToObject(*jsonresponse, HW_MODELNAME, cJSON_CreateString(get_parodus_cfg()->hw_model));
	}
	else if(strcmp(HW_SERIALNUMBER, keyName)==0)
	{
		if(get_parodus_cfg()->hw_serial_number ==NULL)
		{
			ParodusError("retrieveFromMemory: hw_serial_number value is NULL\n");
		 	return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,get_parodus_cfg()->hw_serial_number);
		cJSON_AddItemToObject( *jsonresponse, HW_SERIALNUMBER , cJSON_CreateString(get_parodus_cfg()->hw_serial_number));
	}
	else if(strcmp(HW_MANUFACTURER, keyName)==0)
	{
		if(get_parodus_cfg()->hw_manufacturer ==NULL)
		{
			ParodusError("retrieveFromMemory: hw_manufacturer value is NULL\n");
		 	return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,get_parodus_cfg()->hw_manufacturer);
		cJSON_AddItemToObject( *jsonresponse, HW_MANUFACTURER , cJSON_CreateString(get_parodus_cfg()->hw_manufacturer));
	}
	else if(strcmp(HW_DEVICEMAC, keyName)==0)
	{
		if(get_parodus_cfg()->hw_mac ==NULL)
		{
			ParodusError("retrieveFromMemory: hw_mac value is NULL\n");
		 	return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,get_parodus_cfg()->hw_mac);
		cJSON_AddItemToObject( *jsonresponse, HW_DEVICEMAC , cJSON_CreateString(get_parodus_cfg()->hw_mac));
	}
	else if(strcmp(HW_LAST_REBOOT_REASON, keyName)==0)
	{
		if(get_parodus_cfg()->hw_last_reboot_reason ==NULL)
		{
			ParodusError("retrieveFromMemory: hw_last_reboot_reason value is NULL\n");
		 	return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,get_parodus_cfg()->hw_last_reboot_reason);
		cJSON_AddItemToObject( *jsonresponse, HW_LAST_REBOOT_REASON , cJSON_CreateString(get_parodus_cfg()->hw_last_reboot_reason));
	}
	else if(strcmp(FIRMWARE_NAME, keyName)==0)
	{
		if(get_parodus_cfg()->fw_name ==NULL)
		{
			ParodusError("retrieveFromMemory: fw_name value is NULL\n");
		 	return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,get_parodus_cfg()->fw_name);
		cJSON_AddItemToObject( *jsonresponse, FIRMWARE_NAME , cJSON_CreateString(get_parodus_cfg()->fw_name));
	}
	else if(strcmp(WEBPA_INTERFACE, keyName)==0)
	{
		if(get_parodus_cfg()->webpa_interface_used ==NULL)
		{
			ParodusError("retrieveFromMemory: webpa_interface_used value is NULL\n");
		 	return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,get_parodus_cfg()->webpa_interface_used);
		cJSON_AddItemToObject( *jsonresponse, WEBPA_INTERFACE , cJSON_CreateString(get_parodus_cfg()->webpa_interface_used));
	}
	else if(strcmp(WEBPA_URL, keyName)==0)
	{
		if(get_parodus_cfg()->webpa_url ==NULL)
		{
			ParodusError("retrieveFromMemory: webpa_url value is NULL\n");
		 	return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,get_parodus_cfg()->webpa_url);
		cJSON_AddItemToObject( *jsonresponse, WEBPA_URL , cJSON_CreateString(get_parodus_cfg()->webpa_url));
	}
	else if(strcmp(WEBPA_PROTOCOL, keyName)==0)
	{
		if(get_parodus_cfg()->webpa_protocol ==NULL)
		{
			ParodusError("retrieveFromMemory: webpa_protocol value is NULL\n");
		 	return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,get_parodus_cfg()->webpa_protocol);
		cJSON_AddItemToObject( *jsonresponse, WEBPA_PROTOCOL , cJSON_CreateString(get_parodus_cfg()->webpa_protocol));
	}
	else if(strcmp(WEBPA_UUID, keyName)==0)
	{
		if(get_parodus_cfg()->webpa_uuid ==NULL)
		{
			ParodusError("retrieveFromMemory: webpa_uuid value is NULL\n");
		 	return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,get_parodus_cfg()->webpa_uuid);
		cJSON_AddItemToObject( *jsonresponse, WEBPA_UUID , cJSON_CreateString(get_parodus_cfg()->webpa_uuid));
	}
	else if(strcmp(BOOT_TIME, keyName)==0)
	{
		ParodusInfo("retrieveFromMemory: keyName:%s value:%d\n",keyName,get_parodus_cfg()->boot_time);
		cJSON_AddItemToObject( *jsonresponse, BOOT_TIME , cJSON_CreateNumber(get_parodus_cfg()->boot_time));
	}
	else if(strcmp(WEBPA_PING_TIMEOUT , keyName)==0)
	{
		ParodusInfo("retrieveFromMemory: keyName:%s value:%d\n",keyName,get_parodus_cfg()->webpa_ping_timeout);
		cJSON_AddItemToObject( *jsonresponse, WEBPA_PING_TIMEOUT , cJSON_CreateNumber(get_parodus_cfg()->webpa_ping_timeout));
	}
	else if(strcmp(WEBPA_BACKOFF_MAX, keyName)==0)
	{
		ParodusInfo("retrieveFromMemory: keyName:%s value:%d\n",keyName,get_parodus_cfg()->webpa_backoff_max);
		cJSON_AddItemToObject( *jsonresponse, WEBPA_BACKOFF_MAX , cJSON_CreateNumber(get_parodus_cfg()->webpa_backoff_max));
	}
	else
	{
		ParodusError("Invalid retrieve key object\n");
		return -1;
	}
	
	return 0;
}

int retrieveObject( wrp_msg_t *reqMsg, wrp_msg_t **response )
{
	char *destVal = NULL;
	char *jsonData = NULL;
	char *child_ptr,*obj[5];
	int objlevel = 1, i = 1;
	cJSON *inMemResponse;
	int status, inMemStatus = -1;
	
	status = readFromJSON(&jsonData);
	ParodusInfo("read status %d\n", status);
        cJSON_Parse( jsonData );
        	
	if(reqMsg->u.crud.dest !=NULL)
	{
	   destVal = strdup(reqMsg->u.crud.dest);
	   ParodusInfo("destVal is %s\n", destVal);

	    if( (destVal != NULL))
	    {
	    	child_ptr = strtok(destVal , "/");
	    	
	    	//Get the 1st object
		obj[0] = strdup( child_ptr );
		ParodusInfo( "parent is %s\n", obj[0] );
		
		
		while( child_ptr != NULL ) 
		{
		    child_ptr = strtok( NULL, "/" );
		    if( child_ptr != NULL ) {
		        obj[i] = strdup( child_ptr );
		        ParodusInfo( "child obj[%d]:%s\n", i, obj[i] );
		        i++;
		    }
		}
		
		objlevel = i;
		ParodusInfo( "Number of object level %d\n", objlevel );
		
		if( (objlevel == 3) && ((obj[2] !=NULL) && strcmp( obj[2] , "tag") !=0 ))
		{
			inMemStatus = retrieveFromMemory(obj[2], &inMemResponse );
			
			if(inMemStatus == 0)
			{
				ParodusInfo("inMemory retrieve returns success \n");
        			char *inmem_str = cJSON_PrintUnformatted( inMemResponse );
            			ParodusInfo( "inMemResponse: %s\n", inmem_str );
            			(*response)->u.crud.status = 201;
            			(*response)->u.crud.payload = inmem_str;
            			cJSON_Delete( inMemResponse );
        		}
        		else
        		{
        			ParodusError("Failed to retrieve inMemory value \n");
            			(*response)->u.crud.status = 400;
            			return -1;
        		}
		}
		else
		{
			ParodusInfo("Processing CRUD external tag request \n");
			return -1;
		}
	    }
	    else
	    {
		ParodusError("Unable to parse object details from RETRIEVE request\n");
		return -1;
	    }
	}
			    
	return 0;
}

int updateObject(  )
{
	return 0;
}

int deleteObject(  )
{
	return 0;
}

