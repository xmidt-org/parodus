/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 * Copyright [2014] [Cisco Systems, Inc.]
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/* This is a test version of event_handler.c that can be used
 * to simulate interface down, interface up event
 * You overwrite event_handler.c in the src diectory with this
 * version.  It will generate interface down / interface up events
 * at random intervals between 60 secs and 124 secs
 */

#include "parodus_log.h"
#include "event_handler.h"
#include "connection.h"
#include "config.h"
#include "heartBeat.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "time.h"
#include "close_retry.h"

extern bool g_shutdown;

static pthread_t sysevent_tid;

static void start_interface_down (void)
{
	  	set_interface_down_event();
		ParodusInfo("Interface_down_event is set\n");				
		pause_heartBeatTimer();
}

static void end_interface_down (void)
{
		reset_interface_down_event();
		ParodusInfo("Interface_down_event is reset\n");
		resume_heartBeatTimer();
		set_close_retry();
}

// waits from 60 to 124 secs
int wait_random (const char *msg)
{
  #define HALF_SEC 500000l
  long delay = (random() >> 5) + 60000000l;
  long secs, usecs;
  struct timeval timeout;

  secs = delay / 1000000;
  usecs = delay % 1000000;
  ParodusInfo ("Waiting %ld secs %ld usecs for %s\n", secs, usecs, msg);
  
  while (!g_shutdown) {
	timeout.tv_sec = 0;
	if (delay <= HALF_SEC) {
	  timeout.tv_usec = delay;
	  select (0, NULL, NULL, NULL, &timeout);
	  return 0;
	}
	timeout.tv_usec = HALF_SEC;
	delay -= HALF_SEC;
	select (0, NULL, NULL, NULL, &timeout);
  }
  return -1;    
}


static void *parodus_sysevent_handler (void *data)
{
	while (!g_shutdown) {
	  if (wait_random ("interface down") != 0)
	    break;
	  start_interface_down ();
	  wait_random ("interface up");
	  end_interface_down ();	
	}
	ParodusInfo ("Exiting event handler\n");
	return data;
}

void EventHandler()
{
	ParodusInfo ("RAND_MAX is %ld (0x%lx)\n", RAND_MAX, RAND_MAX);
	srandom (getpid());

	pthread_create(&sysevent_tid, NULL, parodus_sysevent_handler, NULL);
}
