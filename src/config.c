#include <string.h>
#include "ParodusInternal.h"
#include "config.h"

void loadParodusCfg(ParodusCfg * config,ParodusCfg *cfg)
{
    ParodusCfg *pConfig =config;
    
    if(strlen (pConfig->hw_model) !=0)
    {
        strncpy(cfg->hw_model, pConfig->hw_model,strlen(pConfig->hw_model)+1);
        
    }
    else
    {
        PARODUS_DEBUG("hw_model is NULL. read from tmp file\n");
    }
    if( strlen(pConfig->hw_serial_number) !=0)
    {
        strncpy(cfg->hw_serial_number, pConfig->hw_serial_number,strlen(pConfig->hw_serial_number)+1);
    }
    else
    {
        PARODUS_DEBUG("hw_serial_number is NULL. read from tmp file\n");
    }
    if(strlen(pConfig->hw_manufacturer) !=0)
    {
        strncpy(cfg->hw_manufacturer, pConfig->hw_manufacturer,strlen(pConfig->hw_manufacturer)+1);
    }
    else
    {
        PARODUS_DEBUG("hw_manufacturer is NULL. read from tmp file\n");
    }
    if(strlen(pConfig->hw_mac) !=0)
    {
       strncpy(cfg->hw_mac, pConfig->hw_mac,strlen(pConfig->hw_mac)+1);
    }
    else
    {
        PARODUS_DEBUG("hw_mac is NULL. read from tmp file\n");
    }
    if(strlen (pConfig->hw_last_reboot_reason) !=0)
    {
         strncpy(cfg->hw_last_reboot_reason, pConfig->hw_last_reboot_reason,strlen(pConfig->hw_last_reboot_reason)+1);
    }
    else
    {
        PARODUS_DEBUG("hw_last_reboot_reason is NULL. read from tmp file\n");
    }
    if(strlen(pConfig->fw_name) !=0)
    {   
        strncpy(cfg->fw_name, pConfig->fw_name,strlen(pConfig->fw_name)+1);
    }
    else
    {
        PARODUS_DEBUG("fw_name is NULL. read from tmp file\n");
    }
    if( strlen(pConfig->webpa_url) !=0)
    {
        strncpy(cfg->webpa_url, pConfig->webpa_url,strlen(pConfig->webpa_url)+1);
    }
    else
    {
        PARODUS_DEBUG("webpa_url is NULL. read from tmp file\n");
    }
    if(strlen(pConfig->webpa_interface_used )!=0)
    {
        strncpy(cfg->webpa_interface_used, pConfig->webpa_interface_used,strlen(pConfig->webpa_interface_used)+1);
    }
    else
    {
        PARODUS_DEBUG("webpa_interface_used is NULL. read from tmp file\n");
    }
        
    cfg->boot_time = pConfig->boot_time;
    cfg->webpa_ping_timeout = pConfig->webpa_ping_timeout;
    cfg->webpa_backoff_max = pConfig->webpa_backoff_max;
    strncpy(cfg->webpa_protocol, WEBPA_PROTOCOL_VALUE, strlen(WEBPA_PROTOCOL_VALUE)+1);
    strncpy(cfg->webpa_uuid, "1234567-345456546", strlen("1234567-345456546")+1);
    PARODUS_DEBUG("cfg->webpa_uuid is :%s\n", cfg->webpa_uuid);
    
      
}
