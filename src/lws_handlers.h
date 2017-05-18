/**
 * @file lws_handlers.h
 *
 * @description This header defines lws handler functions.
 *
 * Copyright (c) 2015  Comcast
 */

#ifndef _LWS_HANDLERS_H_
#define _LWS_HANDLERS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

extern pthread_mutex_t g_mutex;
extern pthread_cond_t g_cond; 
extern bool close_retry;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/**
 * @brief listenerOnrequest_queue function to add messages to the queue
 *
 * @param[in] reqSize size of the incoming message
 * @param[in] requestMsg The message received from server for various process requests
 */
void listenerOnrequest_queue(void *requestMsg,int reqSize);


#ifdef __cplusplus
}
#endif
    
#endif
