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
 * @file connection.c
 *
 * @description This decribes functions required to manage WebSocket client connections.
 *
 */

#ifndef _HEARBEAT_H_
#define _HEARBEAT_H_

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif


unsigned int get_heartBeatTimer();

void reset_heartBeatTimer();

void increment_heartBeatTimer(unsigned int inc_time_ms);

#ifdef __cplusplus
}
#endif
    
#endif
