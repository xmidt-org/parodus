#include "parodus_log.h"
#include "sysevent_handler.h"
#include "connection.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sysevent/sysevent.h>
#include "time.h"

static pthread_t sysevent_tid;
static int sysevent_fd;
static token_t sysevent_token;

static void *parodus_sysevent_handler (void *data)
{
	ParodusInfo("parodus_sysevent_handler thread\n");
	async_id_t conn_flush;
	sysevent_setnotification(sysevent_fd, sysevent_token, "firewall_flush_conntrack", &conn_flush);
	time_t time_now = { 0 }, time_before = { 0 };

	pthread_detach(pthread_self());

	for (;;)
    {
        char name[25]={0};
		char val[42]={0};
        int namelen = sizeof(name);
        int vallen  = sizeof(val);
        int err;
        async_id_t getnotification_asyncid;
        err = sysevent_getnotification(sysevent_fd, sysevent_token, name, &namelen,  val, &vallen, &getnotification_asyncid);

        if (err)
        {
			/* 
			   * Log should come for every 1hour 
			   * - time_now = getting current time 
			   * - difference between time now and previous time is greater than 
			   *	3600 seconds
			   * - time_before = getting current time as for next iteration 
			   *	checking		   
			   */ 
			time(&time_now);
			
			if(LOGGING_INTERVAL_SECS <= ((unsigned int)difftime(time_now, time_before)))
			{
				ParodusInfo("%s-**********ERR: %d\n", __func__, err);
				time(&time_before);
			}

		   sleep(10);
        }
		else 
		{
			if (strcmp(name, "firewall_flush_conntrack")==0)
		    {
			  int onFlush = atoi(val);
			  ParodusInfo("firewall_flush_conntrack %d \n",onFlush);
			  if(!onFlush) {
			  	ParodusInfo("Received firewall_flush_conntrack event, Close the connection and retry again \n");
				pthread_mutex_lock (&close_mut);
			    close_retry = true;
			    pthread_mutex_unlock (&close_mut);
			  } 		   
			}
	   }

	}
	return 0;
}

void ConnFlushHandler()
{
	sysevent_fd = sysevent_open("127.0.0.1", SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, "parodus-connFlush", &sysevent_token);

	if (sysevent_fd >= 0)
	{
		pthread_create(&sysevent_tid, NULL, parodus_sysevent_handler, NULL);
	}
	return;
}
