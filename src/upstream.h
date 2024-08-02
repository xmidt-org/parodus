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
 * @file upstream.h
 *
 * @description This header defines functions required to manage upstream messages.
 *
 */
 
#ifndef _UPSTREAM_H_
#define _UPSTREAM_H_

#include <pthread.h>
#include <wrp-c.h>
#ifdef ENABLE_WEBCFGBIN
#include <rbus.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
	
/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct UpStreamMsg__
{
	void *msg;
	size_t len;
	struct UpStreamMsg__ *next;
} UpStreamMsg;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
#ifdef ENABLE_WEBCFGBIN
int regConnOnlineEvent();
rbusError_t SendConnOnlineEvent();
rbusError_t CloudConnSubscribeHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish);
#endif
void packMetaData();
void *handle_upstream();
void *processUpstreamMessage();
void registerRBUSlistener();
int getDeviceId(char **device_id, size_t *device_id_len);
int sendUpstreamMsgToServer(void **resp_bytes, size_t resp_size);
void getServiceNameAndSendResponse(wrp_msg_t *msg, void **msg_bytes, size_t msg_size);
void createUpstreamRetrieveMsg(wrp_msg_t *message, wrp_msg_t **retrieve_msg);
void set_global_UpStreamMsgQ(UpStreamMsg * UpStreamQ);
#ifdef WAN_FAILOVER_SUPPORTED
int subscribeCurrentActiveInterfaceEvent();
#endif
UpStreamMsg * get_global_UpStreamMsgQ(void);
pthread_cond_t *get_global_nano_con(void);
pthread_mutex_t *get_global_nano_mut(void);
void clear_metadata();

#ifdef __cplusplus
}
#endif


#endif /* _UPSTREAM_H_ */

