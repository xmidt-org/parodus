/**
 * @file seshat_interface.h
 *
 * @description This header defines interface to register seshat service.
 *
 * Copyright (c) 2015  Comcast
 */
 
#ifndef _SESHAT_INTERFACE_H_
#define _SESHAT_INTERFACE_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/**
 * @brief Helper function to register with seshat.
 * 
 * @note return whether successfully registered.
 *
 * @return true when registered, false otherwise.
 */
bool __registerWithSeshat();
   
#ifdef __cplusplus
}
#endif
    
#endif
