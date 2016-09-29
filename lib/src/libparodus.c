/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>
#include "libparodus.h"
#include <pthread.h>
#include <mqueue.h>

#define SERVER_URL "tcp://127.0.0.1:6666"
//#define SERVER_URL "ipc:///tmp/parodus_server.ipc"

#define CLIENT_URL "tcp://127.0.0.1:6667"
//#define CLIENT_URL "ipc:///tmp/parodus_client.ipc"

#define END_MSG "---END-PARODUS---\n"
const char *end_msg = END_MSG;

typedef struct {
	int len;
	char *msg;
} raw_msg_t;

static int rcv_sock = -1;
static int stop_rcv_sock = -1;
static int send_sock = -1;

#define MAX_QUEUE_MSG_SIZE 64

static mqd_t raw_queue = -1;
static mqd_t wrp_queue = -1;

static pthread_t raw_receiver_tid;
static pthread_t wrp_receiver_tid;

static const char *selected_service;

static int flush_wrp_queue (uint32_t delay_ms);
static void *raw_receiver_thread (void *arg);
static void *wrp_receiver_thread (void *arg);

#define RUN_STATE_RUNNING		1234
#define RUN_STATE_DONE			-1234

static volatile int run_state = 0;


#define LEVEL_NO_LOGGER 99
#define LEVEL_ERROR 0
#define LEVEL_INFO  1
#define LEVEL_DEBUG 2

static void libpd_log ( int level, int os_errno, const char *msg, ...)
{
	char errbuf[100];

	va_list arg_ptr;
	va_start(arg_ptr, msg);
	vprintf(msg, arg_ptr);
	va_end(arg_ptr);
	if (os_errno != 0)
		printf ("%s\n", strerror_r (os_errno, errbuf, 100));
}

static int connect_receiver (const char *rcv_url)
{
	int sock;

	if (NULL == rcv_url) {
		rcv_sock = -1;
		return -1;
	}
  sock = nn_socket (AF_SP, NN_PULL);
	if (sock < 0) {
		libpd_log (LEVEL_ERROR, errno, "Unable to create rcv socket\n");
 		return -1;
	}
  if (nn_bind (sock, rcv_url) < 0) {
		libpd_log (LEVEL_ERROR, errno, "Unable to bind to receive socket %s\n", rcv_url);
		return -1;
	}
	rcv_sock = sock;
	return 0;
}

static void shutdown_socket (int *sock)
{
	if (*sock != -1)
		nn_shutdown (*sock, 0);
	*sock = -1;
}

static int connect_sender (const char *send_url)
{
	int sock;

	if (NULL == send_url) {
		return -1;
	}
  sock = nn_socket (AF_SP, NN_PUSH);
	if (sock < 0) {
		libpd_log (LEVEL_ERROR, errno, "Unable to create send socket: %s\n");
 		return -1;
	}
  if (nn_connect (sock, send_url) < 0) {
		libpd_log (LEVEL_ERROR, errno, "Unable to connect to send socket %s\n",
			send_url);
		return -1;
	}
	return sock;
}

static mqd_t create_queue (const char *qname, int qsize)
{
	mqd_t q = -1;
	struct mq_attr attr;
	attr.mq_maxmsg = qsize;
	attr.mq_msgsize = MAX_QUEUE_MSG_SIZE;
	attr.mq_flags = 0;
	q = mq_open (qname, O_RDWR | O_CREAT, 0666, &attr);
	if (q == -1)
		libpd_log (LEVEL_ERROR, errno, "Unable to create queue %s\n", qname);
	return q;
}

static int create_thread (pthread_t *tid, void *(*thread_func) (void*))
{
	int rtn = pthread_create (tid, NULL, thread_func, NULL);
	if (rtn != 0)
		libpd_log (LEVEL_ERROR, rtn, "Unable to create thread\n");
	return rtn; 
}
 
int libparodus_init (const char *service_name)
{
	if (RUN_STATE_RUNNING == run_state) {
		libpd_log (LEVEL_ERROR, 0, "LIBPARODUS: already running at init\n");
		return EALREADY;
	}
	if (0 != run_state) {
		libpd_log (LEVEL_ERROR, 0, "LIBPARODUS: not idle at init\n");
		return EBUSY;
	}

	selected_service = service_name;
	if (connect_receiver (CLIENT_URL) != 0)
		return -1;
	send_sock = connect_sender (SERVER_URL);
	if (send_sock != 0) {
		shutdown_socket (&rcv_sock);
		return -1;
	}
	stop_rcv_sock = connect_sender (CLIENT_URL);
	if (stop_rcv_sock != 0) {
		shutdown_socket (&rcv_sock);
		shutdown_socket (&send_sock);
		return -1;
	}
	raw_queue = create_queue ("/LIBPD_RAW_QUEUE", 256);
	if (raw_queue < 0) {
		shutdown_socket (&rcv_sock);
		shutdown_socket (&send_sock);
		shutdown_socket (&stop_rcv_sock);
		return -1;
	}
	wrp_queue = create_queue ("/LIBPD_WRP_QUEUE", 24);
	if (wrp_queue < 0) {
		shutdown_socket (&rcv_sock);
		mq_close (raw_queue);
		shutdown_socket (&send_sock);
		shutdown_socket (&stop_rcv_sock);
		return -1;
	}
	if (create_thread (&wrp_receiver_tid, wrp_receiver_thread) != 0) {
		shutdown_socket (&rcv_sock);
		mq_close (raw_queue);
		mq_close (wrp_queue);
		shutdown_socket (&send_sock);
		shutdown_socket (&stop_rcv_sock);
		return -1;
	}
	if (create_thread (&raw_receiver_tid, raw_receiver_thread) != 0) {
		shutdown_socket (&rcv_sock);
		pthread_cancel (wrp_receiver_tid);
		mq_close (raw_queue);
		mq_close (wrp_queue);
		shutdown_socket (&send_sock);
		shutdown_socket (&stop_rcv_sock);
		return -1;
	}
	
	return 0;
}

static int queue_send (mqd_t q, const char *qname, const char *msg, int len)
{
	int rtn;
	if (len < 0)
		len = strlen (msg) + 1; // include terminating null
	rtn = mq_send (q, msg, len, 0);
	if (rtn != 0)
		libpd_log (LEVEL_ERROR, errno, "Unable to send on queue %s\n", qname);
	return rtn; 
}

// msgbuf must be MAX_MSG_QUEUE_SIZE bytes long
static int queue_receive (mqd_t q, const char *qname, char *msgbuf, int *len)
{
	ssize_t bytes = mq_receive (q, msgbuf, MAX_QUEUE_MSG_SIZE, NULL);
	if (bytes < 0) {
		libpd_log (LEVEL_ERROR, errno, "Unable to receive on queue %s\n", qname);
		return -1;
	}
	*len = (int) bytes;
	return 0;
}

static int sock_send (int sock, const char *msg, int msg_len)
{
  int bytes;
	if (msg_len < 0)
		msg_len = strlen (msg) + 1; // include terminating null
	bytes = nn_send (sock, msg, msg_len, 0);
  if (bytes < 0) { 
		libpd_log (LEVEL_ERROR, errno, "Error sending msg\n");
		return -1;
	}
  if (bytes != msg_len) {
		libpd_log (LEVEL_ERROR, 0, "Not all bytes sent, just %d\n", bytes);
		return -1;
	}
	return 0;
}

static int sock_receive (raw_msg_t *msg)
{
	char *buf = NULL;
  msg->len = nn_recv (rcv_sock, &buf, NN_MSG, 0);

  if (msg->len < 0) { 
		libpd_log (LEVEL_ERROR, errno, "Error receiving msg\n");
		return -1;
	}
	msg->msg = buf;
	return 0;
}

int libparodus_shutdown (void)
{
	int rtn;

	run_state = RUN_STATE_DONE;
	sock_send (stop_rcv_sock, end_msg, -1);
 	rtn = pthread_join (raw_receiver_tid, NULL);
	if (rtn != 0)
		libpd_log (LEVEL_ERROR, rtn, "Error terminating raw receiver thread\n");
	shutdown_socket (&rcv_sock);
	flush_wrp_queue (5);
	queue_send (wrp_queue, "/WRP_QUEUE", end_msg, -1);
 	rtn = pthread_join (wrp_receiver_tid, NULL);
	if (rtn != 0)
		libpd_log (LEVEL_ERROR, rtn, "Error terminating wrp receiver thread\n");
	mq_close (raw_queue);
	mq_close (wrp_queue);
	shutdown_socket (&send_sock);
	shutdown_socket (&stop_rcv_sock);
	run_state = 0;
	return 0;
}

static int get_expire_time (uint32_t ms, struct timespec *ts)
{
	struct timeval tv;
	int err = gettimeofday (&tv, NULL);
	if (err != 0) {
		libpd_log (LEVEL_ERROR, errno, "Error getting time of day\n");
		return err;
	}
	tv.tv_sec += ms/1000;
	tv.tv_usec += (ms%1000) * 1000;
	if (tv.tv_usec >= 1000000) {
		tv.tv_sec += 1;
		tv.tv_usec -= 1000000;
	}

	ts->tv_sec = tv.tv_sec;
	ts->tv_nsec = tv.tv_usec * 1000L;
	return 0;
}

// returns 0 OK
//  1 timed out
// -1 mq_receive error
// -2 msg size error, not a ptr
static int timed_wrp_queue_receive (wrp_msg_t **msg, struct timespec *expire_time)
{
	ssize_t bytes;
	char msgbuf[MAX_QUEUE_MSG_SIZE];

	bytes = mq_timedreceive (wrp_queue, msgbuf, MAX_QUEUE_MSG_SIZE, NULL,
		expire_time);
	if (bytes < 0) {
		if (errno == ETIMEDOUT)
			return 1;
		libpd_log (LEVEL_ERROR, errno, "Unable to receive on queue /WRP_QUEUE\n");
		return -1;
	}
	if (bytes != sizeof(wrp_msg_t *)) {
		libpd_log (LEVEL_ERROR, 0, 
			"Invalid msg (not a wrp_msg_t pointer) in wrp queue receive\n");
		return -2;
	}
	memcpy ((void*) msg, (const void*)msgbuf, sizeof(wrp_msg_t *));

	return 0;
}

int libparodus_receive (wrp_msg_t **msg, uint32_t ms)
{
	struct timespec ts;
	int err;

	if (RUN_STATE_RUNNING != run_state) {
		libpd_log (LEVEL_NO_LOGGER, 0, "LIBPARODUS: not running at receive\n");
		return -1;
	}

	err = get_expire_time (ms, &ts);
	if (err != 0) {
		return err;
	}

	err = timed_wrp_queue_receive (msg, &ts);
	if (err == 1)
		return ETIMEDOUT;
	if (err != 0)
		return -1;
	return 0;
}


int libparodus_send (wrp_msg_t *msg)
{
	int rtn;
	ssize_t msg_len;
	void *msg_bytes;

	if (RUN_STATE_RUNNING != run_state) {
		libpd_log (LEVEL_NO_LOGGER, 0, "LIBPARODUS: not running at send\n");
		return -1;
	}

	msg_len = wrp_struct_to (msg, WRP_BYTES, &msg_bytes);
	if (msg_len < 1) {
		libpd_log (LEVEL_ERROR, 0, "LIBPARODUS: error converting WRP to bytes\n");
		return -1;
	}

	rtn = sock_send (send_sock, (const char *)msg_bytes, msg_len);
	free (msg_bytes);
	return rtn;
}

static void *raw_receiver_thread (void *arg)
{
	int rtn;
	raw_msg_t msg;
	int end_msg_len = strlen(end_msg);

	while (1) {
		rtn = sock_receive (&msg);
		if (rtn != 0)
			break;
		if (msg.len >= end_msg_len)
			if (strncmp (msg.msg, end_msg, end_msg_len) == 0)
				break;
		if (RUN_STATE_RUNNING != run_state) {
			nn_freemsg (msg.msg);
			continue;
		}
		queue_send (raw_queue, "/RAW_QUEUE", (const char *) &msg, sizeof(raw_msg_t));

	}
	return NULL;
}

static char *find_wrp_msg_dest (wrp_msg_t *wrp_msg)
{
	if (wrp_msg->msg_type == WRP_MSG_TYPE__REQ)
		return wrp_msg->u.req.dest;
	if (wrp_msg->msg_type == WRP_MSG_TYPE__EVENT)
		return wrp_msg->u.event.dest;
	if (wrp_msg->msg_type == WRP_MSG_TYPE__CREATE)
		return wrp_msg->u.crud.dest;
	if (wrp_msg->msg_type == WRP_MSG_TYPE__RETREIVE)
		return wrp_msg->u.crud.dest;
	if (wrp_msg->msg_type == WRP_MSG_TYPE__UPDATE)
		return wrp_msg->u.crud.dest;
	if (wrp_msg->msg_type == WRP_MSG_TYPE__DELETE)
		return wrp_msg->u.crud.dest;
	if (wrp_msg->msg_type == WRP_MSG_TYPE__SVC_REGISTRATION)
		return wrp_msg->u.reg.service_name;
	return NULL;
}

static void *wrp_receiver_thread (void *arg)
{
	int rtn, msg_len;
	raw_msg_t raw_msg;
	wrp_msg_t *wrp_msg;
	int end_msg_len = strlen(end_msg);
	char *dest;
	char msg_buf[MAX_QUEUE_MSG_SIZE];

	while (1) {
		rtn = queue_receive (raw_queue, "/RAW_QUEUE", msg_buf, &msg_len);
		if (rtn != 0)
			break;
		if (msg_len >= end_msg_len)
			if (strncmp (msg_buf, end_msg, end_msg_len) == 0)
				break;
		memcpy ((void*)&raw_msg, (const void*)msg_buf, sizeof(raw_msg_t));
		if (RUN_STATE_RUNNING != run_state) {
			nn_freemsg (raw_msg.msg);
			continue;
		}
 		msg_len = (int) wrp_to_struct (raw_msg.msg, raw_msg.len, WRP_BYTES, &wrp_msg);
		if (msg_len < 1) {
			libpd_log (LEVEL_ERROR, 0, "LIBPARODUS: error converting bytes to WRP\n");
			continue;
		}
		// Pass thru all AUTH messages
		// Pass thru REQ, EVENT, and CRUD if dest matches the selected service
		// Pass thru Registration messages if service name matches the selected service.
		dest = find_wrp_msg_dest (wrp_msg);
		if (NULL != dest) {
			if (strcmp (dest, selected_service) != 0) {
				wrp_free_struct (wrp_msg);
				continue;
			}
		}
		queue_send (wrp_queue, "/WRP_QUEUE", (const char *) &wrp_msg, 
			sizeof(wrp_msg_t *));
	}
	return NULL;
}

static int flush_wrp_queue (uint32_t delay_ms)
{
	wrp_msg_t *wrp_msg;
	struct timespec ts;
	int err = get_expire_time (delay_ms, &ts);
	if (err != 0) {
		return err;
	}
	
	while (1) {
		err = timed_wrp_queue_receive (&wrp_msg, &ts);
		if (err == 1)	// timed out
			break;
		if (err == -2) // bad msg
			continue;
		if (err != 0)
			return -1;
		wrp_free_struct (wrp_msg);
	}
	return 0;
}





