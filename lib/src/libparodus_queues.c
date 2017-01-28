/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#include "libparodus_queues.h"
#include "libparodus_time.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <cimplog.h>

typedef struct queue {
	const char *queue_name;
	unsigned max_msgs;
	int msg_count;
	pthread_mutex_t mutex;
	pthread_cond_t not_empty_cond;
	pthread_cond_t not_full_cond;
	void **msg_array;
	int head_index;
	int tail_index;
} queue_t;

int libpd_qcreate (libpd_mq_t *mq, const char *queue_name, unsigned max_msgs)
{
	int err;
	unsigned array_size;
	queue_t *newq;

	*mq = NULL;
	if (max_msgs < 2) {
		cimplog_error("LIBPARODUS: ", 
			"Error creating queue %s: max_msgs(%u) should be at least 2\n",
			queue_name, max_msgs);
		return EINVAL;
	}
		
	array_size = max_msgs * sizeof(void*);
	newq = (queue_t*) malloc (sizeof(queue_t));

	if (NULL == newq) {
		cimplog_error("LIBPARODUS: ", "Unable to allocate memory(1) for queue %s\n",
			queue_name);
		return ENOMEM;
	}

	newq->queue_name = queue_name;
	newq->max_msgs = max_msgs;
	newq->msg_count = 0;
	newq->head_index = -1;
	newq->tail_index = -1;

	err = pthread_mutex_init (&newq->mutex, NULL);
	if (err != 0) {
		cimplog_error("LIBPARODUS: ", "Error creating mutex for queue %s\n",
			queue_name);
		free (newq);
		return err;
	}

	err = pthread_cond_init (&newq->not_empty_cond, NULL);
	if (err != 0) {
		cimplog_error("LIBPARODUS: ", "Error creating not_empty_cond for queue %s\n",
			queue_name);
		pthread_mutex_destroy (&newq->mutex);
		free (newq);
		return err;
	}

	err = pthread_cond_init (&newq->not_full_cond, NULL);
	if (err != 0) {
		cimplog_error("LIBPARODUS: ", "Error creating not_full_cond for queue %s\n",
			queue_name);
		pthread_mutex_destroy (&newq->mutex);
		pthread_cond_destroy (&newq->not_empty_cond);
		free (newq);
		return err;
	}

	newq->msg_array = malloc (array_size);
	if (NULL == newq->msg_array) {
		cimplog_error("LIBPARODUS: ", "Unable to allocate memory(2) for queue %s\n",
			queue_name);
		pthread_mutex_destroy (&newq->mutex);
		pthread_cond_destroy (&newq->not_empty_cond);
		pthread_cond_destroy (&newq->not_full_cond);
		free (newq);
		return ENOMEM;
	}

	*mq = (libpd_mq_t) newq;
	return 0;
}



static bool enqueue_msg (queue_t *q, void *msg)
{
	if (q->msg_count == 0) {
		q->msg_array[0] = msg;
		q->head_index = 0;
		q->tail_index = 0;
		q->msg_count = 1;
		return true;
	}
	if (q->msg_count >= (int)q->max_msgs)
		return false;
	q->tail_index += 1;
	if (q->tail_index >= (int)q->max_msgs)
		q->tail_index = 0;
	q->msg_array[q->tail_index] = msg;
	q->msg_count += 1;
	return true;
}

static void *dequeue_msg (queue_t *q)
{
	void *msg;
	if (q->msg_count <= 0)
		return NULL;
	msg = q->msg_array[q->head_index];
	q->head_index += 1;
	if (q->head_index >= (int)q->max_msgs)
		q->head_index = 0;
	q->msg_count -= 1;
	return msg;
}

int libpd_qdestroy (libpd_mq_t *mq, free_msg_func_t *free_msg_func)
{
	queue_t *q = (queue_t*) *mq;
	void *msg;
	if (NULL == *mq)
		return 0;
	pthread_mutex_lock (&q->mutex);
	if (NULL != free_msg_func) {
		msg = dequeue_msg (q);
		while (NULL != msg) {
			(*free_msg_func) (msg);
			msg = dequeue_msg (q);
		}
	}
	free (q->msg_array);
	pthread_cond_destroy (&q->not_empty_cond);
	pthread_cond_destroy (&q->not_full_cond);
	pthread_mutex_unlock (&q->mutex);
	pthread_mutex_destroy (&q->mutex);
	free (q);
	*mq = NULL;
	return 0;
}

int libpd_qsend (libpd_mq_t mq, void *msg, unsigned timeout_ms)
{
	queue_t *q = (queue_t*) mq;
	struct timespec ts;
	int rtn;

	if (NULL == mq)
		return EINVAL;
	pthread_mutex_lock (&q->mutex);
	while (true) {
		if (enqueue_msg (q, msg))
			break;
		get_expire_time (timeout_ms, &ts);
		rtn = pthread_cond_timedwait (&q->not_full_cond, &q->mutex, &ts);
		if (rtn != 0) {
			if (rtn != ETIMEDOUT)
				cimplog_error("LIBPARODUS: ", 
					"pthread_cond_timedwait error waiting for not_full_cond\n");
			pthread_mutex_unlock (&q->mutex);
			return rtn;
		}
	}
	if (q->msg_count == 1)
		pthread_cond_signal (&q->not_empty_cond);
	pthread_mutex_unlock (&q->mutex);
	return 0;
}

int libpd_qreceive (libpd_mq_t mq, void **msg, unsigned timeout_ms)
{
	queue_t *q = (queue_t*) mq;
	struct timespec ts;
	void *msg__;
	int rtn;

	if (NULL == mq)
		return EINVAL;
	pthread_mutex_lock (&q->mutex);
	while (true) {
		msg__ = dequeue_msg (q);
		if (NULL != msg__)
			break;
		get_expire_time (timeout_ms, &ts);
		rtn = pthread_cond_timedwait (&q->not_empty_cond, &q->mutex, &ts);
		if (rtn != 0) {
			if (rtn != ETIMEDOUT)
				cimplog_error("LIBPARODUS: ", 
					"pthread_cond_timedwait error waiting for not_empty_cond\n");
			pthread_mutex_unlock (&q->mutex);
			return rtn;
		}
	}
	*msg = msg__;
	if ((q->msg_count+1) == (int)q->max_msgs)
		pthread_cond_signal (&q->not_full_cond);
	pthread_mutex_unlock (&q->mutex);
	return 0;
}

