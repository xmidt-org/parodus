/**
 * Copyright 2015 Comcast Cable Communications Management, LLC
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
 * @file config.h
 *
 * @description This file contains configuration details of parodus
 *
 */
 
#ifndef _CONFIG_H_ 
#define _CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

/* WRP CRUD Model Macros */
#define HW_MODELNAME            "hw-model"
#define HW_SERIALNUMBER         "hw-serial-number"
#define HW_MANUFACTURER         "hw-manufacturer"
#define HW_DEVICEMAC            "hw-mac"
#define HW_LAST_REBOOT_REASON   "hw-last-reboot-reason"
#define FIRMWARE_NAME           "fw-name"
#define BOOT_TIME               "boot-time"
#define LAST_RECONNECT_REASON   "webpa-last-reconnect-reason"
#define WEBPA_PROTOCOL          "webpa-protocol"
#define WEBPA_INTERFACE         "webpa-inteface-used"
#define WEBPA_UUID              "webpa-uuid"
#define WEBPA_URL               "webpa-url"
#define WEBPA_PING_TIMEOUT      "webpa-ping-timeout"
#define WEBPA_BACKOFF_MAX       "webpa-backoff-max"
#define PARTNER_ID              "partner-id"

#define PROTOCOL_VALUE          "PARODUS-2.0"
#define WEBPA_PATH_URL          "/api/v2/device"

#ifdef ENABLE_CJWT
#   define DNS_ID               "fabric"
#endif
#define PARODUS_UPSTREAM        "tcp://127.0.0.1:6666"

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

typedef struct
{
    char hw_model[64];
    char hw_serial_number[64];
    char hw_manufacturer[64];
    char hw_mac[64];
    char hw_last_reboot_reason[64];
    char fw_name[64];
    unsigned int boot_time;
    unsigned int webpa_ping_timeout;
    char webpa_url[124];
    char webpa_path_url[124];
    unsigned int webpa_backoff_max;
    char webpa_interface_used[16];
    char webpa_protocol[32];
    char webpa_uuid[64];
    unsigned int flags;
    char local_url[124];
    char partner_id[64];
#ifdef ENABLE_SESHAT
    char seshat_url[128];
#endif
#ifdef ENABLE_CJWT
    char dns_id[64];
    char jwt_algo[32];  // bit mask set for each allowed algorithm
    char jwt_key[4096]; // may be read in from a pem file
#endif
    char cert_path[64];
    char webpa_new_header[4096];
    unsigned int secure_flag;
    unsigned int port;
	char header_create_filename[64];
    char header_read_filename[64];
} ParodusCfg;

#define FLAGS_SECURE    (1 << 0)
#define FLAGS_IPV6_ONLY (1 << 1)
#define FLAGS_IPV4_ONLY (1 << 2)

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

void loadParodusCfg(ParodusCfg * config,ParodusCfg *cfg);
void create_header_from_file(char *newHeader, size_t len);
void parseCommandLine(int argc,char **argv,ParodusCfg * cfg);
void setDefaultValuesToCfg(ParodusCfg *cfg); 
void read_header_value(ParodusCfg *cfg);
// Accessor for the global config structure.
ParodusCfg *get_parodus_cfg(void);
void set_parodus_cfg(ParodusCfg *);
#ifdef __cplusplus
}
#endif

#endif
