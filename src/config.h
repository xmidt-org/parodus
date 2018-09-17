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
#define CLOUD_STATUS            "cloud-status"
#define CLOUD_DISCONNECT        "cloud-disconnect"
#define CLOUD_STATUS_ONLINE     "online"
#define CLOUD_STATUS_OFFLINE    "offline"
#define CLOUD_DISCONNECT_REASON "disconnection-reason"
#define BOOT_RETRY_WAIT         "boot-time-retry-wait"

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
    char dns_txt_url[64];
    unsigned int acquire_jwt;
    unsigned int jwt_algo;  // bit mask set for each allowed algorithm
    char jwt_key[4096]; // may be read in from a pem file
    char cert_path[64];
    char webpa_auth_token[4096];
    char token_acquisition_script[64];
    char token_read_script[64];
#ifdef ENABLE_MUTUAL_AUTH
    char client_cert_path[64];
    char client_key_path[64];
#endif
	char *crud_config_file;
	char *cloud_status;
	char *cloud_disconnect;
	unsigned int boot_retry_wait;
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
void set_parodus_cfg(ParodusCfg *);
char *get_token_application(void) ;
void set_cloud_disconnect_reason(ParodusCfg *cfg, char *disconn_reason);
void reset_cloud_disconnect_reason(ParodusCfg *cfg);
/**
 * parse a webpa url. Extract the server address, the port
 * and return whether it's secure or not
 *
 * @param full_url	full url
 * @param server_addr	ptr to a server address ptr
 *                      will be NULL if invalid,
 *                      otherwise will need to be freed
 * @param port 		ptr to port variable
 * @return 1 if insecure connection is allowed, 0 if not,
*    or -1 if error
*/ 
int parse_webpa_url (const char *full_url, 
	char **server_addr, unsigned int *port);

#ifdef __cplusplus
}
#endif

#endif
