/**
 * Copyright 2022 Comcast Cable Communications Management, LLC
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
 * @file xmidtsend_rbus.h
 *
 * @description This header defines functions required to manage xmidt send messages.
 *
 */
 
#ifndef _XMIDTSEND_RBUS_H_
#define _XMIDTSEND_RBUS_H_
#include <rbus.h>
#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct XmidtMsg__
{
	rbusObject_t *msg;
	struct XmidtMsg__ *next;
} XmidtMsg;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

rbusHandle_t get_parodus_rbus_Handle(void);
void addToXmidtUpstreamQ(rbusObject_t* inParams);
void* processXmidtUpstreamMsg();
void processXmidtData();
void parseData(rbusObject_t * msg);
int processXmidtEvent(rbusObject_t inParams);
void sendXmidtEventToServer(char *source, char *destination, char*contenttype, int qos, void *payload, unsigned int payload_len);
#ifdef __cplusplus
}
#endif


#endif /* _XMIDTSEND_RBUS_H_ */

