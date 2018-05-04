/**
 * Copyright 2018 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
/**
 * @file parodus_interface.h
 *
 * @description Declares parodus-to-parodus API.
 *
 */
 
#ifndef _PARODUS_INTERFACE_H_
#define _PARODUS_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/**
 *  @note Call this with valid parameter before using any function
 *
 *  @param url    URL to receive messages from other parodoi.
 *
 *  @return true if success, false if failure
 */
bool spoke_setup_listener(const char *url);

/**
 *  @note Call this after finishing use of any function here to cleanup
 *
 *  @param url    URL to receive messages from other parodoi.
 */
void spoke_cleanup_listener(void);

/**
 *  @note Call this with valid parameter before using any function
 *
 *  @param url    URL to receive messages from other parodoi.
 *
 *  @return true if success, false if failure
 */
bool hub_setup_listener(const char *url);

/**
 *  @note Call this after finishing use of any function here to cleanup
 *
 *  @param url    URL to receive messages from other parodoi.
 */
void hub_cleanup_listener(void);

/**
 *  Send message to hub parodus.
 * 
 *  @param url  hub parodus URL
 *  @param msg  notification
 *  @param size size of notification
 *
 *  @return whether operation succeeded, or not.
 */
bool spoke_send_msg(const char *url, const void *msg, size_t size);

/**
 *  Check for message from spoke parodus.
 *
 *  @note msg needs to be cleaned up by the caller.
 *
 *  @param msg    address of message buffer
 *
 *  @return size of msg
 */
ssize_t hub_check_inbox(void **msg);

/**
 *  Check for message from spoke parodus.
 *
 *  @note msg needs to be cleaned up by the caller.
 *
 *  @param msg    address of message buffer
 *
 *  @return size of msg
 */
ssize_t spoke_check_inbox(void **msg);

/**
 *  Send message to spoke parodus
 * 
 *  @param url  spoke parodus URL
 *  @param msg  notification
 *  @param size size of notification
 *
 *  @return whether operation succeeded, or not.
 */
bool hub_send_msg(const char *url, const void *msg, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _PARODUS_INTERFACE_H_ */

