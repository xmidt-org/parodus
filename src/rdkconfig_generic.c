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
 * @file rdkconfig_generic.c
 *
 * @description This file is to fetch authorization token during parodus cloud connection.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rdkconfig_generic.h"
#include "parodus_log.h"

int rdkconfig_get( uint8_t **buf, size_t *buffsize, const char *reference )
{
	/* This is stub function, No need implemetation */
	ParodusInfo("Inside rdkconfig_get stub function.\n");
	if ( reference == NULL ) {
		ParodusError( "rdkconfig_get: error, bad argument\n" );
		return RDKCONFIG_FAIL;
	}

	if(strcmp(reference,"/tmp/.cfgStaticxpki") == 0)
	{
		*buf = strdup("xxx");
        *buffsize = 3;
	}
	else
	{
		*buf = strdup("yyy\n");
        *buffsize = 4;
	}
	return RDKCONFIG_OK;
}

int rdkconfig_set( const char *reference, uint8_t *buf, size_t buffsize )
{
	/* This is stub function, No need implemetation */
	ParodusInfo("Inside rdkconfig_set stub function.\n");	
	return RDKCONFIG_OK;
}

int rdkconfig_free( uint8_t **buf, size_t buffsize )
{
	ParodusInfo("Inside rdkconfig_free stub function.\n");
	if ( buf == NULL ) return RDKCONFIG_FAIL;
	if ( *buf == NULL ) {
		return RDKCONFIG_OK; // ok if pointer is null
	}
	memset( *buf, 0, buffsize );
	free( *buf );
	buf = NULL;
	return RDKCONFIG_OK;
}
