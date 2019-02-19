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
 * @file parodus_log.h
 *
 * @description This header defines parodus log levels
 *
 */
 
#include <stdarg.h>
#include <cimplog/cimplog.h>

#define LOGGING_MODULE "PARODUS"

/**
* @brief Enables or disables debug logs.
*/

#define ParodusError(...)                   cimplog_error(LOGGING_MODULE, __VA_ARGS__)
#define ParodusInfo(...)                    cimplog_info(LOGGING_MODULE, __VA_ARGS__)
#define ParodusPrint(...)                   cimplog_debug(LOGGING_MODULE, __VA_ARGS__)

#define OnboardLog(...)                     onboarding_log(LOGGING_MODULE, __VA_ARGS__)

