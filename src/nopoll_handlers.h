/**
 * @file nopoll_handlers.h
 *
 * @description This header defines nopoll handler functions.
 *
 * Copyright (c) 2015  Comcast
 */

#ifndef _NOPOLL_HANDLERS_H_
#define _NOPOLL_HANDLERS_H_

#include "ParodusInternal.h"

#ifdef __cplusplus
extern "C" {
#endif

void listenerOnMessage_queue(noPollCtx * ctx, noPollConn * conn, noPollMsg * msg,noPollPtr user_data);
void listenerOnPingMessage (noPollCtx * ctx, noPollConn * conn, noPollMsg * msg, noPollPtr user_data);
void listenerOnCloseMessage (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data);

#ifdef __cplusplus
}
#endif
    
#endif
