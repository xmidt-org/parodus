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
 * @file heartBeat.c
 *
 * @description This decribes functions required to manage heartBeatTimer.
 *
 */

#include "heartBeat.h"

volatile unsigned int heartBeatTimer = 0;

pthread_mutex_t heartBeat_mut=PTHREAD_MUTEX_INITIALIZER;

// Get value of heartBeatTimer
unsigned int get_heartBeatTimer() 
{
	unsigned int tmp = 0;
	pthread_mutex_lock (&heartBeat_mut);
	tmp = heartBeatTimer;
	pthread_mutex_unlock (&heartBeat_mut);
	return tmp;
}

// Reset value of heartBeatTimer to 0
void reset_heartBeatTimer() 
{
	pthread_mutex_lock (&heartBeat_mut);
	heartBeatTimer = 0;
	pthread_mutex_unlock (&heartBeat_mut);
}

// Increment value of heartBeatTimer to desired value
void increment_heartBeatTimer(unsigned int inc_time_ms) 
{
	pthread_mutex_lock (&heartBeat_mut);
	heartBeatTimer += inc_time_ms;
	pthread_mutex_unlock (&heartBeat_mut);
}


