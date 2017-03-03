/**
 * @file spin_thread.h
 *
 * @description This file is used to define thread function
 *
 * Copyright (c) 2015  Comcast
 */
 
#ifndef _SPIN_THREAD_H_
#define _SPIN_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

void StartThread(void *(*start_routine) (void *));


#ifdef __cplusplus
}
#endif

#endif


