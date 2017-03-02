/**
 * @file internal.h
 *
 * @description This file is used to manage internal functions of parodus
 *
 * Copyright (c) 2015  Comcast
 */
#ifndef _SERVICE_ALIVE_H_
#define _SERVICE_ALIVE_H_

#include <stdio.h>

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif
  
extern int numOfClients;
void *serviceAliveTask();


#ifdef __cplusplus
}
#endif
    

#endif

