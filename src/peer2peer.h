/**
 * @file peer2peer.h
 *
 * @description This header defines structures & functions required 
 * to manage parodus peer to peer messages.
 *
 * Copyright (c) 2018  Comcast
 */
 
#ifndef _PEER2PEER_H_
#define _PEER2PEER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct P2P_inQ__
{
	void *msg;
	size_t len;
	struct P2P_inQ__ *next;
} P2P_inQ;

typedef struct P2P_outQ__
{
	void *msg;
	size_t len;
	struct P2P_outQ__ *next;
} P2P_outQ;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

void *handle_P2P_Incoming();
void *process_P2P_IncomingMessage();

void *handle_P2P_Outgoing();
void *process_P2P_OutgoingMessage();

#ifdef __cplusplus
}
#endif


#endif /* _PEER2PEER_H_ */

