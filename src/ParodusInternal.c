/**
 * @file internal.c
 *
 * @description This file is used to manage internal functions of parodus
 *
 * Copyright (c) 2015  Comcast
 */

#include "ParodusInternal.h"
#include "wss_mgr.h"

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/


/**
 * @brief checkHostIp interface to check if the Host server DNS is resolved to correct IP Address.
 * @param[in] serverIP server address DNS
 * Converts HostName to Host IP Address and checks if it is not same as 10.0.0.1 so that we can proceed with connection
 */
int checkHostIp(char * serverIP)
{
	printf("...............Inside checkHostIp..............%s \n", serverIP);
	int status = -1;
	struct addrinfo *res, *result;
	int retVal;
	char addrstr[100];
	void *ptr;
	char *localIp = "10.0.0.1";

	struct addrinfo hints;
	memset(&hints,0,sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;

	retVal = getaddrinfo(serverIP, "http", &hints, &result);
	if (retVal != 0) 
	{
		printf("getaddrinfo: %s\n", gai_strerror(retVal));
	}
	else
	{
		res = result;
		while(res)
		{  
			ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
			inet_ntop (res->ai_family, ptr, addrstr, 100);
			printf ("IPv4 address of %s is %s \n", serverIP, addrstr);
			if (strcmp(localIp,addrstr) == 0)
			{
				printf("Host Ip resolved to 10.0.0.1\n");
				status = -2;
			}
			else
			{
				printf("Host Ip resolved correctly, proceeding with the connection\n");
				status = 0;
				break;
			}
			res = res->ai_next;
		}
		freeaddrinfo(result);
	}
	return status; 
}  




/** To send upstream msgs to server ***/

void handleUpstreamMessage(noPollConn *conn, void *msg, size_t len)
{
	int bytesWritten = 0;
	
	printf("handleUpstreamMessage length %zu\n", len);
	printf("conn object is %s \n", conn);
	if(nopoll_conn_is_ok(conn) && nopoll_conn_is_ready(conn))
	{
		bytesWritten = nopoll_conn_send_binary(conn, msg, len);
		printf("Number of bytes written: %d\n", bytesWritten);
		if (bytesWritten != (int) len) 
		{
			printf("Failed to send bytes %zu, bytes written were=%d (errno=%d, %s)..\n", len, bytesWritten, errno, strerror(errno));
		}
	}
	else
	{
		printf("Failed to send msg upstream as connection is not OK\n");
	}
	
}
  
