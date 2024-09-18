/**
 * Copyright 2024 Comcast Cable Communications Management, LLC
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
 * @file partners_check.h
 *
 * @description This describes functions to validate partner_id.
 *
 */
 
#ifndef _RDKCONFIG_GENERIC_H_
#define _RDKCONFIG_GENERIC_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RDKCONFIG_OK 0
#define RDKCONFIG_FAIL 1

int rdkconfig_get( uint8_t **buf, size_t *buffsize, const char *reference );

int rdkconfig_set( const char *reference, uint8_t *buf, size_t buffsize );

int rdkconfig_free( uint8_t **buf, size_t buffsize );

#ifdef __cplusplus
}
#endif

#endif /* _RDKCONFIG_GENERIC_H_ */
