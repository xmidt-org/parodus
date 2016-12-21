/**
 *  Copyright 2010-2016 Comcast Cable Communications Management, LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <CUnit/Basic.h>
#include <stdbool.h>

#include "../src/libparodus.h"
#include "../src/libparodus_time.h"
#include "../src/libparodus_queues.h"
#include <pthread.h>

#define MOCK_MSG_COUNT 10
#define MOCK_MSG_COUNT_STR "10"
#define TESTS_DIR_TAIL "/parodus/lib/tests"
#define BUILD_DIR_TAIL "/parodus/build"
#define BUILD_TESTS_DIR_TAIL "/parodus/build/lib/tests"
#define END_PIPE_MSG  "--END--\n"
#define SEND_EVENT_MSGS 1

//#define TCP_URL(ip) "tcp://" ip

#define TEST_RCV_URL "tcp://127.0.0.1:6006"
#define BAD_RCV_URL  "tcp://127.0.0.1:X006"
#define BAD_CLIENT_URL "tcp://127.0.0.1:X006"
#define GOOD_CLIENT_URL "tcp://127.0.0.1:6667"
//#define PARODUS_URL "ipc:///tmp/parodus_server.ipc"
#define TEST_SEND_URL  "tcp://127.0.0.1:6007"
#define BAD_SEND_URL   "tcp://127.0.0.1:x007"
#define BAD_PARODUS_URL "tcp://127.0.0.1:x007"
#define GOOD_PARODUS_URL "tcp://127.0.0.1:6666"
//#define CLIENT_URL "ipc:///tmp/parodus_client.ipc"

static char current_dir_buf[256];

#define CURRENT_DIR_IS_BUILD	0
#define CURRENT_DIR_IS_TESTS	1
#define CURRENT_DIR_IS_BUILD_TESTS	2

static int current_dir_id = CURRENT_DIR_IS_BUILD;


#define RUN_TESTS_NAME(name) ( \
  (current_dir_id == CURRENT_DIR_IS_TESTS) ? "./" name : \
  (current_dir_id == CURRENT_DIR_IS_BUILD) ? "../lib/tests/" name : \
  "../../../lib/tests/" name )   

#define BUILD_TESTS_NAME(name) ( \
  (current_dir_id == CURRENT_DIR_IS_TESTS) ? "../../build/lib/tests/" name : \
  (current_dir_id == CURRENT_DIR_IS_BUILD) ? "./lib/tests/" name : \
  "./" name )


#define END_PIPE_NAME() RUN_TESTS_NAME("mock_parodus_end.txt")
#define MOCK_PARODUS_PATH() BUILD_TESTS_NAME("mock_parodus")
#define TEST_MSGS_PATH() RUN_TESTS_NAME("parodus_test_msgs.txt")

static int end_pipe_fd = -1;
static char *end_pipe_msg = END_PIPE_MSG;

static void initEndKeypressHandler();
static void *endKeypressHandlerTask();

static pthread_t endKeypressThreadId;

static const char *service_name = "iot";
//static const char *service_name = "config";

static bool using_mock = false;
static bool no_mock_send_only_test = false;

// libparodus functions to be tested
extern int flush_wrp_queue (uint32_t delay_ms);
extern int connect_receiver (const char *rcv_url);
extern int connect_sender (const char *send_url);
extern void shutdown_socket (int *sock);

extern bool is_auth_received (void);
extern int libparodus_receive__ (wrp_msg_t **msg, uint32_t ms);

// libparodus_log functions to be tested
extern int get_valid_file_num (const char *file_name, const char *date);
extern int get_last_file_num_in_dir (const char *date, const char *log_dir);

// test helper functions defined in libparodus
extern bool test_create_wrp_queue (void);
extern void test_close_wrp_queue (void);
extern int  test_close_receiver (void);
extern void test_send_wrp_queue_ok (void);


extern const char *wrp_queue_name;
extern const char *parodus_url;
extern const char *client_url;


int check_current_dir (void)
{
	int current_dir_len, tail_len;
	char *pos, *end_pos;
	char *current_dir = getcwd (current_dir_buf, 256);

	if (NULL == current_dir) {
		libpd_log (LEVEL_NO_LOGGER, errno, "Unable to get current directory\n");
		return -1;
	}

	printf ("Current dir in libpd test is %s\n", current_dir);

	current_dir_len = strlen (current_dir_buf);
	end_pos = current_dir + current_dir_len;

	tail_len = strlen (TESTS_DIR_TAIL);
	pos = end_pos - tail_len;
	if (strcmp (pos, TESTS_DIR_TAIL) == 0) {
		current_dir_id = CURRENT_DIR_IS_TESTS;
		return 0;
	}

	tail_len = strlen (BUILD_DIR_TAIL);
	pos = end_pos - tail_len;
	if (strcmp (pos, BUILD_DIR_TAIL) == 0) {
		current_dir_id = CURRENT_DIR_IS_BUILD;
		return 0;
	}

	tail_len = strlen (BUILD_TESTS_DIR_TAIL);
	pos = end_pos - tail_len;
	if (strcmp (pos, BUILD_TESTS_DIR_TAIL) == 0) {
		current_dir_id = CURRENT_DIR_IS_BUILD_TESTS;
		return 0;
	}

	libpd_log (LEVEL_NO_LOGGER, 0, "Not in parodus lib tests or build directory\n");
	return -1;
}

static int create_end_pipe (void)
{
	const char *end_pipe_name = END_PIPE_NAME();
	int err = remove (end_pipe_name);
	if ((err != 0) && (errno != ENOENT)) {
		libpd_log (LEVEL_NO_LOGGER, errno, "Error removing pipe %s\n", end_pipe_name);
		return -1;
	}
	printf ("LIBPD TEST: Removed pipe %s\n", end_pipe_name);
	err = mkfifo (end_pipe_name, 0666);
	if (err != 0) {
		libpd_log (LEVEL_NO_LOGGER, errno, "Error creating pipe %s\n", end_pipe_name);
		return -1;
	}
	printf ("LIBPD_TEST: Created fifo %s\n", end_pipe_name);
	return 0;
}

static int open_end_pipe (void)
{
	const char *end_pipe_name = END_PIPE_NAME();
	end_pipe_fd = open (end_pipe_name, O_WRONLY, 0222);
	if (end_pipe_fd == -1) {
		libpd_log (LEVEL_NO_LOGGER, errno, "Error opening pipe %s\n", end_pipe_name);
		return -1;
	}
	printf ("LIBPD_TEST: Opened fifo %s\n", end_pipe_name);
	return 0;
}

static int write_end_pipe (void)
{
	const char *end_pipe_name = END_PIPE_NAME();
	int rtn, fd_flags;
	fd_flags = fcntl (end_pipe_fd, F_GETFL);
	if (fd_flags < 0) {
		libpd_log (LEVEL_ERROR, errno, "Error retrieving pipe %s flags\n", end_pipe_name);
		return -1;
	}
	rtn = fcntl (end_pipe_fd, F_SETFL, fd_flags | O_NONBLOCK);
	if (rtn < 0) {
		libpd_log (LEVEL_ERROR, errno, "Error settinging pipe %s flags\n", end_pipe_name);
		return -1;
	}
	rtn = write (end_pipe_fd, end_pipe_msg, strlen (end_pipe_msg));
	if (rtn <= 0) {
		libpd_log (LEVEL_ERROR, errno, "Error writing pipe %s\n", end_pipe_name);
		return -1;
	}
	return 0;
}

void show_src_dest_payload (char *src, char *dest, void *payload, size_t payload_size)
{
	size_t i;
	char *payload_str = (char *) payload;
	printf (" SOURCE:  %s\n", src);
	printf (" DEST  :  %s\n", dest);
	printf (" PAYLOAD: ");
	for (i=0; i<payload_size; i++)
		putchar (payload_str[i]);
	putchar ('\n');
}

void show_wrp_req_msg (struct wrp_req_msg *msg)
{
	show_src_dest_payload (msg->source, msg->dest, msg->payload, msg->payload_size);
}

void show_wrp_event_msg (struct wrp_event_msg *msg)
{
	show_src_dest_payload (msg->source, msg->dest, msg->payload, msg->payload_size);
}

void show_wrp_msg (wrp_msg_t *wrp_msg)
{
	printf ("Received WRP Msg type %d\n", wrp_msg->msg_type);
	if (wrp_msg->msg_type == WRP_MSG_TYPE__REQ) {
		show_wrp_req_msg (&wrp_msg->u.req);
		return;
	}
	if (wrp_msg->msg_type == WRP_MSG_TYPE__EVENT) {
		show_wrp_event_msg (&wrp_msg->u.event);
		return;
	}
	return;
}

void send_reply (wrp_msg_t *wrp_msg)
{
	size_t i;
	size_t payload_size = wrp_msg->u.req.payload_size;
	char *payload = (char *) wrp_msg->u.req.payload;
	char *temp;
	// swap source and dest
	temp = wrp_msg->u.req.source;
	wrp_msg->u.req.source = wrp_msg->u.req.dest;
	wrp_msg->u.req.dest = temp;
	// Alter the payload
	for (i=0; i<payload_size; i++)
		payload[i] = tolower (payload[i]);
	libparodus_send (wrp_msg);
}

char *new_str (const char *str)
{
	char *buf = malloc (strlen(str) + 1);
	if (NULL == buf)
		return NULL;
	strcpy (buf, str);
	return buf;
}

void insert_number_into_buf (char *buf, unsigned num)
{
	char *pos = strrchr (buf, '#');
	if (NULL == pos)
		return;
	while (true) {
		*pos = (num%10) + '0';
		num /= 10;
		if (pos <= buf)
			break;
		pos--;
		if (*pos != '#')
			break;
	}
}

int send_event_msg (const char *src, const char *dest, 
	const char *payload, unsigned event_num)
{
	int rtn = 0;
	char *payload_buf;
	wrp_msg_t *new_msg;

#ifndef SEND_EVENT_MSGS
	return 0;
#endif
	new_msg = malloc (sizeof (wrp_msg_t));
	if (NULL == new_msg)
		return -1;
	printf ("Making event msg\n");
	new_msg->msg_type = WRP_MSG_TYPE__EVENT;
	new_msg->u.event.source = new_str (src);
	new_msg->u.event.dest = new_str (dest);
	new_msg->u.event.headers = NULL;
	new_msg->u.event.metadata = NULL;
	payload_buf = new_str (payload);
	insert_number_into_buf (payload_buf, event_num);
	new_msg->u.event.payload = (void*) payload_buf;
	new_msg->u.event.payload_size = strlen (payload) + 1;
	printf ("Sending event msg %u\n", event_num);
	rtn = libparodus_send (new_msg);
	//printf ("Freeing event msg\n");
	wrp_free_struct (new_msg);
	//printf ("Freed event msg\n");
	return rtn;
}

int send_event_msgs (unsigned *msg_num, unsigned *event_num, int count)
{
	int i;
	unsigned msg_num_mod;

#ifndef SEND_EVENT_MSGS
	return 0;
#endif
	if (NULL != msg_num) {
		(*msg_num)++;
		msg_num_mod = (*msg_num) % 3;
		if (msg_num_mod != 0)
			return 0;
	}
	for (i=0; i<count; i++) {
		(*event_num)++;
		if (send_event_msg ("---LIBPARODUS---", "---ParodusService---",
			"---EventMessagePayload####", *event_num) != 0)
			return -1;
	}
	return 0;
}

int start_mock_parodus ()
{
	int pid;
	const char *mock_parodus_path = MOCK_PARODUS_PATH();
	const char *test_msgs_path = TEST_MSGS_PATH();

	pid = fork ();
	if (pid == -1) {
		libpd_log (LEVEL_NO_LOGGER, errno, "Fork failed\n");
		CU_ASSERT_FATAL (-1 != pid);
	}
	if (pid == 0) {
		// child
		int err = execlp (mock_parodus_path, mock_parodus_path, 
			"-f", test_msgs_path, "-d", "3",
#ifdef MOCK_MSG_COUNT
			"-c", MOCK_MSG_COUNT_STR,
#endif
			(char*)NULL);
		if (err != 0)
			libpd_log (LEVEL_NO_LOGGER, errno, "Failed execlp of mock_parodus\n");
		printf ("Child finished\n");
	}
	return pid;	
}

void test_time (void)
{
	int rtn;
	char timestamp[20];
	struct timespec ts1;
	struct timespec ts2;
	bool ts2_greater;

	rtn = make_current_timestamp (timestamp);
	if (rtn == 0)
		printf ("LIBPD_TEST: Current time is %s\n", timestamp);
	else
		printf ("LIBPD_TEST: make timestamp error %d\n", rtn);
	CU_ASSERT (rtn == 0);
	rtn = get_expire_time (500, &ts1);
	CU_ASSERT (rtn == 0);
	rtn = get_expire_time (500, &ts2);
	CU_ASSERT (rtn == 0);
	printf ("ts1: (%u, %u), ts2: (%u, %u)\n", 
		(unsigned)ts1.tv_sec, (unsigned)ts1.tv_nsec,
		(unsigned)ts2.tv_sec, (unsigned)ts2.tv_nsec);
	if (ts2.tv_sec != ts1.tv_sec)
		ts2_greater = (bool) (ts2.tv_sec > ts1.tv_sec);
	else
		ts2_greater = (bool) (ts2.tv_nsec >= ts1.tv_nsec);
	CU_ASSERT (ts2_greater);
	rtn = get_expire_time (5000, &ts1);
	CU_ASSERT (rtn == 0);
	rtn = get_expire_time (500, &ts2);
	CU_ASSERT (rtn == 0);
	if (ts2.tv_sec != ts1.tv_sec)
		ts2_greater = (bool) (ts2.tv_sec > ts1.tv_sec);
	else
		ts2_greater = (bool) (ts2.tv_nsec >= ts1.tv_nsec);
	CU_ASSERT (!ts2_greater);
}

void test_queue_send_msg (libpd_mq_t q, unsigned timeout_ms, int n)
{
	void *msg;
	int msgsize;
	char msgbuf[100];

	sprintf (msgbuf, "Test Message # %d\n", n);
	msgsize = strlen(msgbuf) + 1;
	msg = malloc (msgsize);
	CU_ASSERT_FATAL (msg != NULL);
	strncpy ((char*)msg, msgbuf, msgsize);
	CU_ASSERT (libpd_qsend (q, msg, timeout_ms) == 0);
}

int get_msg_num (const char *msg)
{
	int num = -1;
	bool found_pound = false;
	int i;
	char c;

	for (i=0; (c=msg[i]) != 0; i++)
	{
		if (!found_pound) {
			if (c == '#')
				found_pound = true;
			continue;
		}
		if ((c>='0') && (c<='9')) {
			if (num == -1)
				num = c - '0';
			else
				num = 10*num + (c - '0');
		}
	}
	return num;
}

void test_queue_rcv_msg (libpd_mq_t q, unsigned timeout_ms, int n)
{
	int err, msg_num;
	void *msg;
	err = libpd_qreceive (q, &msg, timeout_ms);
	CU_ASSERT (err == 0);
	if (err != 0)
		return;
	fputs ((char*) msg, stdout);
	if (n < 0) {
		free (msg);
		return;
	}
	msg_num = get_msg_num ((char*)msg);
	free (msg);
	CU_ASSERT (msg_num >= 0);
	if (msg_num < 0)
		return;
	CU_ASSERT (msg_num == n);
}

static int flush_queue_count = 0;

void qfree (void * msg)
{
	flush_queue_count++;
	free (msg);
}

void delay_ms(unsigned int secs, unsigned int msecs)
{
  struct timespec ts;
  ts.tv_sec = (time_t) secs;
  ts.tv_nsec = (long) msecs * 1000000L;
  nanosleep (&ts, NULL);
}

typedef struct {
	libpd_mq_t queue;
	unsigned initial_wait_ms;
	unsigned num_msgs;
	unsigned send_interval_ms;
} test_queue_info_t;

static void *test_queue_sender_thread (void *arg)
{
	test_queue_info_t *qinfo = (test_queue_info_t *) arg;
	unsigned i;

	printf ("LIBPD_TEST: started test_queue_sender_thread\n");
	if (qinfo->initial_wait_ms != 0)
		delay_ms (0, qinfo->initial_wait_ms);

	for (i=0; i<qinfo->num_msgs; i++)
	{
		test_queue_send_msg (qinfo->queue, qinfo->send_interval_ms, i);
	}
	printf ("LIBPD_TEST: ended test_queue_sender_thread\n");
	return NULL;
}

void test_queues (void)
{
	test_queue_info_t qinfo;
	int i, rtn;
	void *msg;
	pthread_t sender_test_tid;

	qinfo.initial_wait_ms = 2000;
	qinfo.num_msgs = 5;
	qinfo.send_interval_ms = 500;

	CU_ASSERT (libpd_qcreate (&qinfo.queue, "//TEST_QUEUE", 5) == 0);
	for (i=0; i< 5; i++)
		test_queue_send_msg (qinfo.queue, qinfo.send_interval_ms, i);
	CU_ASSERT (libpd_qsend (qinfo.queue, "extra message", qinfo.send_interval_ms)
		 == ETIMEDOUT);
	for (i=0; i< 5; i++)
		test_queue_rcv_msg (qinfo.queue, qinfo.send_interval_ms, i);
	CU_ASSERT (libpd_qreceive (qinfo.queue, &msg, qinfo.send_interval_ms)
		 == ETIMEDOUT);
	for (i=0; i< 5; i++)
	{
		test_queue_send_msg (qinfo.queue, qinfo.send_interval_ms, i);
		test_queue_rcv_msg (qinfo.queue, qinfo.send_interval_ms, i);
	}
	flush_queue_count = 0;
	CU_ASSERT (libpd_qdestroy (&qinfo.queue, &qfree) == 0);
	CU_ASSERT (flush_queue_count == 0);

	CU_ASSERT (libpd_qcreate (&qinfo.queue, "//TEST_QUEUE", 5) == 0);
	for (i=0; i< 5; i++)
		test_queue_send_msg (qinfo.queue, qinfo.send_interval_ms, i);
	flush_queue_count = 0;
	CU_ASSERT (libpd_qdestroy (&qinfo.queue, &qfree) == 0);
	CU_ASSERT (flush_queue_count == 5);

	CU_ASSERT (libpd_qcreate (&qinfo.queue, "//TEST_QUEUE", qinfo.num_msgs) == 0);
	rtn = pthread_create 
		(&sender_test_tid, NULL, test_queue_sender_thread, (void*) &qinfo);
	CU_ASSERT (rtn == 0);
	if (rtn == 0) {
		for (i=0; i< (int)qinfo.num_msgs; i++)
			test_queue_rcv_msg (qinfo.queue, 4000, -1);
		pthread_join (sender_test_tid, NULL);
	}
	flush_queue_count = 0;
	CU_ASSERT (libpd_qdestroy (&qinfo.queue, &qfree) == 0);
	CU_ASSERT (flush_queue_count == 0);

	CU_ASSERT (libpd_qcreate (&qinfo.queue, "//TEST_QUEUE", qinfo.num_msgs) == 0);
	qinfo.initial_wait_ms = 0;
	qinfo.num_msgs += 5;
	qinfo.send_interval_ms = 5000;
	rtn = pthread_create 
		(&sender_test_tid, NULL, test_queue_sender_thread, (void*) &qinfo);
	CU_ASSERT (rtn == 0);
	if (rtn == 0) {
		delay_ms (0, 2000);
		for (i=0; i< (int)qinfo.num_msgs; i++) {
			test_queue_rcv_msg (qinfo.queue, 4000, -1);
			delay_ms (0, 500);
		}
		pthread_join (sender_test_tid, NULL);
	}
	flush_queue_count = 0;
	CU_ASSERT (libpd_qdestroy (&qinfo.queue, &qfree) == 0);
	CU_ASSERT (flush_queue_count == 0);
}

void dbg_log_err (const char *fmt, ...)
{
		char errbuf[100];

    va_list arg_ptr;
    va_start(arg_ptr, fmt);
    vprintf(fmt, arg_ptr);
    va_end(arg_ptr);
		printf ("LIBPD_TEST: %s\n", strerror_r (errno, errbuf, 100));
}

int make_test_log_file (const char *date, int file_num)
{
	int current_fd, rtn;
	int flags = O_WRONLY | O_CREAT | O_TRUNC | O_SYNC;
	const char *test_line = "Log Test Line\n";
	char name_buf[64];

	sprintf (name_buf, "log%s.%d", date, file_num);
	current_fd = open (name_buf, flags, 0666);
	if (current_fd == -1) {
		dbg_log_err ("Unable to open test log file %s\n", name_buf);
		return -1;
	}
	rtn = write (current_fd, test_line, strlen(test_line));
	close (current_fd);
	if (rtn > 0)
		return 0;
	dbg_log_err ("Unable to write test log file %s\n", name_buf);
	return -1;
}

void test_log (void)
{
	CU_ASSERT (get_valid_file_num ("log20161102.4", "20161102") == 4);
	CU_ASSERT (get_valid_file_num ("lg20161102.4", "20161102") == -1);
	CU_ASSERT (get_valid_file_num ("log20161103.4", "20161102") == -1);
	CU_ASSERT (get_valid_file_num ("log20161102.x", "20161102") == -1);
	CU_ASSERT (get_valid_file_num ("log20161102,4", "20161102") == -1);
	CU_ASSERT (make_test_log_file ("20161102", 4) == 0);
	CU_ASSERT (get_last_file_num_in_dir ("20161102", ".") == 4);
}

void wait_auth_received (void)
{
	if (!is_auth_received ()) {
		printf ("Waiting for auth received\n");
		sleep(1);
	}
	if (!is_auth_received ()) {
		printf ("Waiting for auth received\n");
		sleep(1);
	}
	CU_ASSERT (is_auth_received ());
}

void test_send_only (void)
{
	unsigned event_num = 0;

	CU_ASSERT (libparodus_init_ext (service_name, NULL, "C") == 0);
	CU_ASSERT (send_event_msgs (NULL, &event_num, 10) == 0);
	CU_ASSERT (libparodus_shutdown () == 0);
}

void test_1(void)
{
	unsigned msgs_received_count = 0;
	unsigned timeout_cnt = 0;
	int rtn;
	int test_sock;
	wrp_msg_t *wrp_msg;
	unsigned event_num = 0;
	unsigned msg_num = 0;
	const char *parodus_url_orig = GOOD_PARODUS_URL;
	const char *client_url_orig = GOOD_CLIENT_URL;

	test_time ();
	test_log ();
	CU_ASSERT_FATAL (check_current_dir() == 0);

	CU_ASSERT_FATAL (log_init (".", NULL) == 0);
	test_queues ();

	printf ("LIBPD_TEST: test connect receiver, good IP\n");
	test_sock = connect_receiver (TEST_RCV_URL);
	CU_ASSERT (test_sock != -1) ;
	if (test_sock != -1)
		shutdown_socket(&test_sock);
	printf ("LIBPD_TEST: test connect receiver, bad IP\n");
	test_sock = connect_receiver (BAD_RCV_URL);
	CU_ASSERT (test_sock == -1);
	printf ("LIBPD_TEST: test connect receiver, good IP\n");
	test_sock = connect_receiver (TEST_RCV_URL);
	CU_ASSERT (test_sock != -1) ;
	if (test_sock != -1)
		shutdown_socket(&test_sock);
	printf ("LIBPD_TEST: test connect sender, good IP\n");
	test_sock = connect_sender (TEST_SEND_URL);
	CU_ASSERT (test_sock != -1) ;
	if (test_sock != -1)
		shutdown_socket(&test_sock);
	printf ("LIBPD_TEST: test connect sender, bad IP\n");
	test_sock = connect_sender (BAD_SEND_URL);
	CU_ASSERT (test_sock == -1);

	printf ("LIBPD_TEST: test create wrp queue\n");
	CU_ASSERT (test_create_wrp_queue () == 0);
	printf ("LIBPD_TEST: test libparodus receive good\n");
	test_send_wrp_queue_ok ();
	CU_ASSERT (libparodus_receive__ (&wrp_msg, 500) == 0);
	wrp_free_struct (wrp_msg);
	printf ("LIBPD_TEST: test wrp_flush_queue\n");
	test_send_wrp_queue_ok ();
	test_send_wrp_queue_ok ();
	test_send_wrp_queue_ok ();
	CU_ASSERT (flush_wrp_queue (500) == 3);
	CU_ASSERT (flush_wrp_queue (500) == 0);
	printf ("LIBPD_TEST: test wrp_flush_queue with close msg\n");
	test_send_wrp_queue_ok ();
	test_send_wrp_queue_ok ();
	test_close_receiver ();
	CU_ASSERT (flush_wrp_queue (500) == 3);

	printf ("LIBPD_TEST: test libparodus receive timeout\n");
	CU_ASSERT (libparodus_receive__ (&wrp_msg, 500) == 1);
	CU_ASSERT (test_close_receiver() == 0);
	printf ("LIBPD_TEST: test libparodus receive close msg\n");
	rtn = libparodus_receive__ (&wrp_msg, 500);
	if (rtn != 2) {
		printf ("LIBPD_TEST: expected receive rtn==2 after close, got %d\n", rtn);
	}
	CU_ASSERT (rtn == 2);

	test_close_wrp_queue ();

	CU_ASSERT (libparodus_receive (&wrp_msg, 500) == -1);

	printf ("LIBPD_TEST: libparodus_init bad parodus ip\n");
	CU_ASSERT (setenv( "PARODUS_SERVICE_URL", BAD_PARODUS_URL, 1) == 0);
	CU_ASSERT (libparodus_init (service_name, NULL) != 0);
	CU_ASSERT (setenv( "PARODUS_SERVICE_URL", parodus_url_orig, 1) == 0);
	CU_ASSERT (setenv( "PARODUS_CLIENT_URL", BAD_CLIENT_URL, 1) == 0);
	printf ("LIBPD_TEST: libparodus_init bad client url\n");
	CU_ASSERT (libparodus_init (service_name, NULL) != 0);
	CU_ASSERT (setenv( "PARODUS_CLIENT_URL", client_url_orig, 1) == 0);
	printf ("LIBPD_TEST: libparodus_init bad options\n");
	CU_ASSERT (libparodus_init_ext (service_name, NULL, "X") == EINVAL);

	log_shutdown ();

	if (no_mock_send_only_test) {
		test_send_only ();
		return;
	}

	if (using_mock) {
		rtn = create_end_pipe ();
		CU_ASSERT_FATAL (rtn == 0);
		if (rtn != 0)
			return;
		if (start_mock_parodus () == 0)
			return; // if in child process
		printf ("LIBPD mock_parodus started\n");
		rtn = open_end_pipe ();
	}

	printf ("LIBPD_TEST: no receive option\n");
	CU_ASSERT (libparodus_init_ext (service_name, NULL, "") == 0);
	CU_ASSERT (send_event_msgs (NULL, &event_num, 5) == 0);
	CU_ASSERT (libparodus_receive (&wrp_msg, 500) == -3);
	CU_ASSERT (libparodus_shutdown () == 0);

	CU_ASSERT (libparodus_init (service_name, NULL) == 0);
	printf ("LIBPD_TEST: libparodus_init successful\n");
	initEndKeypressHandler ();

	wait_auth_received ();
	if (is_auth_received()) {
		printf ("LIBPD_TEST: Test invalid wrp message\n");
		wrp_msg = (wrp_msg_t *) "*** Invalid WRP message\n";
		CU_ASSERT (libparodus_send (wrp_msg) != 0);
	}

	printf ("LIBPD_TEST: starting msg receive loop\n");
	while (true) {
		rtn = libparodus_receive (&wrp_msg, 2000);
		if (rtn == 1) {
			printf ("LIBPD_TEST: Timed out waiting for msg\n");
#ifdef MOCK_MSG_COUNT
			if (using_mock) {
				timeout_cnt++;
				if (timeout_cnt >= 6) {
					libparodus_close_receiver ();
					continue;
				}
			}
#endif
			if (msgs_received_count > 0)
				if (send_event_msgs (&msg_num, &event_num, 5) != 0)
					break;
			continue;
		}
		if (rtn != 0)
			break;
		show_wrp_msg (wrp_msg);
		timeout_cnt = 0;
		msgs_received_count++;
		if (wrp_msg->msg_type == WRP_MSG_TYPE__REQ)
			send_reply (wrp_msg);
		wrp_free_struct (wrp_msg);
		if (send_event_msgs (&msg_num, &event_num, 5) != 0)
			break;
	}
	if (using_mock) {
#ifdef MOCK_MSG_COUNT
		bool close_pipe = (msgs_received_count < MOCK_MSG_COUNT);
#else
		bool close_pipe = true;
#endif
		if (close_pipe) {
			printf ("LIBPD writing end pipe\n");
			write_end_pipe (); 
			printf ("LIBPD closing end pipe\n");
			close (end_pipe_fd);
			wait (NULL);
		}
	}
	printf ("Messages received %u\n", msgs_received_count);
	CU_ASSERT (libparodus_shutdown () == 0);
}

/*
 * @brief To initiate end keypress handler
 */
static void initEndKeypressHandler()
{
	int err = 0;
	err = pthread_create(&endKeypressThreadId, NULL, endKeypressHandlerTask, NULL);
	if (err != 0) 
	{
		libpd_log (LEVEL_ERROR, err, "Error creating End Keypress Handler thread\n");
	}
	else 
	{
		printf ("End Keypress handler Thread created successfully\n");
		printf ("\n--->> Press <Enter> to shutdown the test. ---\n");
	}
}

/*
 * @brief To handle End Keypress
 */
static void *endKeypressHandlerTask()
{
	char inbuf[10];
	memset(inbuf, 0, 10);
	while (true) {
		fgets (inbuf, 10, stdin);
		if ((inbuf[0] != '\n') && (inbuf[0] != '\0')) {
            printf ("endKeyPressHandler exiting\n");
			break;
		}
	}
	libparodus_close_receiver ();
	return NULL;
}


void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "libparodus tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_1 );
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( int argc, char **argv __attribute__((unused)) )
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;

		if (argc <= 1)
			using_mock = true;
		else {
			const char *arg = argv[1];
			if ((arg[0] == 's') || (arg[0] == 'S'))
				no_mock_send_only_test = true;
		}

    if( CUE_SUCCESS == CU_initialize_registry() ) {
        add_suites( &suite );

        if( NULL != suite ) {
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            printf( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            printf( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }

        CU_cleanup_registry();
    }

    if( 0 != rv ) {
        return 1;
    }
    return 0;
}
