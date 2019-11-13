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
 * @file close_retry.h
 *
 * @description Functions required to manage connection close retry.
 *
 */

#ifndef _CLOSERETRY_H_
#define _CLOSERETRY_H_

#include <pthread.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Get value of close_retry
bool get_close_retry();

// Reset value of close_retry to false
void reset_close_retry();

// Set value of close_retry to true
void set_close_retry();

#ifdef __cplusplus
}
#endif
    
#endif
