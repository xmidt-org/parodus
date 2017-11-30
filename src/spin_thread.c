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
 * @file spin_thread.c
 *
 * @description This file is used to define thread function
 *
 */
 
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "spin_thread.h"
#include "parodus_log.h"

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void StartThread(void *(*start_routine) (void *))
{
    int err = 0;
	pthread_t threadId;

	err = pthread_create(&threadId, NULL, start_routine, NULL);
	if (err != 0) 
	{
		ParodusError("Error creating thread :[%s]\n", strerror(err));
        exit(1);
	}
	else
	{
		ParodusPrint("Thread created Successfully %lu\n", (unsigned long) threadId);
	}    
}

         
