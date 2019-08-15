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
 * @file close_retry.c
 *
 * @description Functions required to manage connection close retry.
 *
 */

#include "close_retry.h"

bool close_retry = false;

pthread_mutex_t close_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t close_con=PTHREAD_COND_INITIALIZER;

pthread_cond_t *get_global_close_retry_con(void)
{
    return &close_con;
}

pthread_mutex_t *get_global_close_retry_mut(void)
{
    return &close_mut;
}

// Get value of close_retry
bool get_close_retry() 
{
	bool tmp = false;
	pthread_mutex_lock (&close_mut);
	tmp = close_retry;
	pthread_mutex_unlock (&close_mut);
	return tmp;
}

// Reset value of close_retry to false
void reset_close_retry() 
{
	pthread_mutex_lock (&close_mut);
	close_retry = false;
	pthread_mutex_unlock (&close_mut);
}

// set value of close_retry to true
void set_close_retry() 
{
    pthread_mutex_lock (&close_mut);
    close_retry = true;
    pthread_cond_signal(&close_con);
    pthread_mutex_unlock (&close_mut);
}


