/**
 * @file thread_tasks.h
 *
 * @description This header defines thread functions.
 *
 * Copyright (c) 2015  Comcast
 */
 
#ifndef _THREAD_TASKS_H_
#define _THREAD_TASKS_H_

#include <pthread.h>
#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
extern pthread_mutex_t g_mutex;
extern pthread_cond_t g_cond; 
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

void *messageHandlerTask();

#ifdef __cplusplus
}
#endif


#endif /* THREAD_TASKS_H_ */

