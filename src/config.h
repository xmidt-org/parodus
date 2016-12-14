
#ifndef _CONFIG_H_ 
#define _CONFIG_H_

#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

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
