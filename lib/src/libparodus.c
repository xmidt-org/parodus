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
#include "libparodus_time.h"
#include <pthread.h>
#include <mqueue.h>

#define PARODUS_URL "127.0.0.1:6666"
//#define PARODUS_URL "ipc:///tmp/parodus_server.ipc"

#define CLIENT_URL "127.0.0.1:6667"
//#define CLIENT_URL "ipc:///tmp/parodus_client.ipc"

//const char *parodus_url = PARODUS_URL;
//const char *client_url = CLIENT_URL;

char parodus_url[32] = {'\0'};
char client_url[32] = {'\0'};


#define SOCK_SEND_TIMEOUT_MS 2000

#define END_MSG "---END-PARODUS---\n"
static const char *end_msg = END_MSG;

static char *closed_msg = "---CLOSED---\n";
static wrp_msg_t wrp_closed_msg;
static wrp_msg_t *closed_msg_ptr = &wrp_closed_msg;

typedef struct {
	int len;
	char *msg;
} raw_msg_t;

static int rcv_sock = -1;
static int stop_rcv_sock = -1;
static int send_sock = -1;

#define MAX_QUEUE_MSG_SIZE 64

#define RAW_QUEUE_NAME "/LIBPD_RAW_QUEUE"
#define WRP_QUEUE_NAME "/LIBPD_WRP_QUEUE"

const char *raw_queue_name = RAW_QUEUE_NAME;
const char *wrp_queue_name = WRP_QUEUE_NAME;

static mqd_t raw_queue = (mqd_t)-1;
static mqd_t wrp_queue = (mqd_t)-1;

static pthread_t raw_receiver_tid;
static pthread_t wrp_receiver_tid;

static const char *selected_service;

static void make_closed_msg (wrp_msg_t *msg);
static int wrp_sock_send (wrp_msg_t *msg);
static int flush_wrp_queue (uint32_t delay_ms);
static void *raw_receiver_thread (void *arg);
static void *wrp_receiver_thread (void *arg);
static void getParodusUrl();

#define RUN_STATE_RUNNING		1234
#define RUN_STATE_DONE			-1234

static volatile int run_state = 0;
static volatile bool auth_received = false;

#if 0
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
#endif

//Call free in failure case for parodus_url and client_url
static void getParodusUrl()
{
	const char *parodusIp = NULL;
	const char *clientIp = NULL;
	const char * envParodus = getenv ("PARODUS_URL");
	const char * envClient = getenv ("CLIENT_URL");
	char * protocol = "tcp://";
  if( envParodus != NULL)
  {
    parodusIp = envParodus;
  }
  else
  {
    parodusIp = PARODUS_URL;
  }
  snprintf(parodus_url,sizeof(parodus_url),"%s%s",protocol, parodusIp);
  printf("formatted parodus Url %s\n",parodus_url);
  
  if( envClient != NULL)
  {
    clientIp = envClient;
  }
  else
  {
    clientIp = CLIENT_URL;
  }
  snprintf(client_url,sizeof(client_url),"%s%s",protocol, clientIp);
  printf("formatted client Url %s\n",client_url);
	
}

bool is_auth_received (void)
{
	return auth_received;
}

int connect_receiver (const char *rcv_url)
{
	int sock;

	if (NULL == rcv_url) {
		return -1;
	}
  sock = nn_socket (AF_SP, NN_PULL);
	if (sock < 0) {
		libpd_log (LEVEL_ERROR, errno, "Unable to create rcv socket %s\n", rcv_url);
 		return -1;
	}
  if (nn_bind (sock, rcv_url) < 0) {
		libpd_log (LEVEL_ERROR, errno, "Unable to bind to receive socket %s\n", rcv_url);
		return -1;
	}
	return sock;
}

#define SHUTDOWN_SOCKET(sock) \
	if ((sock) != -1) \
		nn_shutdown ((sock), 0); \
	(sock) = 0;

void shutdown_socket (int *sock)
{
	if (*sock != -1) {
		nn_shutdown (*sock, 0);
		nn_close (*sock);
	}
	*sock = -1;
}

int connect_sender (const char *send_url)
{
	int sock;
	int send_timeout = SOCK_SEND_TIMEOUT_MS;

	if (NULL == send_url) {
		return -1;
	}
  sock = nn_socket (AF_SP, NN_PUSH);
	if (sock < 0) {
		libpd_log (LEVEL_ERROR, errno, "Unable to create send socket: %s\n", send_url);
 		return -1;
	}
	if (nn_setsockopt (sock, NN_SOL_SOCKET, NN_SNDTIMEO, 
				&send_timeout, sizeof (send_timeout)) < 0) {
		libpd_log (LEVEL_ERROR, errno, "Unable to set socket timeout: %s\n", send_url);
 		return -1;
	}
  if (nn_connect (sock, send_url) < 0) {
		libpd_log (LEVEL_ERROR, errno, "Unable to connect to send socket %s\n",
			send_url);
		return -1;
	}
	return sock;
}

mqd_t create_queue (const char *qname, int qsize __attribute__ ((unused)) )
{
	mqd_t q = -1;
	struct mq_attr attr;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = MAX_QUEUE_MSG_SIZE;
	attr.mq_flags = 0;
	mq_unlink (qname);
	q = mq_open (qname, O_RDWR | O_CREAT, 0666, &attr);
	//q = mq_open (qname, O_RDWR | O_CREAT, 0666, NULL);
	if (q == (mqd_t) -1) {
		libpd_log (LEVEL_ERROR, errno, "Unable to create queue %s\n", qname);
	}
	if (mq_getattr (q, &attr) != 0) {
		libpd_log (LEVEL_ERROR, errno, "mq_getattr error\n");
		mq_close (q);
		return (mqd_t)-1;
	}
	libpd_log (LEVEL_DEBUG, 0, "Queue %s max msgs %d, max msg size %d\n",
		qname, attr.mq_maxmsg, attr.mq_msgsize);
	return q;
}

static int create_thread (pthread_t *tid, void *(*thread_func) (void*))
{
	int rtn = pthread_create (tid, NULL, thread_func, NULL);
	if (rtn != 0)
		libpd_log (LEVEL_ERROR, rtn, "Unable to create thread\n");
	return rtn; 
}

static int send_registration_msg (const char *service_name)
{
	wrp_msg_t reg_msg;
	reg_msg.msg_type = WRP_MSG_TYPE__SVC_REGISTRATION;
	reg_msg.u.reg.service_name = (char *) service_name;
	reg_msg.u.reg.url = client_url;
	return wrp_sock_send (&reg_msg);
}
 
int libparodus_init (const char *service_name, parlibLogHandler log_handler)
{
	int err;

	if (RUN_STATE_RUNNING == run_state) {
		libpd_log (LEVEL_NO_LOGGER, 0, "LIBPARODUS: already running at init\n");
		return EALREADY;
	}
	if (0 != run_state) {
		libpd_log (LEVEL_NO_LOGGER, 0, "LIBPARODUS: not idle at init\n");
		return EBUSY;
	}
	err = log_init (NULL, log_handler);
	if (err != 0) {
		libpd_log (LEVEL_NO_LOGGER, 0, "Failed to init logger\n");
		return -1;
	}
  //Call getParodusUrl to get parodus and client url
  getParodusUrl();
	make_closed_msg (&wrp_closed_msg);
	auth_received = false;
	selected_service = service_name;
	libpd_log (LEVEL_DEBUG, 0, "LIBPARODUS: connecting receiver\n");
	rcv_sock = connect_receiver (client_url);
	if (rcv_sock == -1) 
		return -1;
	libpd_log (LEVEL_DEBUG, 0, "LIBPARODUS: connecting sender\n");
	send_sock = connect_sender (parodus_url);
	if (send_sock == -1) {
		shutdown_socket(&rcv_sock);
		return -1;
	}
	stop_rcv_sock = connect_sender (client_url);
	if (stop_rcv_sock == -1) {
		shutdown_socket(&rcv_sock);
		shutdown_socket(&send_sock);
		return -1;
	}
	libpd_log (LEVEL_DEBUG, 0, "LIBPARODUS: Opened sockets\n");
	raw_queue = create_queue (raw_queue_name, 256);
	if (raw_queue == (mqd_t)-1) {
		shutdown_socket(&rcv_sock);
		shutdown_socket(&send_sock);
		shutdown_socket(&stop_rcv_sock);
		return -1;
	}
	wrp_queue = create_queue (wrp_queue_name, 24);
	if (wrp_queue == (mqd_t)-1) {
		shutdown_socket(&rcv_sock);
		mq_close (raw_queue);
		shutdown_socket(&send_sock);
		shutdown_socket(&stop_rcv_sock);
		return -1;
	}
	libpd_log (LEVEL_DEBUG, 0, "LIBPARODUS: Created queues\n");
	if (create_thread (&wrp_receiver_tid, wrp_receiver_thread) != 0) {
		shutdown_socket(&rcv_sock);
		mq_close (raw_queue);
		mq_close (wrp_queue);
		shutdown_socket(&send_sock);
		shutdown_socket(&stop_rcv_sock);
		return -1;
	}
	if (create_thread (&raw_receiver_tid, raw_receiver_thread) != 0) {
		shutdown_socket(&rcv_sock);
		pthread_cancel (wrp_receiver_tid);
		mq_close (raw_queue);
		mq_close (wrp_queue);
		shutdown_socket(&send_sock);
		shutdown_socket(&stop_rcv_sock);
		return -1;
	}
	
	run_state = RUN_STATE_RUNNING;

	libpd_log (LEVEL_DEBUG, 0, "LIBPARODUS: sending registration msg\n");
	if (send_registration_msg (selected_service) != 0) {
		libpd_log (LEVEL_ERROR, 0, "LIBPARODUS: error sending registration msg\n");
		libparodus_shutdown ();
		return -1;
	}
	libpd_log (LEVEL_DEBUG, 0, "LIBPARODUS: Sent registration message\n");
	return 0;
}

static int queue_send (mqd_t q, const char *qname, const char *msg, int len)
{
	int rtn;
	if (len < 0)
		len = strlen (msg) + 1;
	rtn = mq_send (q, msg, len, 0);
	if (rtn != 0)
		libpd_log (LEVEL_ERROR, errno, "Unable to send on queue %s\n", qname);
	return rtn; 
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

	if (RUN_STATE_RUNNING != run_state) {
		libpd_log (LEVEL_NO_LOGGER, 0, "LIBPARODUS: not running at shutdown\n");
		return -1;
	}

	run_state = RUN_STATE_DONE;
	libpd_log (LEVEL_DEBUG, 0, "LIBPARODUS: Shutting Down\n");
	sock_send (stop_rcv_sock, end_msg, -1);
 	rtn = pthread_join (raw_receiver_tid, NULL);
	if (rtn != 0)
		libpd_log (LEVEL_ERROR, rtn, "Error terminating raw receiver thread\n");
	shutdown_socket(&rcv_sock);
	libpd_log (LEVEL_DEBUG, 0, "LIBPARODUS: Flushing wrp queue\n");
	flush_wrp_queue (5);
	libpd_log (LEVEL_DEBUG, 0, "LIBPARODUS: Send end msg to raw queue\n");
	queue_send (raw_queue, "/RAW_QUEUE", end_msg, -1);
 	rtn = pthread_join (wrp_receiver_tid, NULL);
	if (rtn != 0)
		libpd_log (LEVEL_ERROR, rtn, "Error terminating wrp receiver thread\n");
	mq_close (raw_queue);
	mq_close (wrp_queue);
	shutdown_socket(&send_sock);
	shutdown_socket(&stop_rcv_sock);
	mq_unlink (raw_queue_name);
	mq_unlink (wrp_queue_name);
	run_state = 0;
	auth_received = false;
	log_shutdown ();
	return 0;
}

// msgbuf must be MAX_QUEUE_MSG_SIZE bytes long
static int raw_queue_receive (char *msgbuf, int *len)
{
	ssize_t bytes = mq_receive (raw_queue, msgbuf, MAX_QUEUE_MSG_SIZE, NULL);
	if (bytes < 0) {
		libpd_log (LEVEL_ERROR, errno, "Unable to receive on /RAW_QUEUE\n");
		return -1;
	}
	*len = (int) bytes;
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
	wrp_msg_t **wrp_msg_buf = (wrp_msg_t **) msgbuf; 

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
			"Invalid msg (len %d) (not a wrp_msg_t pointer) in wrp queue receive\n",
				bytes);
		return -2;
	}
	*msg = *wrp_msg_buf;
	//libpd_log (LEVEL_DEBUG, 0, "LIBPARODUS: receive msg on WRP QUEUE\n");
	return 0;
}

static void make_closed_msg (wrp_msg_t *msg)
{
	msg->msg_type = WRP_MSG_TYPE__REQ;
	msg->u.req.transaction_uuid = closed_msg;
	msg->u.req.source = closed_msg;
	msg->u.req.dest = closed_msg;
	msg->u.req.payload = (void*) closed_msg;
	msg->u.req.payload_size = strlen(closed_msg);
}

static bool is_closed_msg (wrp_msg_t *msg)
{
	return (msg->msg_type == WRP_MSG_TYPE__REQ) &&
		(strcmp (msg->u.req.dest, closed_msg) == 0);
}

// returns 0 OK
//  2 closed msg received
//  1 timed out
// -1 mq_receive error
// -2 msg size error, not a ptr
int libparodus_receive__ (wrp_msg_t **msg, uint32_t ms)
{
	struct timespec ts;
	int err;

	err = get_expire_time (ms, &ts);
	if (err != 0) {
		return err;
	}

	err = timed_wrp_queue_receive (msg, &ts);
	if (err != 0)
		return err;
	if (is_closed_msg (*msg)) {
		libpd_log (LEVEL_DEBUG, 0, "LIBPARODUS: closed msg received\n");
		return 2;
	}
	return 0;
}

// returns 0 OK
//  2 closed msg received
//  1 timed out
// -1 mq_receive error
// -2 msg size error, not a ptr
int libparodus_receive (wrp_msg_t **msg, uint32_t ms)
{
	if (RUN_STATE_RUNNING != run_state) {
		libpd_log (LEVEL_NO_LOGGER, 0, "LIBPARODUS: not running at receive\n");
		return -1;
	}
	return libparodus_receive__ (msg, ms);
}

int libparodus_close_receiver__ (void)
{
	queue_send (wrp_queue, "/WRP_QUEUE", (const char *) &closed_msg_ptr, 
		sizeof(wrp_msg_t *));
	libpd_log (LEVEL_DEBUG, 0, "LIBPARODUS: Sent closed msg\n");
	return 0;
}

int libparodus_close_receiver (void)
{
	if (RUN_STATE_RUNNING != run_state) {
		libpd_log (LEVEL_NO_LOGGER, 0, "LIBPARODUS: not running at close receiver\n");
		return -1;
	}
	return libparodus_close_receiver__ ();
}

static int wrp_sock_send (wrp_msg_t *msg)
{
	int rtn;
	ssize_t msg_len;
	void *msg_bytes;

	msg_len = wrp_struct_to (msg, WRP_BYTES, &msg_bytes);
	if (msg_len < 1) {
		libpd_log (LEVEL_ERROR, 0, "LIBPARODUS: error converting WRP to bytes\n");
		return -1;
	}

	rtn = sock_send (send_sock, (const char *)msg_bytes, msg_len);
	free (msg_bytes);
	return rtn;
}

int libparodus_send__ (wrp_msg_t *msg)
{
	if (!auth_received) {
		libpd_log (LEVEL_NO_LOGGER, 0, "LIBPARODUS: AUTH not received at send\n");
		return -1;
	}

	return wrp_sock_send (msg);
}

int libparodus_send (wrp_msg_t *msg)
{
	if (RUN_STATE_RUNNING != run_state) {
		libpd_log (LEVEL_NO_LOGGER, 0, "LIBPARODUS: not running at send\n");
		return -1;
	}
	return libparodus_send__ (msg);
}

static void *raw_receiver_thread (void *arg __attribute__ ((unused)) )
{
	int rtn;
	raw_msg_t msg;
	int end_msg_len = strlen(end_msg);

	libpd_log (LEVEL_DEBUG, 0, "Starting raw receiver thread\n");
	while (1) {
		rtn = sock_receive (&msg);
		if (rtn != 0)
			break;
		if (msg.len >= end_msg_len) {
			if (strncmp (msg.msg, end_msg, end_msg_len) == 0) {
				nn_freemsg (msg.msg);
				break;
			}
		}
		if (RUN_STATE_RUNNING != run_state) {
			nn_freemsg (msg.msg);
			continue;
		}
		libpd_log (LEVEL_DEBUG, 0, "LIBPARODUS: received raw msg from parodus service\n");
		queue_send (raw_queue, "/RAW_QUEUE", (const char *) &msg, sizeof(raw_msg_t));
	}
	libpd_log (LEVEL_DEBUG, 0, "Ended raw receiver thread\n");
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
	return NULL;
}

static void *wrp_receiver_thread (void *arg __attribute__ ((unused)) )
{
	int rtn, msg_len;
	raw_msg_t raw_msg;
	wrp_msg_t *wrp_msg;
	int end_msg_len = strlen(end_msg);
	char *msg_dest, *msg_service;
	char msg_buf[MAX_QUEUE_MSG_SIZE];

	libpd_log (LEVEL_DEBUG, 0, "Starting wrp receiver thread\n");
	while (1) {
		rtn = raw_queue_receive (msg_buf, &msg_len);
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
		libpd_log (LEVEL_DEBUG, 0, "LIBPARODUS: Converting bytes to WRP\n"); 
 		msg_len = (int) wrp_to_struct (raw_msg.msg, raw_msg.len, WRP_BYTES, &wrp_msg);
		nn_freemsg (raw_msg.msg);
		if (msg_len < 1) {
			libpd_log (LEVEL_ERROR, 0, "LIBPARODUS: error converting bytes to WRP\n");
			continue;
		}
		if (wrp_msg->msg_type == WRP_MSG_TYPE__AUTH) {
			if (auth_received)
				libpd_log (LEVEL_ERROR, 0, "LIBPARODUS: extra AUTH msg received\n");
			else
				libpd_log (LEVEL_DEBUG, 0, "LIBPARODUS: AUTH msg received\n");
			auth_received = true;
			wrp_free_struct (wrp_msg);
			continue;
		}
		if (!auth_received) {
			libpd_log (LEVEL_ERROR, 0, "LIBPARADOS: AUTH msg not received\n");
			wrp_free_struct (wrp_msg);
			continue;
		}

		// Pass thru REQ, EVENT, and CRUD if dest matches the selected service
		msg_dest = find_wrp_msg_dest (wrp_msg);
		if (NULL == msg_dest) {
			libpd_log (LEVEL_ERROR, 0, "LIBPARADOS: Unprocessed msg type %d received\n",
				wrp_msg->msg_type);
			wrp_free_struct (wrp_msg);
			continue;
		}
		msg_service = strrchr (msg_dest, '/');
		if (NULL == msg_service) {
			wrp_free_struct (wrp_msg);
			continue;
		}
		msg_service++;
		if (strcmp (msg_service, selected_service) != 0) {
			wrp_free_struct (wrp_msg);
			continue;
		}
		libpd_log (LEVEL_DEBUG, 0, "LIBPARODUS: received msg directed to service %s\n",
			selected_service);
		queue_send (wrp_queue, "/WRP_QUEUE", (const char *) &wrp_msg, 
			sizeof(wrp_msg_t *));
	}
	libpd_log (LEVEL_DEBUG, 0, "Ended wrp receiver thread\n");
	return NULL;
}

static int flush_wrp_queue (uint32_t delay_ms)
{
	wrp_msg_t *wrp_msg;
	struct timespec ts;
	int count = 0;
	int err = get_expire_time (delay_ms, &ts);
	if (err != 0) {
		return err;
	}

	while (1) {
		err = timed_wrp_queue_receive (&wrp_msg, &ts);
		if (err == 2) { // closed msg
			count++;
			continue;
		}
		if (err == 1)	// timed out
			break;
		if (err == -2) // bad msg
			continue;
		if (err != 0)
			return -1;
		wrp_free_struct (wrp_msg);
		count++;
	}
	libpd_log (LEVEL_DEBUG, 0, "LIBPARODUS: flushed %d messages out of WRP Queue\n", 
		count);
	return 0;
}

// Functions used by libpd_test.c

bool test_create_raw_queue (void)
{
	raw_queue = create_queue (raw_queue_name, 256);
	return (bool) (raw_queue != (mqd_t)-1);
}

bool test_create_wrp_queue (void)
{
	wrp_queue = create_queue (wrp_queue_name, 24);
	return (bool) (wrp_queue != (mqd_t)-1);
}

void test_close_raw_queue (void)
{
	mq_close (raw_queue);
}

void test_close_wrp_queue (void)
{
	mq_close (wrp_queue);
}

void test_send_raw_queue_ok (void)
{
	raw_msg_t msg;
	msg.msg = "Test message";
	msg.len = strlen(msg.msg);
	queue_send (raw_queue, "/RAW_QUEUE", (const char *) &msg, sizeof(raw_msg_t));
}

void test_send_raw_queue_error (void)
{
	queue_send (raw_queue, "/RAW_QUEUE", "***Invalid RAW message\n", -1);
}

int test_raw_queue_receive (void)
{
	int rtn;
	int len;
	char buf[MAX_QUEUE_MSG_SIZE];
	rtn = raw_queue_receive (buf, &len);
	if (rtn != 0)
		return -1;
	if (len == sizeof (raw_msg_t))
		return 0;
  return -2;
}

void test_send_wrp_queue_ok (void)
{
	wrp_msg_t reg_msg;
	reg_msg.msg_type = WRP_MSG_TYPE__SVC_REGISTRATION;
	reg_msg.u.reg.service_name = "iot";
	reg_msg.u.reg.url = CLIENT_URL;
	wrp_msg_t *msg = &reg_msg;
	queue_send (wrp_queue, "/WRP_QUEUE", (const char *) &msg, 
		sizeof(wrp_msg_t *));
}

void test_send_wrp_queue_error (void)
{
	queue_send (wrp_queue, "/WRP_QUEUE", "***Invalid WRP message\n", -1);
}

int test_close_receiver (void)
{
	make_closed_msg (&wrp_closed_msg);
	return libparodus_close_receiver__ ();
}



