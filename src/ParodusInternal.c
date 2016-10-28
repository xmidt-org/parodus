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



/**
 * @brief listenerOnMessage function to create WebSocket listener to receive connections
 *
 * @param[in] ctx The context where the connection happens.
 * @param[in] conn The Websocket connection object
 * @param[in] msg The message received from server for various process requests
 * @param[out] user_data data which is to be sent
 */
void listenerOnMessage(void * msg, size_t msgSize, int *numOfClients, reg_client **clients)
{
	
	int rv =0;
	wrp_msg_t *message;
	char* destVal = NULL;
	char dest[32] = {'\0'};
	
	int msgType;
	int p =0;
	int bytes =0;
	int destFlag =0;	
	const char *recivedMsg = NULL;
	recivedMsg =  (const char *) msg;
	
	printf("Received msg from server:%s\n", recivedMsg);	
	if(recivedMsg!=NULL) 
	{
	
		/*** Decoding downstream recivedMsg to check destination ***/
		
		rv = wrp_to_struct(recivedMsg, msgSize, WRP_BYTES, &message);
				
		if(rv > 0)
		{
			printf("\nDecoded recivedMsg of size:%d\n", rv);
			msgType = message->msg_type;
			printf("msgType received:%d\n", msgType);
		
			if((message->u.req.dest !=NULL))
			{
				destVal = message->u.req.dest;
				strtok(destVal , "/");
				strcpy(dest,strtok(NULL , "/"));
				printf("Received downstream dest as :%s\n", dest);
			
				//Checking for individual clients & Sending to each client
				
				for( p = 0; p < *numOfClients; p++ ) 
				{
					printf("clients[%d].service_name is %s \n",p, clients[p]->service_name);
				    // Sending message to registered clients
				    if( strcmp(dest, clients[p]->service_name) == 0) 
				    {  
				    	printf("sending to nanomsg client %s\n", dest);     
					bytes = nn_send(clients[p]->sock, recivedMsg, msgSize, 0);
					printf("sent downstream message '%s' to reg_client '%s'\n",recivedMsg,clients[p]->url);
					printf("downstream bytes sent:%d\n", bytes);
					destFlag =1;
			
				    } 
				    
				}
				
				if(destFlag ==0)
				{
					printf("Unknown dest:%s\n", dest);
				}
			
		  	 }
	  	}
	  	
	  	else
	  	{
	  		printf( "Failure in msgpack decoding for receivdMsg: rv is %d\n", rv );
	  	}
	  	
	  	printf("free for downstream decoded msg\n");
	  	wrp_free_struct(message);
	  

        }
                
     
}


void parStrncpy(char *destStr, const char *srcStr, size_t destSize)
{
    strncpy(destStr, srcStr, destSize-1);
    destStr[destSize-1] = '\0';
}




/** 
 * @brief createMutex Nopoll create mutex handler
 */ 
noPollPtr createMutex()
{
  	pthread_mutexattr_t attr;
	pthread_mutex_t * mutex;
  	int rtn;
	mutex = nopoll_new (pthread_mutex_t, 1);
	if (mutex == NULL)
	{
		printf("Failed to create mutex\n");
		return NULL;
	}
	pthread_mutexattr_init( &attr);
	/*pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK);*/
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE);
	
	/* init the mutex using default values */
	rtn = pthread_mutex_init (mutex, &attr);
  	pthread_mutexattr_destroy (&attr);
	if (rtn != 0) 
	{
		printf("Error in init Mutex\n");
		nopoll_free(mutex);
		return NULL;
	}
	else 
	{
		printf("mutex init successfully\n");
	} 

	return mutex;
}



/** 
 * @brief lockMutex Nopoll lock mutex handler
 */
void lockMutex(noPollPtr _mutex)
{
	int rtn;
	char errbuf[100];
	/* printf("Inside Lock mutex\n"); */
	if (_mutex == NULL)
	{
		printf("Received null mutex\n");
		return;
	}
	pthread_mutex_t * mutex = _mutex;

	/* lock the mutex */
	rtn = pthread_mutex_lock (mutex);
	if (rtn != 0) 
	{
		strerror_r (rtn, errbuf, 100);
		printf("Error in Lock mutex: %s\n", errbuf);
		/* do some reporting */
		return;
	}
	else
	{
		/* printf("Mutex locked \n"); */
	} /* end if */
	return;
}


/** 
 * @brief unlockMutex Nopoll unlock mutex handler
 */
void unlockMutex(noPollPtr _mutex)
{
	int rtn;
	char errbuf[100];
	/* printf("Inside unlock mutex\n"); */
	if (_mutex == NULL)
	{
		printf("Received null mutex\n");
		return;
	}
	pthread_mutex_t * mutex = _mutex;

	/* unlock mutex */
	rtn = pthread_mutex_unlock (mutex);
	if (rtn != 0) 
	{
		/* do some reporting */
		strerror_r (rtn, errbuf, 100);
		printf("Error in unlock mutex: %s\n", errbuf);
		return;
	}
	else
	{
		/* printf("Mutex unlocked \n"); */
	} /* end if */
	return;
}



/** 
 * @brief destroyMutex Nopoll destroy mutex handler
 */
void destroyMutex(noPollPtr _mutex)
{
	if (_mutex == NULL)
	{
		printf("Received null mutex\n");
		return;
	}
	pthread_mutex_t * mutex = _mutex;
	
	if (pthread_mutex_destroy (mutex) != 0) 
	{
		/* do some reporting */
		printf("problem in destroy\n");
		return;
	}
	else
	{
		printf("Mutex destroyed \n");
	}
	nopoll_free (mutex);

	return;
}
  
