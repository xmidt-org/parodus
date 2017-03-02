/**
 * @file ParodusInternal.h
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
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <cJSON.h>
#include <nopoll.h>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <wrp-c.h>

#include "parodus_log.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define UNUSED(x) (void )(x)

/* WebPA default Config */
#define WEBPA_SERVER_PORT                               8080
#define HTTP_CUSTOM_HEADER_COUNT                    	4
#define METADATA_COUNT 					11			
#define HEARTBEAT_RETRY_SEC                         	30      /* Heartbeat (ping/pong) timeout in seconds */
#define PARODUS_UPSTREAM "tcp://127.0.0.1:6666"
#define NANOMSG_SOCKET_TIMEOUT_MSEC                      2000

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

extern void *metadataPack;
extern size_t metaPackSize;

extern bool LastReasonStatus;
extern char *reconnect_reason;

extern ParodusMsg *ParodusMsgQ;
extern UpStreamMsg *UpStreamMsgQ;

extern pthread_mutex_t g_mutex;
extern pthread_cond_t g_cond; 
extern pthread_mutex_t close_mut;

extern int numOfClients;
extern reg_list_item_t * head;

extern volatile unsigned int heartBeatTimer;
extern bool close_retry;

extern volatile bool terminated;
extern char parodus_url[32];

int checkHostIp(char * serverIP);

void parStrncpy(char *destStr, const char *srcStr, size_t destSize);

char* getWebpaConveyHeader();

void packMetaData();

void getParodusUrl();


#ifdef __cplusplus
}
#endif
    

#endif

