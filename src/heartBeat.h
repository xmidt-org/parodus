/**
 * Copyright 2018 Comcast Cable Communications Management, LLC
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
 * @file heartBeat.h
 *
 * @description This decribes functions required to manage heartBeatTimer.
 *
 */

#ifndef _HEARBEAT_H_
#define _HEARBEAT_H_

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// Get value of heartBeatTimer
unsigned int get_heartBeatTimer();

// Reset value of heartBeatTimer to 0
void reset_heartBeatTimer();

// Increment value of heartBeatTimer to desired value
void increment_heartBeatTimer(unsigned int inc_time_ms);

// Pause heartBeatTimer, i.e. stop incrementing
void pause_heartBeatTimer();

// Resume heartBeatTimer, i.e. resume incrementing
void resume_heartBeatTimer();

#ifdef __cplusplus
}
#endif
    
#endif
