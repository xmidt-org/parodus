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

typedef struct _socket_handles
{
        int pipeline;
        int pubsub;
} socket_handles_t;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

// Producer thread to pull and add P2P Incoming queue items 
// from the peer 2 peer pipe
void *handle_P2P_Incoming();

// Consumer thread to process the P2P Incoming queue items 
void *process_P2P_IncomingMessage();

// Consumer thread to process the P2P Outgoing queue items 
void *process_P2P_OutgoingMessage();

// Add outgoing messages to the P2P Outgoing queue
void add_P2P_OutgoingMessage(void **message, size_t len);

#ifdef __cplusplus
}
#endif


#endif /* _PEER2PEER_H_ */
