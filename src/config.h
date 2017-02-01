
#ifndef _CONFIG_H_ 
#define _CONFIG_H_

#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

/* WRP CRUD Model Macros */
#define HW_MODELNAME                                	"hw-model"
#define HW_SERIALNUMBER                                 "hw-serial-number"
#define HW_MANUFACTURER                                 "hw-manufacturer"
#define HW_DEVICEMAC                                  	"hw-mac"
#define HW_LAST_REBOOT_REASON                           "hw-last-reboot-reason"
#define FIRMWARE_NAME                                  	"fw-name"
#define BOOT_TIME                                  	"boot-time"
#define LAST_RECONNECT_REASON                           "webpa-last-reconnect-reason"
#define WEBPA_PROTOCOL                                  "webpa-protocol"
#define WEBPA_INTERFACE                                 "webpa-inteface-used"
#define WEBPA_UUID                                      "webpa-uuid"
#define WEBPA_URL                                       "webpa-url"
#define WEBPA_PING_TIMEOUT                              "webpa-ping-timeout"
#define WEBPA_BACKOFF_MAX                               "webpa-backoff-max"

#define WEBPA_PROTOCOL_VALUE 							"WebPA-1.6"
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
    unsigned int webpa_backoff_max;
    char webpa_interface_used[16];
    char webpa_protocol[16];
    char webpa_uuid[64];
    unsigned int secureFlag;
} ParodusCfg;

void loadParodusCfg(ParodusCfg * config,ParodusCfg *cfg);


#ifdef __cplusplus
}
#endif

#endif
