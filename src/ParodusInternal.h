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
 * @file ParodusInternal.h
 *
 * @description This file is used to manage internal functions of parodus
 *
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
#include <nopoll.h>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include <wrp-c.h>

#include "parodus_log.h"
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define UNUSED(x) (void )(x)
#define NANO_SOCKET_SEND_TIMEOUT_MS                     2000
#define NANO_SOCKET_RCV_TIMEOUT_MS			500

#ifndef TEST
#define FOREVER()   1
#else
extern int numLoops;
#define FOREVER()   numLoops--
#endif

// Return values for find_servers() in connection.c
#define FIND_SUCCESS 0
#define FIND_INVALID_DEFAULT -2
#define FIND_JWT_FAIL -1


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

// Used in token.c to get jwt from dns server
typedef struct {
	const char *rr_ptr;
	int rr_len;
} rr_rec_t;

//------------ Used in comnection.c -----------------
typedef struct {
  int allow_insecure;
  char *server_addr;  // must be freed
  unsigned int port;
} server_t;

typedef struct {
  server_t defaults;	// from command line
  server_t jwt;		// from jwt endpoint claim
  server_t redirect;	// from redirect response to
			//  nopoll_conn_wait_until_connection_ready
} server_list_t;

//---- Used in connection.c for expire timer
typedef struct {
  int running;
  struct timespec start_time;
  struct timespec end_time;
} expire_timer_t;

//--- Used in connection.c for backoff delay timer
typedef struct {
  int count;
  int max_count;
  int delay;
} backoff_timer_t;

//--- Used in connection.c for init_header_info
typedef struct {
  char *conveyHeader;	// Do not free
  char *device_id;	// Need to free
  char *user_agent;	// Need to free
} header_info_t;

// connection context which is defined in createNopollConnection
// and passed into functions keep_retrying_connect, connect_and_wait,
// wait_connection_ready, and nopoll_connect 
typedef struct {
  noPollCtx *nopoll_ctx;
  server_list_t server_list;
  server_t *current_server;
  header_info_t header_info;
  char *extra_headers;		// need to be freed
  expire_timer_t connect_timer;
} create_connection_ctx_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
extern bool g_shutdown;
extern ParodusMsg *ParodusMsgQ;
int numLoops;
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

int checkHostIp(char * serverIP);

void parStrncpy(char *destStr, const char *srcStr, size_t destSize);

char* getWebpaConveyHeader();

void *CRUDHandlerTask();
void addCRUDmsgToQueue(wrp_msg_t *crudMsg);

void timespec_diff(struct timespec *start, struct timespec *stop,
                   struct timespec *result);


/*------------------------------------------------------------------------------*/
/*                              For Wan-staus Flag                              */
/*------------------------------------------------------------------------------*/

// Get value of wan_stop_flag
bool get_wan_stop_flag();

// Reset value of wan_stop_flag to false
void reset_wan_stop_flag();

// Set value of wan_stop_flag to true
void set_wan_stop_flag();
  

#ifdef __cplusplus
}
#endif
    

#endif

