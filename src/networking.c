/**
 * Copyright 2016 Comcast Cable Communications Management, LLC
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
#include <cimplog/cimplog.h>
#include "ParodusInternal.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
/**
 *  Interface to check if the Host server DNS is resolved to an IP address that
 *  is not 10.0.0.1.
 *
 *  @param[in] serverIP server address DNS
 *
 *  @returns 0 on success, error otherwise
 */
int checkHostIp(char * serverIP)
{
	int status = -1;
	struct addrinfo *res, *result;
	int retVal;
	char addrstr[100];
	void *ptr;
	char *localIp = "10.0.0.1";

	struct addrinfo hints;


	cimplog_debug("PARODUS", "...............Inside checkHostIp..............%s \n", serverIP);

	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;

	retVal = getaddrinfo(serverIP, "http", &hints, &result);
	if (retVal != 0) 
	{
		cimplog_error("PARODUS", "getaddrinfo: %s\n", gai_strerror(retVal));
	}
	else
	{
		res = result;
		while(res)
		{  
			ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
			inet_ntop (res->ai_family, ptr, addrstr, 100);
		
			cimplog_debug("PARODUS", "IPv4 address of %s is %s \n", serverIP, addrstr);
			if (strcmp(localIp,addrstr) == 0)
			{
				cimplog_debug("PARODUS", "Host Ip resolved to 10.0.0.1\n");
				status = -2;
			}
			else
			{
				cimplog_debug("PARODUS", "Host Ip resolved correctly, proceeding with the connection\n");
				status = 0;
				break;
			}
			res = res->ai_next;
		}
		freeaddrinfo(result);
	}
	return status; 
}  

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
