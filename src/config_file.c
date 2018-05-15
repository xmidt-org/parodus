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
    setDefaultValuesToCfg(cfg);

    item = cJSON_GetObjectItem(jsp, HW_MODELNAME);
    if (item && (cJSON_String == item->type)) {
        parStrncpy(cfg->hw_model, item->valuestring, sizeof(cfg->hw_model));
        ParodusInfo("hw-model is %s\n",cfg->hw_model);
    }

    item = cJSON_GetObjectItem(jsp, HW_SERIALNUMBER);
    if (item && (cJSON_String == item->type)) {
        parStrncpy(cfg->hw_serial_number, item->valuestring, sizeof(cfg->hw_serial_number));
        ParodusInfo("hw_serial_number is %s\n",cfg->hw_serial_number);
    }

    item = cJSON_GetObjectItem(jsp, HW_MANUFACTURER);
    if (item && (cJSON_String == item->type)) {
        parStrncpy(cfg->hw_manufacturer, item->valuestring, sizeof(cfg->hw_manufacturer));
        ParodusInfo("hw_manufacturer is %s\n",cfg->hw_manufacturer);
    }

    item = cJSON_GetObjectItem(jsp, HW_DEVICEMAC);
    if (item && (cJSON_String == item->type)) {
        if (parse_mac_address (cfg->hw_mac, item->valuestring) == 0)
        {
            ParodusInfo ("hw_mac is %s\n",cfg->hw_mac);
        }
        else
        {
            ParodusError ("Bad mac address %s\n", optarg);
            return -1;
        }
    }

    item = cJSON_GetObjectItem(jsp, HW_LAST_REBOOT_REASON);
    if (item && (cJSON_String == item->type)) {
        parStrncpy(cfg->hw_last_reboot_reason, item->valuestring, sizeof(cfg->hw_last_reboot_reason));
        ParodusInfo("hw_last_reboot_reason is %s\n",cfg->hw_last_reboot_reason);
    }

    item = cJSON_GetObjectItem(jsp, FIRMWARE_NAME);
    if (item && (cJSON_String == item->type)) {
        parStrncpy(cfg->fw_name, item->valuestring, sizeof(cfg->fw_name));
        ParodusInfo ("fw_name is %s\n",cfg->fw_name);
    }

    item = cJSON_GetObjectItem(jsp, BOOT_TIME);
    if (item && (cJSON_Number == item->type)) {
        cfg->boot_time = item->valueint;
        ParodusInfo("boot_time is %d\n",cfg->boot_time);
    }

    item = cJSON_GetObjectItem(jsp, WEBPA_PING_TIMEOUT);
    if (item && (cJSON_Number == item->type)) {
        cfg->webpa_ping_timeout = item->valueint;
        ParodusInfo("webpa_ping_timeout is %d\n",cfg->webpa_ping_timeout);
    }

    item = cJSON_GetObjectItem(jsp, WEBPA_URL);
    if (item && (cJSON_String == item->type)) {
        parStrncpy(cfg->webpa_url, item->valuestring, sizeof(cfg->webpa_url));
		if (server_is_http (cfg->webpa_url, NULL) < 0) {
			ParodusError ("Bad webpa url %s\n", item->valuestring);
			return -1;
		}
        ParodusInfo("webpa_url is %s\n",cfg->webpa_url);
    }

    item = cJSON_GetObjectItem(jsp, WEBPA_BACKOFF_MAX);
    if (item && (cJSON_Number == item->type)) {
        cfg->webpa_backoff_max = item->valueint;
        ParodusInfo("webpa_backoff_max is %d\n",cfg->webpa_backoff_max);
    }

    item = cJSON_GetObjectItem(jsp, WEBPA_INTERFACE);
    if (item && (cJSON_String == item->type)) {
        parStrncpy(cfg->webpa_interface_used, item->valuestring, sizeof(cfg->webpa_interface_used));
        ParodusInfo("webpa_interface_used is %s\n",cfg->webpa_interface_used);
    }

    item = cJSON_GetObjectItem(jsp, PARODUS_LOCAL_URL);
    if (item && (cJSON_String == item->type)) {
        parStrncpy(cfg->local_url, item->valuestring, sizeof(cfg->local_url));
        ParodusInfo("parodus local_url is %s\n",cfg->local_url);
    }

    item = cJSON_GetObjectItem(jsp, PARTNER_ID);
    if (item && (cJSON_String == item->type)) {
        parStrncpy(cfg->partner_id, item->valuestring, sizeof(cfg->partner_id));
        ParodusInfo("partner_id is %s\n",cfg->partner_id);
    }
#ifdef ENABLE_SESHAT
    item = cJSON_GetObjectItem(jsp, SESHAT_URL);
    if (item && (cJSON_String == item->type)) {
        parStrncpy(cfg->seshat_url, item->valuestring, sizeof(cfg->seshat_url));
        ParodusInfo("seshat_url is %s\n",cfg->seshat_url);
    }
#endif

    item = cJSON_GetObjectItem(jsp, TXT_URL);
    if (item && (cJSON_String == item->type)) {
        parStrncpy(cfg->dns_txt_url, item->valuestring, sizeof(cfg->dns_txt_url));
        ParodusInfo("parodus dns-txt-url is %s\n",cfg->dns_txt_url);
    }

    item = cJSON_GetObjectItem(jsp, ACQUIRE_JWT);
    if (item && (cJSON_Number == item->type)) {
        cfg->acquire_jwt = item->valueint;
        ParodusInfo("acquire jwt option is %d\n",cfg->acquire_jwt);
    }

    item = cJSON_GetObjectItem(jsp, JWT_ALGORITHM);
    if (item && (cJSON_String == item->type)) {
        cfg->jwt_algo = get_algo_mask (item->valuestring);
		if (cfg->jwt_algo == (unsigned int) -1)
		{
		  return -1;
		}
		ParodusInfo("jwt_algo is %u\n",cfg->jwt_algo);
    }

    item = cJSON_GetObjectItem(jsp, JWT_KEY);
    if (item && (cJSON_String == item->type)) {
        read_key_from_file (item->valuestring, cfg->jwt_key, sizeof(cfg->jwt_key));
        ParodusInfo("jwt_key is %s\n",cfg->jwt_key);
    }

    item = cJSON_GetObjectItem(jsp, CERT_PATH);
    if (item && (cJSON_String == item->type)) {
        parStrncpy(cfg->cert_path, item->valuestring, sizeof(cfg->cert_path));
        ParodusInfo("cert_path is %s\n",cfg->cert_path);
    }

    item = cJSON_GetObjectItem(jsp, ACQUISITION_SCRIPT);
    if (item && (cJSON_String == item->type)) {
        parStrncpy(cfg->token_acquisition_script, item->valuestring, sizeof(cfg->token_acquisition_script));
    }

    item = cJSON_GetObjectItem(jsp, READ_SCRIPT);
    if (item && (cJSON_String == item->type)) {
        parStrncpy(cfg->token_read_script, item->valuestring, sizeof(cfg->token_read_script));
    }

    item = cJSON_GetObjectItem(jsp, HUB_OR_SPOKE);
    if (item && (cJSON_String == item->type)) {
        parStrncpy(cfg->hub_or_spk, item->valuestring, sizeof(cfg->hub_or_spk));
        ParodusInfo("hub_or_spk is %s\n",cfg->hub_or_spk);
    }

    item = cJSON_GetObjectItem(jsp, FORCE_IPV4);
    if (item) {
        if(cJSON_True == item->type)
        {
            ParodusInfo("Force IPv4\n");
            cfg->flags |= FLAGS_IPV4_ONLY;
        }
    }

    item = cJSON_GetObjectItem(jsp, FORCE_IPV6);
    if (item) {
        if(cJSON_True == item->type)
        {
            ParodusInfo("Force IPv6\n");
            cfg->flags |= FLAGS_IPV6_ONLY;
        }
    }

    return 0;
}
