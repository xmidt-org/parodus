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
#include <pthread.h>

#define MOCK_MSG_COUNT 10
#define MOCK_MSG_COUNT_STR "10"
#define TESTS_DIR_TAIL "/parodus/lib/tests"
#define BUILD_DIR_TAIL "/parodus/build"
#define BUILD_TESTS_DIR_TAIL "/parodus/build/lib/tests"
#define END_PIPE_MSG  "--END--\n"
#define SEND_EVENT_MSGS 1

static char current_dir_buf[256];

#define CURRENT_DIR_IS_BUILD	0
#define CURRENT_DIR_IS_TESTS	1
#define CURRENT_DIR_IS_BUILD_TESTS	2

static int current_dir_id = CURRENT_DIR_IS_BUILD;


#define RUN_TESTS_NAME(name) ( \
  (current_dir_id == CURRENT_DIR_IS_TESTS) ? "./" name : \
  (current_dir_id == CURRENT_DIR_IS_BUILD) ? "../lib/tests/" name : \
  "../../lib/tests/" name )   

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
	printf ("Removed pipe %s\n", end_pipe_name);
	err = mkfifo (end_pipe_name, 0666);
	if (err != 0) {
		libpd_log (LEVEL_NO_LOGGER, errno, "Error creating pipe %s\n", end_pipe_name);
		return -1;
	}
	printf ("Created fifo %s\n", end_pipe_name);
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
	printf ("Opened fifo %s\n", end_pipe_name);
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

int send_event_msgs (unsigned *msg_num, unsigned *event_num)
{
	int i;
	unsigned msg_num_mod;

#ifndef SEND_EVENT_MSGS
	return 0;
#endif
	(*msg_num)++;
	msg_num_mod = (*msg_num) % 3;
	if (msg_num_mod != 0)
		return 0;
	for (i=0; i<5; i++) {
		(*event_num)++;
		if (send_event_msg ("---LIBPARODUS---", "---ParodusService---",
			"---EventMessagePayload####", *event_num) != 0)
			return -1;
	}
	return 0;
}

int start_mock_parodus (void)
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

void test_1()
{
	unsigned msgs_received_count = 0;
	unsigned timeout_cnt = 0;
	int rtn;
	wrp_msg_t *wrp_msg;
	unsigned event_num = 0;
	unsigned msg_num = 0;
	char timestamp[20];


	rtn = make_current_timestamp (timestamp);
	if (rtn == 0)
		printf ("Current time is %s\n", timestamp);
	else
		printf ("make timestamp error %d\n", rtn);
	CU_ASSERT (rtn == 0);

	CU_ASSERT_FATAL (check_current_dir() == 0);

	if (using_mock) {
		rtn = create_end_pipe ();
		CU_ASSERT_FATAL (rtn == 0);
		if (rtn != 0)
			return;
		if (start_mock_parodus () == 0)
			return; // if in child process
		rtn = open_end_pipe ();
	}
	CU_ASSERT (libparodus_init (service_name, NULL) == 0);
	initEndKeypressHandler ();
	while (true) {
		rtn = libparodus_receive (&wrp_msg, 2000);
		if (rtn == 1) {
			printf ("Lipd test Timed out waiting for msg\n");
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
				if (send_event_msgs (&msg_num, &event_num) != 0)
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
		if (send_event_msgs (&msg_num, &event_num) != 0)
			break;
	}
	if (using_mock) {
#ifdef MOCK_MSG_COUNT
		bool close_pipe = (msgs_received_count < MOCK_MSG_COUNT);
#else
		bool close_pipe = true;
#endif
		if (close_pipe) {
			write_end_pipe (); 
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
