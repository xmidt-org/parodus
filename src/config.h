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
#define CERT_PATH               "ssl-cert-path"

#define PROTOCOL_VALUE 					"PARODUS-2.0"
#define WEBPA_PATH_URL                  "/api/v2/device"
#define JWT_ALGORITHM					"jwt-algo"
#define	JWT_KEY						"jwt-key"
#define DNS_TXT_URL	"fabric"
#define PARODUS_UPSTREAM                "tcp://127.0.0.1:6666"

#define ALLOW_NON_RSA_ALG	false

    
/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

typedef struct
{
    char *hw_model;
    char *hw_serial_number;
    char *hw_manufacturer;
    char *hw_mac;
    char *hw_last_reboot_reason;
    char *fw_name;
    unsigned int boot_time;
    unsigned int webpa_ping_timeout;
    char *webpa_url;
    char *webpa_path_url;
    unsigned int webpa_backoff_max;
    char *webpa_interface_used;
    char *webpa_protocol;
    char *webpa_uuid;
    unsigned int flags;
    char *local_url;
    char *partner_id;
#ifdef ENABLE_SESHAT
    char *seshat_url;
#endif
    char *dns_txt_url;
    unsigned int acquire_jwt;
    unsigned int jwt_algo;  // bit mask set for each allowed algorithm
    char *jwt_key; // may be read in from a pem file
    char *cert_path;
    char *webpa_auth_token;
    char *token_acquisition_script;
    char *token_read_script;
} ParodusCfg;

#define FLAGS_IPV6_ONLY (1 << 0)
#define FLAGS_IPV4_ONLY (1 << 1)
#define FLAGS_IPV6_IPV4 (FLAGS_IPV6_ONLY | FLAGS_IPV4_ONLY)

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

void loadParodusCfg(ParodusCfg * config,ParodusCfg *cfg);
void createNewAuthToken(char *newToken, size_t len);

/**
 * parse command line arguments and create config structure
 * and return whether args are valid or not
 *
 * @param argc	number of command line arguments
 * @param argv	command line argument lis
 * @return 0 if OK
*    or -1 if error
*/ 
int parseCommandLine(int argc,char **argv,ParodusCfg * cfg);

void setDefaultValuesToCfg(ParodusCfg *cfg); 
void getAuthToken(ParodusCfg *cfg);
// Accessor for the global config structure.
ParodusCfg *get_parodus_cfg(void);

/*
 Called on program exit
 */
void clean_up_parodus_cfg(ParodusCfg *cfg);

/* File Utility Function */
/* Mallocs memory for the entire file, caller will have to call free(*data) */
void read_key_from_file (const char *fname, char **data);


void set_parodus_cfg(ParodusCfg *);
char *get_token_application(void) ;

/**
 * parse a webpa url. Extract the server address, the port
 * and return whether it's secure or not
 *
 * @param full_url		full url
 * @param server_addr	 buffer containing server address found in url
 * @param server_addr_buflen len of the server addr buffer provided by caller
 * @param port_buf 		buffer containing port value found in url
 * @param port_buflen	len of the port buffer provided by caller
 * @return 1 if insecure connection is allowed, 0 if not,
*    or -1 if error
*/ 
int parse_webpa_url(const char *full_url, 
	char *server_addr, int server_addr_buflen,
	char *port_buf, int port_buflen);

#ifdef __cplusplus
}
#endif

#endif
