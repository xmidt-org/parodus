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

void packMetaData();
void *handle_upstream();
void *processUpstreamMessage();

void sendUpstreamMsgToServer(void **resp_bytes, size_t resp_size);
void sendToAllRegisteredClients(void **resp_bytes, size_t resp_size);

#ifdef __cplusplus
}
#endif


#endif /* _UPSTREAM_H_ */

