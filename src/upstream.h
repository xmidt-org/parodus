/**
 * @file upstream.h
 *
 * @description This header defines functions required to manage upstream messages.
 *
 * Copyright (c) 2015  Comcast
 */
 
#ifndef _UPSTREAM_H_
#define _UPSTREAM_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct UpStreamMsg__
{
	void *msg;
	size_t len;
	struct UpStreamMsg__ *next;
} UpStreamMsg;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

void packMetaData();
void *handle_upstream();
void *processUpstreamMessage();
const char *getParodusUrl();
void sendUpstreamMsgToServer(void **resp_bytes, size_t resp_size);

#ifdef __cplusplus
}
#endif

#define MAX_PARODUS_URL_SIZE 32

#endif /* _UPSTREAM_H_ */

