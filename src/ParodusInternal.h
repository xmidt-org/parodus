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
#include "math.h"
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
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <wrp-c.h>
#include <libwebsockets.h>

#include "parodus_log.h"
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define UNUSED(x) (void )(x)
#define NANOMSG_SOCKET_TIMEOUT_MSEC                      2000

#ifndef TEST
#define FOREVER()   1
#else
extern int numLoops;
#define FOREVER()   numLoops--
#endif

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

typedef struct ParodusMsg__
{
	void * payload;
	size_t len;
	struct ParodusMsg__ *next;
} ParodusMsg;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
extern bool LastReasonStatus;
extern ParodusMsg *ParodusMsgQ;
int numLoops;
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

void parStrncpy(char *destStr, const char *srcStr, size_t destSize);

char* getWebpaConveyHeader();

#ifdef __cplusplus
}
#endif
    

#endif

