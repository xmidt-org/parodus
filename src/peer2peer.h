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
typedef struct P2P_Msg__
{
	void *msg;
	size_t len;
	struct P2P_Msg__ *next;
} P2P_Msg;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

// Thread to pull, add and process P2P Incoming queue items 
void *handle_and_process_P2P_IncomingMessage();

// Consumer thread to process the P2P Outgoing queue items 
void *process_P2P_OutgoingMessage();

// Add outgoing messages to the P2P Outgoing queue
void add_P2P_OutgoingMessage(void **message, size_t len);

#ifdef __cplusplus
}
#endif


#endif /* _PEER2PEER_H_ */
