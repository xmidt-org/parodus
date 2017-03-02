/**
 * @file parodus_log.h
 *
 * @description This header defines parodus log levels
 *
 * Copyright (c) 2015  Comcast
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
