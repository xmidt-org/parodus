#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#include "spin_thread.h"
#include "ParodusInternal.h"

void StartThread(void *(*start_routine) (void *))
{
        int err = 0;
	pthread_t threadId;

	err = pthread_create(&threadId, NULL, start_routine, NULL);
	if (err != 0) 
	{
		PARODUS_ERROR("Error creating thread :[%s]\n", strerror(err));
                exit(1);
	}
	else
	{
		PARODUS_DEBUG("Thread created Successfully %d\n", (int ) threadId);
	}    
}

        
