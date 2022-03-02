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
#include "connection.h"
#include "close_retry.h"
#include "client_list.h"

#define SERVICE_STATUS "service-status"

static void freeObjArray(char *(*obj)[], int size);
static int writeIntoCrudJson(cJSON *res_obj, char * object, cJSON *objValue, int freeFlag);
static int parse_dest_elements_to_string(wrp_msg_t *reqMsg, char *(*obj)[]);
static char* strdupptr( const char *s, const char *e );
static int ConnDisconnectFromCloud(char *reason);
static int validateDisconnectString(char *reason);
static int getClientStatus(char *service, cJSON **jsonresponse);

int writeToJSON(char *data)
{
	FILE *fp;
	fp = fopen(get_parodus_cfg()->crud_config_file , "w+");
	if (fp == NULL)
	{
		ParodusError("Failed to open file %s\n", get_parodus_cfg()->crud_config_file );
		return 0;
	}
	if(data !=NULL)
	{
		fwrite(data, strlen(data), 1, fp);
		fclose(fp);
		return 1;
	}
	else
	{
		ParodusError("WriteToJson failed, Data is NULL\n");
		return 0;
	}
}

int readFromJSON(char **data)
{
	return readFromFile (get_parodus_cfg()->crud_config_file, data);
}
/*
*	@res_obj 	json object to add it in crud config json file
*	@object 	parent json obj name i.e tags
*	@objValue	child json obj to be added to parent i.e test
*	@freeFlag	Based on this flag, writeIntoCrudJson decides to do free/cjson_delete for the json object
*/
int writeIntoCrudJson(cJSON *res_obj, char * object, cJSON *objValue, int freeFlag)
{
	char *out = NULL;
	int write_status = 0;
	cJSON_AddItemToObject(res_obj , object, objValue);
	out = cJSON_PrintUnformatted(res_obj );
	ParodusPrint("out : %s\n",out);

	write_status = writeToJSON(out);

	if(out !=NULL)
	{
		free( out );
	}
	if(res_obj !=NULL && freeFlag)
	{
		cJSON_Delete(res_obj);
	}
	else if(res_obj != NULL)
	{
		free(res_obj);
	}
	return write_status;
}

int createObject( wrp_msg_t *reqMsg , wrp_msg_t **response)
{
	cJSON *json, *jsonPayload = NULL;
	char *obj[5];
	int objlevel = 0, j=0, i =0;
	char *jsonData = NULL;
	cJSON *testObj1 = NULL;
	char *resPayload = NULL;
	int jsontagitemSize, create_status =0;
	int value, status, jsonPayloadSize =0;
	char *key = NULL, *testkey = NULL;
	const char*parse_error =NULL;
	int expireFlag = 0;

	ParodusInfo("Processing createObject\n");

	status = readFromJSON(&jsonData);
	ParodusPrint("read status %d\n", status);

	if(status == 0)
	{
		ParodusInfo("Proceed creating CRUD config %s\n", get_parodus_cfg()->crud_config_file );
		json = cJSON_Parse( jsonData );
	}
	else
	{
		if((jsonData !=NULL) && (strlen(jsonData)>0))
		{
			json = cJSON_Parse( jsonData );
			if( json == NULL )
			{
				parse_error = cJSON_GetErrorPtr();
				ParodusError("Parse Error before: %s\n", parse_error);
				if(jsonData !=NULL)
				{
					free( jsonData );
					jsonData = NULL;
				}
				(*response)->u.crud.status = 500;
				return -1;
			}
			else
			{
				ParodusInfo("CRUD config json parse success\n");
			}
		}
		else
		{
			ParodusInfo("CRUD config is empty, proceed creation of new object\n");
			json = cJSON_Parse( jsonData );
		}
	}

	if(jsonData !=NULL)
	{
		free( jsonData );
		jsonData = NULL;
	}

	if(reqMsg->u.crud.dest !=NULL)
	{
		ParodusInfo("reqMsg->u.crud.dest is %s\n", reqMsg->u.crud.dest);

		objlevel = parse_dest_elements_to_string(reqMsg, &obj);
		if(objlevel < 0)
		{
			(*response)->u.crud.status = 400;
			cJSON_Delete( json);
			return -1;
		}

		ParodusInfo( "Number of object level %d\n", objlevel);

		/* Valid request will be mac:14cfexxxx/parodus/tag/${name} which is objlevel 4 */
		if(objlevel == 4 && ((obj[2] != NULL) && (strcmp(obj[2] ,  "parodus") == 0) ) && ((obj[3] != NULL) &&(strcmp(obj[3] ,  "tag") == 0 )))
		{
			if(reqMsg->u.crud.payload != NULL)
			{
				ParodusInfo("reqMsg->u.crud.payload is %s\n", (char *)reqMsg->u.crud.payload);
				jsonPayload = cJSON_Parse( reqMsg->u.crud.payload );
				if(jsonPayload !=NULL)
				{
					jsonPayloadSize = cJSON_GetArraySize( jsonPayload );
					ParodusPrint( "jsonPayloadSize is %d\n", jsonPayloadSize );
					if(jsonPayloadSize)
					{
						cJSON* res_obj = NULL;

						/* checking the mandatory field "expires" key in request payload */
						for (i =0 ; i < jsonPayloadSize ; i++)
						{
							key= cJSON_GetArrayItem( jsonPayload, i )->string;

							if (key && (cJSON_Number == cJSON_GetArrayItem( jsonPayload, i )->type))
							{
								value = cJSON_GetArrayItem( jsonPayload, i )->valueint;
								ParodusPrint("key:%s value:%d\n", key, value);
								if(strcmp(key, "expires") == 0 )
								{
									ParodusPrint("Found expires key \n");
									if(!value)
									{
										ParodusError("Invalid request. Incorrect expires value\n");
										(*response)->u.crud.status = 400;
										cJSON_Delete( jsonPayload );
										jsonPayload = NULL;
										cJSON_Delete(json);
										json = NULL;
										freeObjArray(&obj, objlevel);
										return -1;
									}
									else
									{
										expireFlag = 1;
										break;
									}
								}
								else
								{
									ParodusPrint("expires key not found in payload, iterating ..\n");
								}
							}
						}

						if(!expireFlag )
						{
							ParodusError("Invalid request. Numeric expires value is mandatory in payload\n");
							(*response)->u.crud.status = 400;
							cJSON_Delete( jsonPayload );
							jsonPayload = NULL;
							cJSON_Delete(json);
							json = NULL;
							freeObjArray(&obj, objlevel);
							return -1;
						}

						/* checking for proper string values in request payload */
						for (i =0 ; i < jsonPayloadSize ; i++)
						{
							if(cJSON_GetArrayItem( jsonPayload, i )->string != NULL && strlen(cJSON_GetArrayItem( jsonPayload, i )->string) == 0)
							{
								ParodusError("Invalid Request. Key is NULL in request payload\n");
								(*response)->u.crud.status = 400;
								cJSON_Delete( jsonPayload );
								jsonPayload = NULL;
								cJSON_Delete(json);
								json = NULL;
								freeObjArray(&obj, objlevel);
								return -1;
							}
							else
							{
								if (cJSON_String == cJSON_GetArrayItem( jsonPayload, i )->type)
								{
									if(cJSON_GetArrayItem( jsonPayload, i )->valuestring != NULL && strlen(cJSON_GetArrayItem( jsonPayload, i )->valuestring) == 0)
									{
										ParodusError("Invalid request. Object value is NULL\n");
										(*response)->u.crud.status = 400;
										cJSON_Delete( jsonPayload );
										jsonPayload = NULL;
										cJSON_Delete(json);
										json = NULL;
										freeObjArray(&obj, objlevel);
										return -1;
									}
									else
									{
										ParodusPrint("key:%s value:%s\n", cJSON_GetArrayItem( jsonPayload, i )->string, cJSON_GetArrayItem( jsonPayload, i )->valuestring);
									}
								}
							}
						}

						//check tags object exists
						cJSON *tagObj = cJSON_GetObjectItem( json, "tags" );
						if(tagObj !=NULL)
						{
							ParodusInfo("tag obj exists in json\n");
							//check requested test object exists under tags
							cJSON *testObj = cJSON_GetObjectItem( tagObj, obj[objlevel] );
							if(testObj !=NULL)
							{
								jsontagitemSize = cJSON_GetArraySize( tagObj );
								ParodusPrint( "jsontagitemSize is %d\n", jsontagitemSize );
								//traverse through each test objects to find match
								for( j = 0 ; j < jsontagitemSize ; j++ )
								{
									testkey = cJSON_GetArrayItem( tagObj, j )->string;
									ParodusPrint("testkey is %s\n", testkey);
									if( strcmp( testkey, obj[objlevel] ) == 0 )
									{
										ParodusError( "Create is not allowed. testObj %s is already exists in json\n", testkey );
										(*response)->u.crud.status = 409;
										cJSON_Delete( jsonPayload );
										jsonPayload = NULL;
										cJSON_Delete(json);
										json = NULL;
										freeObjArray(&obj, objlevel);
										return -1;
									}
									else
									{
										ParodusPrint("testObj not found, iterating..\n");
									}
								}
							}
							else
							{
								ParodusInfo("testObj doesnot exists in json, adding it\n");
								res_obj = cJSON_CreateObject();
								//To add into crud json config file
								cJSON_AddItemToObject(tagObj, obj[objlevel], testObj1 = cJSON_CreateObject());

								for (i =0 ; i < jsonPayloadSize ; i++)
								{
									if (cJSON_Number == cJSON_GetArrayItem( jsonPayload, i )->type)
									{
										cJSON_AddNumberToObject(testObj1, cJSON_GetArrayItem( jsonPayload, i )->string, cJSON_GetArrayItem( jsonPayload, i )->valueint);
									}
									else if (cJSON_String == cJSON_GetArrayItem( jsonPayload, i )->type)
									{
										cJSON_AddStringToObject(testObj1, cJSON_GetArrayItem( jsonPayload, i )->string, cJSON_GetArrayItem( jsonPayload, i )->valuestring);
									}
									else
									{
										ParodusError("Invalid Type in request payload\n");
										(*response)->u.crud.status = 400;
										cJSON_Delete(res_obj);
										res_obj = NULL;
										cJSON_Delete( jsonPayload );
										jsonPayload = NULL;
										cJSON_Delete(json);
										json = NULL;
										freeObjArray(&obj, objlevel);
										return -1;
									}
								}

								//To add into response payload
								cJSON *payloadObj = cJSON_CreateObject();
								cJSON * tmpObj = NULL;
								cJSON_AddItemToObject(payloadObj, obj[objlevel], tmpObj = cJSON_CreateObject());

								for (i =0 ; i < jsonPayloadSize ; i++)
								{
									if (cJSON_Number == cJSON_GetArrayItem( jsonPayload, i )->type)
									{
										cJSON_AddNumberToObject(tmpObj, cJSON_GetArrayItem( jsonPayload, i )->string, cJSON_GetArrayItem( jsonPayload, i )->valueint);
									}
									else
									{
										cJSON_AddStringToObject(tmpObj, cJSON_GetArrayItem( jsonPayload, i )->string, cJSON_GetArrayItem( jsonPayload, i )->valuestring);
									}
								}

								resPayload = cJSON_PrintUnformatted(payloadObj);
								cJSON_Delete( payloadObj );
								(*response)->u.crud.payload = resPayload;
								(*response)->u.crud.payload_size = strlen(resPayload);
								(*response)->u.crud.status = 201;
								//Pass freeflag as 0 if you add new child obj to parent obj i.e test obj creation
								create_status = writeIntoCrudJson(res_obj,"tags", tagObj, 0);
							}
						}
						else
						{
							ParodusInfo("tagObj doesnot exists in json, adding it\n");
							//To add into crud json config file
							res_obj = cJSON_CreateObject();
							tagObj = cJSON_CreateObject();
							cJSON_AddItemToObject(tagObj, obj[objlevel], testObj1 = cJSON_CreateObject());

							for (i =0 ; i < jsonPayloadSize ; i++)
							{
								if (cJSON_Number == cJSON_GetArrayItem( jsonPayload, i )->type)
								{
									cJSON_AddNumberToObject(testObj1, cJSON_GetArrayItem( jsonPayload, i )->string, cJSON_GetArrayItem( jsonPayload, i )->valueint);
								}
								else if (cJSON_String == cJSON_GetArrayItem( jsonPayload, i )->type)
								{
									cJSON_AddStringToObject(testObj1, cJSON_GetArrayItem( jsonPayload, i )->string, cJSON_GetArrayItem( jsonPayload, i )->valuestring);
								}
								else
								{
									ParodusError("Invalid Type in request payload\n");
									(*response)->u.crud.status = 400;
									cJSON_Delete(res_obj);
									res_obj = NULL;
									cJSON_Delete(tagObj);
									tagObj = NULL;
									cJSON_Delete( jsonPayload );
									jsonPayload = NULL;
									cJSON_Delete(json);
									json = NULL;
									freeObjArray(&obj, objlevel);
									return -1;
								}
							}

							//To add into response payload
							resPayload = cJSON_PrintUnformatted(tagObj);
							(*response)->u.crud.payload = resPayload;
							(*response)->u.crud.payload_size = strlen(resPayload);
							(*response)->u.crud.status = 201;
							//Pass freeflag as 1 if you create new parent json object i.e tag obj creation
							create_status = writeIntoCrudJson(res_obj,"tags", tagObj, 1);
						}

						cJSON_Delete( jsonPayload );
						jsonPayload = NULL;
						cJSON_Delete(json);
						json = NULL;
						freeObjArray(&obj, objlevel);

						if(create_status == 1)
						{
							ParodusPrint("Data is successfully added to JSON\n");
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
						ParodusError("Invalid CREATE request, payload size is 0\n");
						(*response)->u.crud.status = 400;
						freeObjArray(&obj, objlevel);
						cJSON_Delete( jsonPayload );
						jsonPayload = NULL;
						cJSON_Delete( json);
						return -1;
					}
				}
				else
				{
					ParodusError("Invalid CREATE request, payload is not json\n");
					(*response)->u.crud.status = 400;
					freeObjArray(&obj, objlevel);
					cJSON_Delete( json);
					return -1;
				}
			}
			else
			{
				ParodusError("Invalid CREATE request, payload is NULL\n");
				(*response)->u.crud.status = 400;
				freeObjArray(&obj, objlevel);
				cJSON_Delete( json );
				return -1;
			}
		}
		else
		{
			//  Return error for request format other than parodus/tag/${name}
			ParodusError("Invalid CREATE request\n");
			(*response)->u.crud.status = 400;
			freeObjArray(&obj, objlevel);
			cJSON_Delete( json );
			return -1;
		}
	}
	else
	{
		ParodusError("Requested dest path is NULL\n");
		(*response)->u.crud.status = 400;
		cJSON_Delete( json );
		return -1;
	}
	return 0;
}


// To retrieve from in-memory read only config list
int retrieveFromMemory(char *keyName, cJSON **jsonresponse)
{
	*jsonresponse = cJSON_CreateObject();

	if(strcmp(HW_MODELNAME, keyName)==0)
	{
		if((get_parodus_cfg()->hw_model !=NULL) && (strlen(get_parodus_cfg()->hw_model)==0))
		{
			ParodusError("retrieveFromMemory: hw_model value is NULL\n");
			return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName: %s value: %s\n",keyName,get_parodus_cfg()->hw_model);
		cJSON_AddItemToObject(*jsonresponse, HW_MODELNAME, cJSON_CreateString(get_parodus_cfg()->hw_model));
	}
	else if(strcmp(HW_SERIALNUMBER, keyName)==0)
	{
		if((get_parodus_cfg()->hw_serial_number !=NULL) && (strlen(get_parodus_cfg()->hw_serial_number)==0))
		{
			ParodusError("retrieveFromMemory: hw_serial_number value is NULL\n");
			return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,get_parodus_cfg()->hw_serial_number);
		cJSON_AddItemToObject( *jsonresponse, HW_SERIALNUMBER , cJSON_CreateString(get_parodus_cfg()->hw_serial_number));
	}
	else if(strcmp(HW_MANUFACTURER, keyName)==0)
	{
		if((get_parodus_cfg()->hw_manufacturer !=NULL) && (strlen(get_parodus_cfg()->hw_manufacturer)==0))
		{
			ParodusError("retrieveFromMemory: hw_manufacturer value is NULL\n");
			return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,get_parodus_cfg()->hw_manufacturer);
		cJSON_AddItemToObject( *jsonresponse, HW_MANUFACTURER , cJSON_CreateString(get_parodus_cfg()->hw_manufacturer));
	}
	else if(strcmp(HW_DEVICEMAC, keyName)==0)
	{
		if((get_parodus_cfg()->hw_mac !=NULL) && (strlen(get_parodus_cfg()->hw_mac)==0))
		{
			ParodusError("retrieveFromMemory: hw_mac value is NULL\n");
			return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,get_parodus_cfg()->hw_mac);
		cJSON_AddItemToObject( *jsonresponse, HW_DEVICEMAC , cJSON_CreateString(get_parodus_cfg()->hw_mac));
	}
	else if(strcmp(HW_LAST_REBOOT_REASON, keyName)==0)
	{
		if((get_parodus_cfg()->hw_last_reboot_reason !=NULL) && (strlen(get_parodus_cfg()->hw_last_reboot_reason)==0))
		{
			ParodusError("retrieveFromMemory: hw_last_reboot_reason value is NULL\n");
			return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,get_parodus_cfg()->hw_last_reboot_reason);
		cJSON_AddItemToObject( *jsonresponse, HW_LAST_REBOOT_REASON , cJSON_CreateString(get_parodus_cfg()->hw_last_reboot_reason));
	}
	else if(strcmp(FIRMWARE_NAME,keyName)==0) 
	{
		if((get_parodus_cfg()->fw_name !=NULL) && (strlen(get_parodus_cfg()->fw_name)==0))
		{
			ParodusError("retrieveFromMemory: fw_name value is NULL\n");
			return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,get_parodus_cfg()->fw_name);
		cJSON_AddItemToObject( *jsonresponse, FIRMWARE_NAME , cJSON_CreateString(get_parodus_cfg()->fw_name));
	}
	else if(strcmp(WEBPA_INTERFACE, keyName)==0)
	{
		if((getWebpaInterface() !=NULL)&& (strlen(get_parodus_cfg()->fw_name)==0))
		{
			ParodusError("retrieveFromMemory: webpa_interface_used value is NULL\n");
			return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,getWebpaInterface());
		cJSON_AddItemToObject( *jsonresponse, WEBPA_INTERFACE , cJSON_CreateString(getWebpaInterface()));
	}
	else if(strcmp(WEBPA_URL, keyName)==0)
	{
		if((get_parodus_cfg()->webpa_url !=NULL) && (strlen(get_parodus_cfg()->webpa_url)==0))
		{
			ParodusError("retrieveFromMemory: webpa_url value is NULL\n");
			return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,get_parodus_cfg()->webpa_url);
		cJSON_AddItemToObject( *jsonresponse, WEBPA_URL , cJSON_CreateString(get_parodus_cfg()->webpa_url));
	}
	else if(strcmp(WEBPA_PROTOCOL, keyName)==0)
	{
		if((get_parodus_cfg()->webpa_protocol !=NULL)  && (strlen(get_parodus_cfg()->webpa_protocol)==0))
		{
			ParodusError("retrieveFromMemory: webpa_protocol value is NULL\n");
			return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,get_parodus_cfg()->webpa_protocol);
		cJSON_AddItemToObject( *jsonresponse, WEBPA_PROTOCOL , cJSON_CreateString(get_parodus_cfg()->webpa_protocol));
	}
	else if(strcmp(WEBPA_UUID, keyName)==0)
	{
		if((get_parodus_cfg()->webpa_uuid !=NULL) && (strlen(get_parodus_cfg()->webpa_uuid)==0))
		{
			ParodusError("retrieveFromMemory: webpa_uuid value is NULL\n");
			return -1;
		}
		ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n",keyName,get_parodus_cfg()->webpa_uuid);
		cJSON_AddItemToObject( *jsonresponse, WEBPA_UUID , cJSON_CreateString(get_parodus_cfg()->webpa_uuid));
	}
	else if(strcmp(CLOUD_STATUS, keyName)==0)
	{
		if(get_parodus_cfg()->cloud_status ==NULL)
		{
			ParodusError("retrieveFromMemory: cloud_status value is NULL\n");
			return -1;
		}
		else if((get_parodus_cfg()->cloud_status !=NULL) && (strlen(get_parodus_cfg()->cloud_status)==0))
		{
			ParodusError("retrieveFromMemory: cloud_status value is empty\n");
			return -1;
		}
		else
		{
			ParodusInfo("retrieveFromMemory: keyName:%s value:%s\n", keyName, get_parodus_cfg()->cloud_status);
			cJSON_AddItemToObject( *jsonresponse, CLOUD_STATUS , cJSON_CreateString(get_parodus_cfg()->cloud_status));
		}
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
		ParodusError("Invalid retrieve key object: %s\n", keyName);
		return -1;
	}

	return 0;
}


int retrieveObject( wrp_msg_t *reqMsg, wrp_msg_t **response )
{
	cJSON *paramArray = NULL;
	cJSON *json = NULL, *childObj = NULL, *subitemObj =NULL;
	char *jsonData = NULL;
	char *obj[5];
	int objlevel = 0, i = 0, j=0, found = 0, status;
	cJSON *inMemResponse = NULL;
	int inMemStatus = -1, itemSize =0;
	char *str1 = NULL;
	const char *parse_error = NULL;

	if(reqMsg->u.crud.dest !=NULL)
	{
		ParodusInfo("reqMsg->u.crud.dest is %s\n", reqMsg->u.crud.dest);

		objlevel = parse_dest_elements_to_string(reqMsg, &obj);
		if(objlevel < 0)
		{
			(*response)->u.crud.status = 400;
			cJSON_Delete( json);
			return -1;
		}

		ParodusInfo( "Number of object level %d\n", objlevel );
		if((objlevel == 3 && ((obj[3] !=NULL) && strstr(obj[3] ,"tags") == NULL)) || (objlevel == 4 && ((obj[3] !=NULL) && strstr(obj[3] ,"service-status") != NULL)))
		{
			//To support dest "mac:14xxxxxxxxxx/parodus/service-status/service" to retrieve online/offline status of registered clients.
			if(strstr(obj[3] ,"service-status") !=NULL)
			{
				inMemStatus = getClientStatus(obj[4], &inMemResponse );
			}
			else
			{
				inMemStatus = retrieveFromMemory(obj[3], &inMemResponse );
			}

			if(inMemStatus == 0)
			{
				ParodusInfo("inMemory retrieve returns success \n");
				char *inmem_str = cJSON_PrintUnformatted( inMemResponse );
				ParodusInfo( "inMemResponse: %s\n", inmem_str );
				(*response)->u.crud.status = 200;
				(*response)->u.crud.payload = inmem_str;
				(*response)->u.crud.payload_size = strlen(inmem_str);
				cJSON_Delete( inMemResponse );
				freeObjArray(&obj, objlevel);
			}
			else
			{
				ParodusError("Failed to retrieve inMemory value \n");
				(*response)->u.crud.status = 400;
				cJSON_Delete( inMemResponse );
				freeObjArray(&obj, objlevel);
				return -1;
			}
		}
		else
		{
			ParodusInfo("Processing CRUD external tag request \n");

			status = readFromJSON(&jsonData);
			ParodusPrint("read status %d\n", status);

			if(status)
			{
				if((jsonData !=NULL) && (strlen(jsonData)>0))
				{
					json = cJSON_Parse( jsonData );
					if(jsonData !=NULL)
					{
						free( jsonData );
						jsonData = NULL;
					}
					if( json == NULL )
					{
						parse_error = cJSON_GetErrorPtr();
						if (parse_error != NULL)
						{
							ParodusError("Parse Error before: %s\n", parse_error);
						}
						(*response)->u.crud.status = 500;
						freeObjArray(&obj, objlevel);
						return -1;
					}
					else
					{
						ParodusInfo("CRUD config json parse success\n");
						cJSON *jsonresponse = cJSON_CreateObject();
						paramArray = cJSON_GetObjectItem( json, "tags" );
						if( paramArray != NULL )
						{
							itemSize = cJSON_GetArraySize( paramArray );
							if( itemSize == 0 )
							{
								ParodusError("itemSize is 0, tags object is empty in json\n");
								(*response)->u.crud.status = 400;
								cJSON_Delete( jsonresponse );
								cJSON_Delete( json );
								freeObjArray(&obj, objlevel);
								return -1;
							}
							else
							{
								//To retrieve top level tags object
								if( strcmp( cJSON_GetObjectItem( json, "tags" )->string, obj[objlevel] ) == 0 )
								{
									ParodusInfo("top level tags object\n");
									cJSON_AddItemToObject( jsonresponse, obj[objlevel ] , childObj = cJSON_CreateObject());
									//To add test objects to jsonresponse
									for( i = 0; i < itemSize; i++ )
									{
										cJSON* subitem = cJSON_GetArrayItem( paramArray, i );
										int subitemSize = cJSON_GetArraySize( subitem );
										cJSON_AddItemToObject( childObj, cJSON_GetArrayItem( paramArray, i )->string, subitemObj = cJSON_CreateObject() );
										//To add subitem objects to jsonresponse
										for( j = 0 ; j < subitemSize ; j++ )
										{
											if (cJSON_Number == cJSON_GetArrayItem( subitem, j )->type)
											{
												ParodusPrint( " %s : %d \n", cJSON_GetArrayItem( subitem, j )->string, cJSON_GetArrayItem( subitem, j )->valueint );
												cJSON_AddItemToObject( subitemObj, cJSON_GetArrayItem( subitem, j )->string, cJSON_CreateNumber(cJSON_GetArrayItem( subitem, j )->valueint));
											}
											else
											{
												ParodusPrint( " %s : %s \n", cJSON_GetArrayItem( subitem, j )->string, cJSON_GetArrayItem( subitem, j )->valuestring );
												cJSON_AddItemToObject( subitemObj, cJSON_GetArrayItem( subitem, j )->string, cJSON_CreateString(cJSON_GetArrayItem( subitem, j )->valuestring));
											}
										}
									}
									cJSON *tagObj = cJSON_GetObjectItem( jsonresponse, "tags" );
									str1 = cJSON_PrintUnformatted( tagObj );
									ParodusInfo( "jsonResponse %s\n", str1 );
									(*response)->u.crud.status = 200;
									(*response)->u.crud.payload = str1;
									(*response)->u.crud.payload_size = strlen(str1);
								}
								else
								{
									if ((obj[3] !=NULL) && (strcmp(obj[3] ,  "tag") == 0))
									{
										//To traverse through total number of test objects in json
										for( i = 0 ; i < itemSize ; i++ )
										{
											cJSON* subitem = cJSON_GetArrayItem( paramArray, i );
											int subitemSize = cJSON_GetArraySize( subitem );
											if( strcmp( cJSON_GetArrayItem( paramArray, i )->string, obj[objlevel] ) == 0 )
											{
												//To add subitem objects to jsonresponse
												for( j = 0 ; j < subitemSize ; j++ )
												{
													//retrieve test object value
													if (cJSON_Number == cJSON_GetArrayItem( subitem, j )->type)
													{
														ParodusPrint( " %s : %d \n", cJSON_GetArrayItem( subitem, j )->string, cJSON_GetArrayItem( subitem, j )->valueint );
														cJSON_AddItemToObject( jsonresponse, cJSON_GetArrayItem( subitem, j )->string , cJSON_CreateNumber(cJSON_GetArrayItem( subitem, j )->valueint));
													}
													else
													{
														ParodusPrint( " %s : %s \n", cJSON_GetArrayItem( subitem, j )->string, cJSON_GetArrayItem( subitem, j )->valuestring );
														cJSON_AddItemToObject( jsonresponse, cJSON_GetArrayItem( subitem, j )->string , cJSON_CreateString(cJSON_GetArrayItem( subitem, j )->valuestring));
													}
												}
												ParodusInfo("Retrieve: requested object found \n");
												found = 1;
												break;
											}
											else
											{
												ParodusPrint("retrieve object not found, traversing\n");
											}
										}
										if(!found)
										{
											ParodusError("Unable to retrieve requested object\n");
											(*response)->u.crud.status = 400;
											cJSON_Delete( jsonresponse );
											cJSON_Delete( json );
											freeObjArray(&obj, objlevel);
											return -1;
										}

										char *str1 = cJSON_PrintUnformatted( jsonresponse );
										ParodusInfo( "jsonResponse %s\n", str1 );
										(*response)->u.crud.status = 200;
										(*response)->u.crud.payload = str1;
										(*response)->u.crud.payload_size = strlen(str1);
									}
									else
									{
										//  Return error for request format other than parodus/tag/${name}
										ParodusError("Invalid RETRIEVE request\n");
										(*response)->u.crud.status = 400;
										cJSON_Delete( jsonresponse );
										cJSON_Delete( json );
										freeObjArray(&obj, objlevel);
										return -1;
									}
								}
							}
						}
						else
						{
							ParodusError("Failed to RETRIEVE object from json\n");
							(*response)->u.crud.status = 400;
							cJSON_Delete( jsonresponse );
							cJSON_Delete( json );
							freeObjArray(&obj, objlevel);
							return -1;
						}
						cJSON_Delete( jsonresponse );
						cJSON_Delete( json );
					}
				}
				else
				{
					ParodusError("CRUD config %s is empty\n", get_parodus_cfg()->crud_config_file);
					(*response)->u.crud.status = 500;
					freeObjArray(&obj, objlevel);
					return -1;
				}
			}
			else
			{
				ParodusError("CRUD config %s is not available\n", get_parodus_cfg()->crud_config_file);
				(*response)->u.crud.status = 500;
				freeObjArray(&obj, objlevel);
				return -1;
			}
			freeObjArray(&obj, objlevel);
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

int updateObject( wrp_msg_t *reqMsg, wrp_msg_t **response )
{
	cJSON *json, *jsonPayload = NULL;
	char *obj[5];
	int objlevel = 0, j=0, i =0;
	char *jsonData = NULL;
	cJSON *testObj1 = NULL, *testObj2 = NULL;
	int update_status = 0, jsonPayloadSize =0;
	int jsontagitemSize = 0, value =0;
	char *key = NULL, *testkey = NULL;
	const char *parse_error = NULL;
	int status =0, valid =0;
	int expireFlag = 0;
	int disconnStatus = 0;
	char *disconn_str = NULL;

	status = readFromJSON(&jsonData);
	ParodusPrint("read status %d\n", status);
	if(status == 0)
	{
		ParodusInfo("Proceed creating CRUD config %s\n", get_parodus_cfg()->crud_config_file );
		json = cJSON_Parse( jsonData );
	}
	else
	{
		if((jsonData !=NULL) && (strlen(jsonData)>0))
		{
			json = cJSON_Parse( jsonData );
			if( json == NULL )
			{
				parse_error = cJSON_GetErrorPtr();
				ParodusError("Parse Error before: %s\n", parse_error);
				if(jsonData !=NULL)
				{
					free( jsonData );
					jsonData = NULL;
				}
				(*response)->u.crud.status = 500;
				return -1;
			}
			else
			{
				ParodusInfo("CRUD config json parse success\n");
			}
		}
		else
		{
			ParodusInfo("CRUD config is empty, proceed creation of new object\n");
			json = cJSON_Parse( jsonData );
		}
	}

	if(jsonData !=NULL)
	{
		free( jsonData );
		jsonData = NULL;
	}

	if(reqMsg->u.crud.dest !=NULL)
	{
		ParodusInfo("reqMsg->u.crud.dest is %s\n", reqMsg->u.crud.dest);
		objlevel = parse_dest_elements_to_string(reqMsg, &obj);
		if(objlevel < 0)
		{
			(*response)->u.crud.status = 400;
			cJSON_Delete( json);
			return -1;
		}

		ParodusInfo( "Number of object level %d\n", objlevel );

		/* Valid request will be mac:14cfexxxx/parodus/tag/${name} which is objlevel 4 */
		if(objlevel == 4 && ((obj[2] != NULL) && (strcmp(obj[2] ,  "parodus") == 0) ) && ((obj[3] != NULL) &&(strcmp(obj[3] ,  "tag") == 0 )))
		{
			if(reqMsg->u.crud.payload != NULL)
			{
				ParodusInfo("reqMsg->u.crud.payload is %s\n", (char *)reqMsg->u.crud.payload);
				jsonPayload = cJSON_Parse( reqMsg->u.crud.payload );
				if(jsonPayload !=NULL)
				{
					jsonPayloadSize = cJSON_GetArraySize( jsonPayload );
					ParodusPrint( "jsonPayloadSize is %d\n", jsonPayloadSize );
					if(jsonPayloadSize)
					{
						/* checking the mandatory field "expires" key in request payload */
						for (i =0 ; i < jsonPayloadSize ; i++)
						{
							key= cJSON_GetArrayItem( jsonPayload, i )->string;

							if (key && (cJSON_Number == cJSON_GetArrayItem( jsonPayload, i )->type))
							{
								value = cJSON_GetArrayItem( jsonPayload, i )->valueint;
								ParodusPrint("key:%s value:%d\n", key, value);
								if(strcmp(key, "expires") == 0 )
								{
									ParodusPrint("Found expires key \n");
									if(!value)
									{
										ParodusError("Invalid request. Incorrect expires value\n");
										(*response)->u.crud.status = 400;
										cJSON_Delete( jsonPayload );
										jsonPayload = NULL;
										cJSON_Delete(json);
										json = NULL;
										freeObjArray(&obj, objlevel);
										return -1;
									}
									else
									{
										expireFlag = 1;
										break;
									}
								}
								else
								{
									ParodusPrint("expires key not found in payload, iterating ..\n");
								}
							}
						}

						if(!expireFlag )
						{
							ParodusError("Invalid request. Numeric expires value is mandatory in payload\n");
							(*response)->u.crud.status = 400;
							cJSON_Delete( jsonPayload );
							jsonPayload = NULL;
							cJSON_Delete(json);
							json = NULL;
							freeObjArray(&obj, objlevel);
							return -1;
						}

						/* checking for proper string values in request payload */
						for (i =0 ; i < jsonPayloadSize ; i++)
						{
							if(cJSON_GetArrayItem( jsonPayload, i )->string != NULL && strlen(cJSON_GetArrayItem( jsonPayload, i )->string) == 0)
							{
								ParodusError("Invalid Request. Key is NULL in request payload\n");
								(*response)->u.crud.status = 400;
								cJSON_Delete( jsonPayload );
								jsonPayload = NULL;
								cJSON_Delete(json);
								json = NULL;
								freeObjArray(&obj, objlevel);
								return -1;
							}
							else
							{
								if (cJSON_String == cJSON_GetArrayItem( jsonPayload, i )->type)
								{
									if(cJSON_GetArrayItem( jsonPayload, i )->valuestring != NULL && strlen(cJSON_GetArrayItem( jsonPayload, i )->valuestring) == 0)
									{
										ParodusError("Invalid request. Object value is NULL\n");
										(*response)->u.crud.status = 400;
										cJSON_Delete( jsonPayload );
										jsonPayload = NULL;
										cJSON_Delete(json);
										json = NULL;
										freeObjArray(&obj, objlevel);
										return -1;
									}
									else
									{
										ParodusPrint("key:%s value:%s\n", cJSON_GetArrayItem( jsonPayload, i )->string, cJSON_GetArrayItem( jsonPayload, i )->valuestring);
									}
								}
							}
						}
						cJSON* res_obj = cJSON_CreateObject();
						//check tags object exists
						cJSON *tagObj = cJSON_GetObjectItem( json, "tags" );
						if(tagObj !=NULL)
						{
							ParodusInfo("tag obj exists in json\n");

							//check requested test object exists under tags
							cJSON *testObj = cJSON_GetObjectItem( tagObj, obj[objlevel] );
							if(testObj !=NULL)
							{
								jsontagitemSize = cJSON_GetArraySize( tagObj );
								ParodusPrint( "jsontagitemSize is %d\n", jsontagitemSize );

								//traverse through each test objects to find match
								for( j = 0 ; j < jsontagitemSize ; j++ )
								{
									testkey = cJSON_GetArrayItem( tagObj, j )->string;
									ParodusPrint("testkey is %s\n", testkey);
									if( strcmp( testkey, obj[objlevel] ) == 0 )
									{
										ParodusInfo( "testObj already exists in json. Update it\n" );
										//Removes existing test object from json and adding new object
										cJSON_DeleteItemFromArray(tagObj, j);
										cJSON_AddItemToObject(tagObj, obj[objlevel], testObj2 = cJSON_CreateObject());
										for (i =0 ; i < jsonPayloadSize ; i++)
										{
											if (cJSON_Number == cJSON_GetArrayItem( jsonPayload, i )->type)
											{
												ParodusPrint("%s %d\n", cJSON_GetArrayItem( jsonPayload, i )->string, cJSON_GetArrayItem( jsonPayload, i )->valueint);
												cJSON_AddNumberToObject(testObj2, cJSON_GetArrayItem( jsonPayload, i )->string, cJSON_GetArrayItem( jsonPayload, i )->valueint);
											}
											else if (cJSON_String == cJSON_GetArrayItem( jsonPayload, i )->type)
											{
												ParodusPrint("%s %s\n", cJSON_GetArrayItem( jsonPayload, i )->string, cJSON_GetArrayItem( jsonPayload, i )->valuestring);
												cJSON_AddStringToObject(testObj2, cJSON_GetArrayItem( jsonPayload, i )->string, cJSON_GetArrayItem( jsonPayload, i )->valuestring);
											}
											else
											{
												ParodusError("Invalid Type in request payload\n");
												(*response)->u.crud.status = 400;
												cJSON_Delete(res_obj);
												res_obj = NULL;
												cJSON_Delete( jsonPayload );
												jsonPayload = NULL;
												cJSON_Delete(json);
												json = NULL;
												freeObjArray(&obj, objlevel);
												return -1;
											}
										}
										(*response)->u.crud.status = 200;
										//Pass freeflag as 0 if you add new child obj to parent obj i.e test obj creation
										update_status = writeIntoCrudJson(res_obj,"tags",tagObj,0);
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
								ParodusInfo("testObj doesnot exists in json, adding it\n");
								cJSON_AddItemToObject(tagObj, obj[objlevel], testObj1 = cJSON_CreateObject());

								for (i =0 ; i < jsonPayloadSize ; i++)
								{
									if (cJSON_Number == cJSON_GetArrayItem( jsonPayload, i )->type)
									{
										cJSON_AddNumberToObject(testObj1, cJSON_GetArrayItem( jsonPayload, i )->string, cJSON_GetArrayItem( jsonPayload, i )->valueint);
									}
									else if (cJSON_String == cJSON_GetArrayItem( jsonPayload, i )->type)
									{
										cJSON_AddStringToObject(testObj1, cJSON_GetArrayItem( jsonPayload, i )->string, cJSON_GetArrayItem( jsonPayload, i )->valuestring);
									}
									else
									{
										ParodusError("Invalid Type in request payload\n");
										(*response)->u.crud.status = 400;
										cJSON_Delete(res_obj);
										res_obj = NULL;
										cJSON_Delete( jsonPayload );
										jsonPayload = NULL;
										cJSON_Delete(json);
										json = NULL;
										freeObjArray(&obj, objlevel);
										return -1;
									}
								}
								(*response)->u.crud.status = 201;
								//Pass freeflag as 0 if you add new child obj to parent obj i.e test obj creation
								update_status = writeIntoCrudJson(res_obj,"tags",tagObj,0);
							}
						}
						else
						{
							ParodusInfo("tagObj doesnot exists in json, adding it\n");

							tagObj = cJSON_CreateObject();
							cJSON_AddItemToObject(tagObj, obj[objlevel], testObj1 = cJSON_CreateObject());

							for (i =0 ; i < jsonPayloadSize ; i++)
							{
								if (cJSON_Number == cJSON_GetArrayItem( jsonPayload, i )->type)
								{
									cJSON_AddNumberToObject(testObj1, cJSON_GetArrayItem( jsonPayload, i )->string, cJSON_GetArrayItem( jsonPayload, i )->valueint);
								}
								else if (cJSON_String == cJSON_GetArrayItem( jsonPayload, i )->type)
								{
									cJSON_AddStringToObject(testObj1, cJSON_GetArrayItem( jsonPayload, i )->string, cJSON_GetArrayItem( jsonPayload, i )->valuestring);
								}
								else
								{
									ParodusError("Invalid Type in request payload\n");
									(*response)->u.crud.status = 400;
									cJSON_Delete(tagObj);
									tagObj = NULL;
									cJSON_Delete(res_obj);
									res_obj = NULL;
									cJSON_Delete( jsonPayload );
									jsonPayload = NULL;
									cJSON_Delete(json);
									json = NULL;
									freeObjArray(&obj, objlevel);
									return -1;
								}
							}
							(*response)->u.crud.status = 201;
							//Pass freeflag as 1 if you create new parent json object i.e tag obj creation
							update_status = writeIntoCrudJson(res_obj,"tags",tagObj,1);
						}

						cJSON_Delete( jsonPayload );
						jsonPayload = NULL;
						cJSON_Delete(json);
						json = NULL;
						freeObjArray(&obj, objlevel);
						if(update_status == 1)
						{
							ParodusPrint("Data is successfully added to JSON\n");
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
						ParodusError("Invalid UPDATE request, payload size is 0\n");
						(*response)->u.crud.status = 400;
						freeObjArray(&obj, objlevel);
						cJSON_Delete( jsonPayload );
						jsonPayload = NULL;
						cJSON_Delete( json);
						return -1;
					}
				}
				else
				{
					ParodusError("Invalid UPDATE request, payload is not json\n");
					(*response)->u.crud.status = 400;
					freeObjArray(&obj, objlevel);
					cJSON_Delete( json);
					return -1;
				}
			}
			else
			{
				ParodusError("Invalid UPDATE request, payload is NULL\n");
				(*response)->u.crud.status = 400;
				freeObjArray(&obj, objlevel);
				cJSON_Delete( json );
				return -1;
			}
		}
		else
		{
			//Checks for parodus/cloud-disconnect request with objlevel = 3
			if(objlevel == 3 && ((obj[3] != NULL) && (strcmp(obj[3] ,  "cloud-disconnect") == 0)))
			{
				if(reqMsg->u.crud.payload != NULL)
				{
					jsonPayload = cJSON_Parse( reqMsg->u.crud.payload );
					if(jsonPayload !=NULL)
					{
						if((cJSON_GetObjectItem( jsonPayload, CLOUD_DISCONNECT_REASON )) !=NULL)
						{
							if (cJSON_String == cJSON_GetObjectItem( jsonPayload, CLOUD_DISCONNECT_REASON )->type)
							{
								disconn_str = cJSON_GetObjectItem( jsonPayload, CLOUD_DISCONNECT_REASON )->valuestring;
								if (disconn_str != NULL && strlen(disconn_str) == 0)
								{
									ParodusError("Invalid cloud-disconnect request. disconnect reason is NULL\n");
									(*response)->u.crud.status = 400;
									cJSON_Delete( jsonPayload );
									jsonPayload = NULL;
									cJSON_Delete(json);
									json = NULL;
									freeObjArray(&obj, objlevel);
									return -1;
								}
								else
								{
									//check disconnection reason is character string of [a-zA-Z0-9 ]*

									valid = validateDisconnectString(disconn_str);
									if(valid >0)
									{
										//set the disconnection reason value to in-memory
										set_cloud_disconnect_reason(get_parodus_cfg(), disconn_str);
										ParodusInfo("set cloud-disconnect reason as %s \n", get_parodus_cfg()->cloud_disconnect);

									}
									else
									{
										ParodusError("Invalid cloud-disconnect request. disconnect reason is not alphanumeric\n");
										(*response)->u.crud.status = 400;
										cJSON_Delete( jsonPayload );
										jsonPayload = NULL;
										cJSON_Delete(json);
										json = NULL;
										freeObjArray(&obj, objlevel);
										return -1;
									}
								}
							}
							else
							{
								ParodusError("Invalid cloud-disconnect request, disconnect reason is not string\n");
								(*response)->u.crud.status = 400;
								cJSON_Delete( jsonPayload );
								jsonPayload = NULL;
								cJSON_Delete(json);
								json = NULL;
								freeObjArray(&obj, objlevel);
								return -1;
							}
						}
						else
						{
							ParodusError("Invalid cloud-disconnect request, disconnection-reason not found\n");
							(*response)->u.crud.status = 400;
							freeObjArray(&obj, objlevel);
							cJSON_Delete( jsonPayload );
							jsonPayload = NULL;
							cJSON_Delete( json);
							return -1;
						}
						cJSON_Delete( jsonPayload );
						jsonPayload = NULL;
					}
					else
					{
						ParodusError("Invalid cloud-disconnect request, payload is not json\n");
						(*response)->u.crud.status = 400;
						freeObjArray(&obj, objlevel);
						cJSON_Delete( json);
						return -1;
					}
				}
				else
				{
					ParodusInfo("cloud-disconnect failed as payload is NULL\n");
					(*response)->u.crud.status = 400;
					freeObjArray(&obj, objlevel);
					return -1;
				}
				char *reason = strdup(get_parodus_cfg()->cloud_disconnect);
				disconnStatus = ConnDisconnectFromCloud(reason);
				freeObjArray(&obj, objlevel);
				cJSON_Delete( json );
				if (disconnStatus >0)
				{
					ParodusInfo("Sending update response for cloud-disconnect\n");
					(*response)->u.crud.status = 200;
				}
				else
				{
					ParodusInfo("Failure in disconnecting from cloud ..\n");
					(*response)->u.crud.status = 500;
					return -1;
				}
			}
			else
			{
				//  Return error for request format other than parodus/tag/${name}
				ParodusError("Invalid UPDATE request\n");
				(*response)->u.crud.status = 400;
				freeObjArray(&obj, objlevel);
				cJSON_Delete( json );
				return -1;
			}
		}
	}
	else
	{
		ParodusError("Requested dest path is NULL\n");
		(*response)->u.crud.status = 400;
		cJSON_Delete( json );
		return -1;
	}
	return 0;
}

static int ConnDisconnectFromCloud(char *disconn_reason)
{
	bool close_retry = false;
	close_retry = get_close_retry();
	if(!close_retry)
	{
		ParodusInfo("Reconnect detected, setting reason %s for Reconnect\n", disconn_reason);
		OnboardLog("Reconnect detected, setting reason %s for Reconnect\n", disconn_reason);
		set_global_reconnect_reason(disconn_reason);
		set_global_reconnect_status(true);
		set_close_retry();
	}
	else
	{
		ParodusInfo("close_retry is %d, connection close and retrying is already in progress\n", close_retry);
		return -1;
	}
	return 1;
}

int deleteObject( wrp_msg_t *reqMsg, wrp_msg_t **response )
{
	cJSON *paramArray = NULL, *json = NULL;
	char *jsonData = NULL;
	char *obj[5], *out = NULL;
	int i = 0, status =0, objlevel=0, found =0;
	int itemSize = 0, delete_status = 0;
	const char *parse_error = NULL;

	status = readFromJSON(&jsonData);
	if(status)
	{
		if((jsonData !=NULL) && (strlen(jsonData)>0))
		{
			json = cJSON_Parse( jsonData );
			if(jsonData !=NULL)
			{
				free( jsonData );
				jsonData = NULL;
			}
			if( json == NULL )
			{
			    parse_error = cJSON_GetErrorPtr();
			    if (parse_error != NULL)
				{
				    ParodusError("Parse Error before: %s\n", parse_error);
				}
			    (*response)->u.crud.status = 500;
			    return -1;
			}
			else
			{
				ParodusInfo("CRUD config json parse success\n");
				if(reqMsg->u.crud.dest !=NULL)
				{
					ParodusInfo("reqMsg->u.crud.dest is %s\n", reqMsg->u.crud.dest);
					objlevel = parse_dest_elements_to_string(reqMsg, &obj);
					if(objlevel < 0)
					{
						(*response)->u.crud.status = 400;
						cJSON_Delete( json);
						return -1;
					}

					ParodusInfo( "Number of object level %d\n", objlevel );

					/* Valid request will be mac:14cfexxxx/parodus/tag/${name} which is objlevel 4 */

					if(objlevel == 4 && ((obj[2] != NULL) && (strcmp(obj[2] ,  "parodus") == 0) ) && ((obj[3] != NULL) &&(strcmp(obj[3] ,  "tag") == 0 )))
					{
						paramArray = cJSON_GetObjectItem( json, "tags" );
						if( paramArray != NULL )
						{
							itemSize = cJSON_GetArraySize( paramArray );
							if( itemSize == 0 )
							{
								ParodusInfo("Invalid delete, tags object is empty in json\n");
								(*response)->u.crud.status = 400;
								freeObjArray(&obj, objlevel);
								cJSON_Delete( json );
								return -1;
							}
							else
							{
								//top level tags object
								if( strcmp( cJSON_GetObjectItem( json, "tags" )->string, obj[objlevel] ) == 0 )
								{
									ParodusInfo("Top level tags object delete not supported\n");
									(*response)->u.crud.status = 400;
									freeObjArray(&obj, objlevel);
									cJSON_Delete( json );
									return -1;
								}
								else
								{
									//to traverse through total number of objects in json
									for( i = 0 ; i < itemSize ; i++ )
									{
										if( strcmp( cJSON_GetArrayItem( paramArray, i )->string, obj[objlevel] ) == 0 )
										{
											ParodusInfo("Delete: requested object found \n");
											cJSON_DeleteItemFromArray(paramArray, i);
											found = 1;
											(*response)->u.crud.status = 200;
											break;
										}
									}

									if(!found)
									{
										ParodusError("requested object not found\n");
										(*response)->u.crud.status = 400;
										freeObjArray(&obj, objlevel);
										cJSON_Delete( json );
										return -1;
									}
								}
							}
						}
						else
						{
							ParodusError("Failed to DELETE object from json\n");
							(*response)->u.crud.status = 400;
							freeObjArray(&obj, objlevel);
							cJSON_Delete( json );
							return -1;
						}
					}
					else
					{
						//  Return error for request format other than parodus/tag/${name}
						ParodusError("Invalid DELETE request\n");
						(*response)->u.crud.status = 400;
						freeObjArray(&obj, objlevel);
						cJSON_Delete( json );
						return -1;
					}
					freeObjArray(&obj, objlevel);
				}
				else
				{
					ParodusError("Requested dest path is NULL\n");
					(*response)->u.crud.status = 400;
					cJSON_Delete( json );
					return -1;
				}
			}
		}
		else
		{
			ParodusError("CRUD config %s is empty\n", get_parodus_cfg()->crud_config_file);
			(*response)->u.crud.status = 500;
			return -1;
		}
	}
	else
	{
		ParodusError("CRUD config %s is not available\n", get_parodus_cfg()->crud_config_file);
		(*response)->u.crud.status = 500;
		return -1;
	}
	out = cJSON_PrintUnformatted( json );
	ParodusPrint("%s\n",out);
	delete_status = writeToJSON(out);
	if(delete_status == 1)
	{
		ParodusPrint("Deleted Data is successfully updated to JSON\n");
	}
	else
	{
		ParodusError("Failed to update deleted data to JSON\n");
	}
	cJSON_Delete( json );
	if(out !=NULL)
	{
		free( out );
	}
	return 0;
}

static void freeObjArray(char *(*obj)[], int size){
	int i;
	for (i = 0; i <= size; i++)
	{
		ParodusPrint("Freeing Object: %s\n",(*obj)[i]);
		free((*obj)[i]);
		(*obj)[i] = NULL;
	}
}

static int parse_dest_elements_to_string(wrp_msg_t *reqMsg, char *(*obj)[])
{
		int i =0, rv = -1;
		char *start = NULL, *end = NULL;

		/* To extract dest elements till wrp application field (mac:44aaf59b18xx/parodus/tags)
		and store it in obj array */

		for( i = 0; i <= WRP_ID_ELEMENT__APPLICATION; i++ )
		{
			(*obj)[i] =  wrp_get_msg_element(i, reqMsg, DEST);
			if((*obj)[i] ==NULL)
			{
				break;
			}
			ParodusPrint("(*obj)[%d] is %s \n", i, (*obj)[i]);
			rv = i;
		}

	   /* To break down individual dest elements from application field if any.
	   e.g. application element tags/test to tags and test & store it in obj array. */

		start = (*obj)[rv];
		end = strchr(start, '/');
		if (NULL != end)
		{
			char *testElem = NULL;
			char *tagElem = NULL;

			tagElem = strdupptr(start, end);
			ParodusPrint("tagElem is %s\n",tagElem);

			start = end;
			start++;

			end = strchr(start, '/');
			if (NULL == end)
			{
				testElem = strdup(start);
				ParodusPrint("testElem is %s\n", testElem);
			}
			else
			{
				ParodusError("Unsupported dest format for CRUD\n");
				freeObjArray(obj, rv);
				rv = -1;
			}

			if(rv != -1)
			{
				//Reuse array of pointer
				free((*obj)[rv]);
				(*obj)[rv] = NULL;
				(*obj)[rv] = tagElem;
				(*obj)[++rv] = testElem;
			}
			else if (tagElem != NULL)
			{
				free(tagElem);
			}
		}

		return rv;
}

static char* strdupptr( const char *s, const char *e )
{
    if (s == e) {
        return NULL;
    }

    return strndup(s, (size_t) (((uintptr_t)e) - ((uintptr_t)s)));
}

static int validateDisconnectString(char *reason)
{
	int k=0, rv =1;
	if(reason !=NULL && strlen(reason) >0 )
	{
		for( k = 0; k < (int)strlen(reason); k++ )
		{
			if ((isalpha(reason[k])) || ((isdigit(reason[k])) != 0))
			{
				ParodusPrint("disconnection string is valid\n");
			}
			else
			{
				ParodusError("disconnection string is Invalid\n");
				rv = -1;
				break;
			}
		}
	}
	else
	{
		rv = -1;
	}
	return rv;
}

static int getClientStatus(char *service, cJSON **jsonresponse)
{
	char regstatus[16] ={0};
	*jsonresponse = cJSON_CreateObject();

	if(service == NULL)
	{
		ParodusError("service is NULL\n");
		return -1;
	}

	if(checkClientStatus(service))
	{
		strncpy(regstatus, "online", sizeof(regstatus)-1);
	}
	else
	{
		strncpy(regstatus, "offline", sizeof(regstatus)-1);
	}
	ParodusPrint("getClientStatus: service:%s value:%s\n", service, regstatus);
	cJSON_AddItemToObject( *jsonresponse, SERVICE_STATUS , cJSON_CreateString(regstatus));
	return 0;
}
