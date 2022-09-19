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
#include "time.h"
#include <stdbool.h>

volatile unsigned int heartBeatTimer = 0;
volatile bool paused = false;
volatile long long pingTimeStamp = 0;

pthread_mutex_t heartBeat_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ping_mut=PTHREAD_MUTEX_INITIALIZER;

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
	if (!paused)
	  heartBeatTimer += inc_time_ms;
	pthread_mutex_unlock (&heartBeat_mut);
}

// Pause heartBeatTimer, i.e. stop incrementing
void pause_heartBeatTimer()
{
	pthread_mutex_lock (&heartBeat_mut);
	heartBeatTimer = 0;
	paused = true;
	pthread_mutex_unlock (&heartBeat_mut);
}

// Resume heartBeatTimer, i.e. resume incrementing
void resume_heartBeatTimer()
{
	pthread_mutex_lock (&heartBeat_mut);
	paused = false;
	pthread_mutex_unlock (&heartBeat_mut);
}

// Set ping received timeStamp
void set_pingTimeStamp()
{
	struct timespec ts;
	getCurrentTime(&ts);
	pthread_mutex_lock (&ping_mut);
	pingTimeStamp = (long long)ts.tv_sec;
	pthread_mutex_unlock (&ping_mut);
}

// Get ping received timeStamp
long long get_pingTimeStamp()
{
	long long tmp = 0;
	pthread_mutex_lock (&ping_mut);
	tmp = pingTimeStamp;
	pthread_mutex_unlock (&ping_mut);
	return tmp;
}
