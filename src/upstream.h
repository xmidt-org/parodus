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
void getParodusUrl();
void sendUpstreamMsgToServer(void **resp_bytes, int resp_size);

#ifdef __cplusplus
}
#endif


#endif /* _UPSTREAM_H_ */

