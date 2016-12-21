/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#ifndef  _LIBPARODUS_QUEUES_H
#define  _LIBPARODUS_QUEUES_H

#include <errno.h>

typedef void *libpd_mq_t;

int libpd_qcreate (libpd_mq_t *mq, const char *queue_name, unsigned max_msgs);

typedef void free_msg_func_t (void *msg);

int libpd_qdestroy (libpd_mq_t *mq, free_msg_func_t *free_msg_func);

// returns 0 on success, ETIMEDOUT, or other error
int libpd_qsend (libpd_mq_t mq, void *msg, unsigned timeout_ms);

// returns 0 on success, ETIMEDOUT, or other error
int libpd_qreceive (libpd_mq_t mq, void **msg, unsigned timeout_ms);

#endif
