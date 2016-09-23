/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/
#ifndef  _PARODUS_WRP_IFC_H
#define  _PARODUS_WRP_IFC_H

#include <wrp-c.h>

/**
 * This module is linked with the client, and provides connectivity
 * to the parodus service.
 */ 


/**
 * @brief Msg Handler Function registered by the client
 * which will be called whenever a wrp message is received
 * on the interface
 *
 * @note the handler function must free the message when finished
 * with it, by calling wrp_free_struct.
 * 
 * @param msg The wrp message received.
 *
 */
typedef void (*pwiMsgHandler) (wrp_msg_t ** msg);

/**
 * Initialize the parodus wrp interface
 *
 * @param parodus_service_url URL of the parodus service
 * @param my_url  my URL
 * @param msg_handler handler to receive all log notifications
 * @return 0 on success, valid errno otherwise.
 */
int parodus_wrp_ifc_init (const char *parodus_service_url, const char *my_url,
		pwiMsgHandler msg_handler);

/**
 * Shut down the parodus wrp interface
 *
 */
int parodus_wrp_ifc_shutdown (void);

/**
 * Send a wrp message to the parodus service
 *
 * @param msg wrp message to send
 * @note This function should not be called within a pwiMsgHandler,
 * as it could lead to a deadlock.
 * @return 0 on success, -1 otherwise.
 */
int pwi_send_msg (wrp_msg_t **msg);


#endif
