/**
 * Copyright 2018 Comcast Cable Communications Management, LLC
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
 * @file config_file.c
 *
 * @description This file contains configuration file parsing for parodus
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <cJSON.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>

#include "config.h"
#include "config_file.h"
#include "ParodusInternal.h"
#include "parodus_log.h"



/* Exported Functions */
int processParodusCfg(ParodusCfg *cfg, char *config_disk_file);


/* Internal Functions */
static cJSON* __parse_config_file( char *file, int *error );
int __setParodusConfig(cJSON *jsp, ParodusCfg *cfg);


/* Local static variables */



int processParodusCfg(ParodusCfg *cfg, char *file)
{
    cJSON *jsp = NULL;
    int error = 0;
    
    (void ) cfg;
    
    jsp = __parse_config_file(file, &error);
    
    if (error) {
      ParodusError("processParodusCfg(cfg, %s): Failed %d\n", file, error);  
      if (jsp) {
        cJSON_Delete(jsp);
      }  
      return error;
    } 
    
    error = __setParodusConfig(jsp, cfg);
    
    cJSON_Delete(jsp);

    return error;
}


cJSON* __parse_config_file( char *file, int *error )
{
    FILE *fp;
    char *text;
    static cJSON_Hooks hooks = { .malloc_fn = malloc,
                                 .free_fn = free };
    cJSON_InitHooks( &hooks );

    text = NULL;
    fp = fopen( file, "r" );
    if( NULL != fp ) {
        if( 0 == fseek(fp, 0, SEEK_END) ) {
            size_t size;
            
            size = ftell( fp );
            if( 0 < size ) {
                rewind( fp );

                text = (char*) malloc( sizeof(char) * (size + 1) );
                if( NULL != text ) {
                    if( size != fread(text, sizeof(char), size, fp) ) {
                        free( text );
                        text = NULL;
                    } else {
                        text[size] = '\0';
                    }
                }
            }
        }
        fclose( fp );
    } else {
        ParodusError("\n__parse_config_file():Can't open file %s\n", file);
        *error = -1;
        return NULL;
    }

    if( NULL != text ) {
        cJSON *rv;
        
        rv = cJSON_Parse(text);
        free(text);
        if (NULL == rv) {
            *error = -2;
        } else {
            *error = 0;
        }
        return rv;
    } else {
        *error = -3;
        ParodusError("__parse_config_file():malloc failed\n");
    }

    return NULL;
}


int __setParodusConfig(cJSON *jsp, ParodusCfg *cfg)
{
    cJSON *item;
    
    memset(cfg, 0, sizeof(ParodusCfg));
       
    item = cJSON_GetObjectItem(jsp, HW_MODELNAME);
    if (item && (cJSON_String == item->type)) {
        strncpy(cfg->hw_model, item->valuestring, 64);
    }
    
    item = cJSON_GetObjectItem(jsp, HW_SERIALNUMBER);
    if (item && (cJSON_String == item->type)) {
        strncpy(cfg->hw_serial_number, item->valuestring, 64);
    }
 
    item = cJSON_GetObjectItem(jsp, HW_MANUFACTURER);
    if (item && (cJSON_String == item->type)) {
        strncpy(cfg->hw_manufacturer, item->valuestring, 64);
    }
    
    item = cJSON_GetObjectItem(jsp, HW_DEVICEMAC);
    if (item && (cJSON_String == item->type)) {
        strncpy(cfg->hw_mac, item->valuestring, 64);
    }
    
    item = cJSON_GetObjectItem(jsp, HW_LAST_REBOOT_REASON);
    if (item && (cJSON_String == item->type)) {
        strncpy(cfg->hw_last_reboot_reason, item->valuestring, 64);
    }
                
    item = cJSON_GetObjectItem(jsp, FIRMWARE_NAME);
    if (item && (cJSON_String == item->type)) {
        strncpy(cfg->fw_name, item->valuestring, 64);
    }
    
    item = cJSON_GetObjectItem(jsp, BOOT_TIME);
    if (item && (cJSON_Number == item->type)) {
        cfg->boot_time = item->valueint;
    }
    
    item = cJSON_GetObjectItem(jsp, WEBPA_PROTOCOL);
    if (item && (cJSON_String == item->type)) {
        strncpy(cfg->webpa_protocol, item->valuestring, 64);
    }    
    
    item = cJSON_GetObjectItem(jsp, WEBPA_INTERFACE);
    if (item && (cJSON_String == item->type)) {
        strncpy(cfg->webpa_interface_used, item->valuestring, 64);
    }
     
    item = cJSON_GetObjectItem(jsp, WEBPA_UUID);
    if (item && (cJSON_String == item->type)) {
        strncpy(cfg->webpa_uuid, item->valuestring, 64);
    }  
    
    item = cJSON_GetObjectItem(jsp, WEBPA_PING_TIMEOUT);
    if (item && (cJSON_Number == item->type)) {
        cfg->webpa_ping_timeout = item->valueint;
    }

    item = cJSON_GetObjectItem(jsp, WEBPA_URL);
    if (item && (cJSON_String == item->type)) {
        strncpy(cfg->webpa_url, item->valuestring, 64);
    }
    
    item = cJSON_GetObjectItem(jsp, WEBPA_BACKOFF_MAX);
    if (item && (cJSON_Number == item->type)) {
        cfg->webpa_backoff_max = item->valueint;
    }

    item = cJSON_GetObjectItem(jsp, PARTNER_ID);
    if (item && (cJSON_String == item->type)) {
        strncpy(cfg->partner_id, item->valuestring, 64);
    }
     
     item = cJSON_GetObjectItem(jsp, CERT_PATH);
    if (item && (cJSON_String == item->type)) {
        strncpy(cfg->cert_path, item->valuestring, 64);
    }
 
    /* ToDo: Do a check of required fields */ 
     
    return 0;
}