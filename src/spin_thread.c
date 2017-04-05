/**
 * @file spin_thread.c
 *
 * @description This file is used to define thread function
 *
 * Copyright (c) 2015  Comcast
 */
 
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include "spin_thread.h"
#include "parodus_log.h"

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
void StartThread(void *(*start_routine) (void *))
{
    int err = 0;
	pthread_t threadId;

        assert(start_routine);
        
	err = pthread_create(&threadId, NULL, start_routine, NULL);
	if (err != 0) 
	{
		ParodusError("Error creating thread :[%s]\n", strerror(err));
        exit(1);
	}
	else
	{
		ParodusPrint("Thread created Successfully %d\n", (int ) threadId);
	}    
}

         
