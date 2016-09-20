/**
 * @file wss_mgr.h
 *
 * @description This header defines functions required to manage WebSocket client connections.
 *
 * Copyright (c) 2015  Comcast
 */
 
#ifndef WEBSOCKET_MGR_H_
#define WEBSOCKET_MGR_H_

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
    unsigned int secureFlag;
} ParodusCfg;

/**
 * @brief Interface to create WebSocket client connections.
 * Loads the WebPA config file, if not provided by the caller,
 *  and creates the intial connection and manages the connection wait, close mechanisms.
 */
void __createSocketConnection(void *config, void (* initKeypress)());

/**
 * @brief Interface to create WebSocket client connections.
 * Loads the WebPA config file and creates the intial connection and manages the connection wait, close mechanisms.
 */
void createSocketConnection();

/**
 * @brief Interface to terminate WebSocket client connections and clean up resources.
 */
void terminateSocketConnection();
int parseCommandLine(int argc,char **argv,ParodusCfg * cfg);
#endif /* WEBSOCKET_MGR_H_ */

