/**
 * @file internal.h
 *
 * @description This file is used to manage internal functions of parodus
 *
 * Copyright (c) 2015  Comcast
 */
#ifndef _PARODUSINTERNAL_H_
#define _PARODUSINTERNAL_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <cJSON.h>
#include <nopoll.h>
#include <sys/time.h>
#include <sys/sysinfo.h>

#include <pthread.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <wrp-c.h>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>

#include "wss_mgr.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/


/* WebPA default Config */
#define WEBPA_SERVER_URL                                "talaria-beta.webpa.comcast.net"
#define WEBPA_SERVER_PORT                               8080
#define WEBPA_RETRY_INTERVAL_SEC                        10
#define WEBPA_MAX_PING_WAIT_TIME_SEC                    180

#define HTTP_CUSTOM_HEADER_COUNT                    	4
#define METADATA_COUNT 					11			
#define WEBPA_MESSAGE_HANDLE_INTERVAL_MSEC          	250
#define HEARTBEAT_RETRY_SEC                         	30      /* Heartbeat (ping/pong) timeout in seconds */
#define PARODUS_UPSTREAM "tcp://127.0.0.1:6666"
#define NANOMSG_SOCKET_TIMEOUT_MSEC                      2000

#define IOT "iot"
#define HARVESTER "harvester"
#define GET_SET "get_set"


/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

typedef struct ParodusMsg__
{
	noPollMsg * msg;
	void * payload;
	size_t len;
	struct ParodusMsg__ *next;
} ParodusMsg;

typedef struct UpStreamMsg__
{
	void *msg;
	size_t len;
	struct UpStreamMsg__ *next;
} UpStreamMsg;


typedef struct reg_list_item
{
	int sock;
	char service_name[32];
	char url[100];
	struct reg_list_item *next;
} reg_list_item_t;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif
    
int checkHostIp(char * serverIP);
void handleUpstreamMessage(noPollConn *conn, void *msg, size_t len);

void listenerOnMessage( void * msg, size_t msgSize, int *numOfClients, reg_list_item_t **head);

void parStrncpy(char *destStr, const char *srcStr, size_t destSize);

noPollPtr createMutex();

void lockMutex(noPollPtr _mutex);

void unlockMutex(noPollPtr _mutex);

void destroyMutex(noPollPtr _mutex);

void sendUpstreamMsgToServer(void **resp_bytes, int resp_size);


#ifdef __cplusplus
}
#endif
    

#endif

