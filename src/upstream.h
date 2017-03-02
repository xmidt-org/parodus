/**
 * @file upstream.h
 *
 * @description This header defines functions required to manage upstream messages.
 *
 * Copyright (c) 2015  Comcast
 */
 
#ifndef _UPSTREAM_H_
#define _UPSTREAM_H_
#include <nopoll.h>
#include "config.h"
#ifdef __cplusplus
extern "C" {
#endif

void *handle_upstream();
void *handleUpStreamEvents();
void sendUpstreamMsgToServer(void **resp_bytes, int resp_size);

#ifdef __cplusplus
}
#endif


#endif /* _UPSTREAM_H_ */

