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
 *  @param pipeline_url  [in]  URL
 *  @param pubsub_url    [in]  URL
 *  @param topics        [in]  string array of topics
 *  @param pipeline_sock [out] sock
 *  @param pubsub_sock   [out] sock
 *
 *  @return true if success, false if failure
 */
bool spoke_setup(const char *pipeline_url, const char *pubsub_url, const char **topics, int *pipeline_sock, int *pubsub_sock);

/**
 *  @note Call this after finishing use of any function here to cleanup
 *
 *  @param pipeline_sock handle
 *  @param pubsub_sock   handle
 */
void spoke_cleanup(int pipeline_sock, int pubsub_sock);

/**
 *  @note Call this with valid parameter before using any function
 *
 *  @param pipeline_url  [in]  URL
 *  @param pubsub_url    [in]  URL
 *  @param pipeline_sock [out] sock
 *  @param pubsub_sock   [out] sock
 *
 *  @return true if success, false if failure
 */
bool hub_setup(const char *pipeline_url, const char *pubsub_url, int *pipeline_sock, int *pubsub_sock);

/**
 *  @note Call this after finishing use of any function here to cleanup
 *
 *  @param pipeline_sock handle
 *  @param pubsub_sock   handle
 */
void hub_cleanup(int pipeline_sock, int pubsub_sock);

/**
 *  Send message to hub parodus.
 *
 *  @param sock socket handle 
 *  @param msg  notification
 *  @param size size of notification
 *
 *  @return whether operation succeeded, or not.
 */
bool spoke_send_msg(int sock, const void *msg, size_t size);

/**
 *  Check for message from spoke parodus.
 *
 *  @note msg needs to be cleaned up by the caller.
 *
 *  @param sock   socket handle
 *  @param msg    address of message buffer
 *
 *  @return size of msg
 */
ssize_t hub_check_inbox(int pipeline_sock, void **msg);

/**
 *  Check for message from spoke parodus.
 *
 *  @note msg needs to be cleaned up by the caller.
 *
 *  @param sock   socket handle
 *  @param msg    address of message buffer
 *
 *  @return size of msg
 */
ssize_t spoke_check_inbox(int sock, void **msg);

/**
 *  Send message to spoke parodus
 * 
 *  @param sock socket handle
 *  @param msg  notification
 *  @param size size of notification
 *
 *  @return whether operation succeeded, or not.
 */
bool hub_send_msg(int sock, const void *msg, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _PARODUS_INTERFACE_H_ */

