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

#include <libparodus.h>
#include <pthread.h>
#include "../src/ParodusInternal.h"

#define SEND_EVENT_MSGS 1

//#define TCP_URL(ip) "tcp://" ip

#define GOOD_CLIENT_URL "tcp://127.0.0.1:6667"
//#define PARODUS_URL "ipc:///tmp/parodus_server.ipc"
#define GOOD_PARODUS_URL "tcp://127.0.0.1:6666"
//#define CLIENT_URL "ipc:///tmp/parodus_client.ipc"


static void initEndKeypressHandler();
static void *endKeypressHandlerTask();

static pthread_t endKeypressThreadId;

static const char *service_name = "iot";
//static const char *service_name = "config";

static bool no_mock_send_only_test = false;

static libpd_instance_t test_instance;

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

extern const char *wrp_queue_name;
extern const char *parodus_url;
extern const char *client_url;
extern volatile int keep_alive_count;
extern volatile int reconnect_count;

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
	libparodus_send (test_instance, wrp_msg);
}

char *new_str (const char *str)
{
	char *buf = malloc (strlen(str) + 1);
	if (NULL == buf)
		return NULL;
	parStrncpy(buf, str, (strlen(str)+1));
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
	rtn = libparodus_send (test_instance, new_msg);
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

void dbg_log_err (const char *fmt, ...)
{
		char errbuf[100];

    va_list arg_ptr;
    va_start(arg_ptr, fmt);
    vprintf(fmt, arg_ptr);
    va_end(arg_ptr);
		printf ("LIBPD_TEST: %s\n", strerror_r (errno, errbuf, 100));
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

        libpd_cfg_t cfg = {.service_name = service_name,
                .receive = false, .keepalive_timeout_secs = 0};
        CU_ASSERT (libparodus_init (&test_instance, &cfg) == 0);
        CU_ASSERT (send_event_msgs (NULL, &event_num, 10) == 0);
	CU_ASSERT (libparodus_shutdown (&test_instance) == 0);
}

 void test_1(void)
{
	unsigned msgs_received_count = 0;
	int rtn;
	wrp_msg_t *wrp_msg;
	unsigned event_num = 0;
	unsigned msg_num = 0;
        libpd_cfg_t cfg = {.service_name = service_name,
                .receive = true, .keepalive_timeout_secs = 0};

	if (no_mock_send_only_test) {
		test_send_only ();
		return;
	}

        cfg.parodus_url = GOOD_PARODUS_URL;
        cfg.client_url = GOOD_CLIENT_URL;
        CU_ASSERT (libparodus_init (&test_instance, &cfg) == 0);
	printf ("LIBPD_TEST: libparodus_init successful\n");
	initEndKeypressHandler ();

	wait_auth_received ();

	printf ("LIBPD_TEST: starting msg receive loop\n");
	while (true) {
		rtn = libparodus_receive (test_instance, &wrp_msg, 2000);
		if (rtn == 1) {
			printf ("LIBPD_TEST: Timed out waiting for msg\n");
			if (msgs_received_count > 0)
				if (send_event_msgs (&msg_num, &event_num, 5) != 0)
					break;
			continue;
		}
		if (rtn != 0)
			break;
		show_wrp_msg (wrp_msg);
		msgs_received_count++;
		if (wrp_msg->msg_type == WRP_MSG_TYPE__REQ)
			send_reply (wrp_msg);
		wrp_free_struct (wrp_msg);
		if (send_event_msgs (&msg_num, &event_num, 5) != 0)
			break;
	}
	printf ("Messages received %u\n", msgs_received_count);
	CU_ASSERT (libparodus_shutdown (&test_instance) == 0);
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
		libpd_log (LEVEL_ERROR, "Error creating End Keypress Handler thread\n");
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
	libparodus_close_receiver (test_instance);
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

		if (argc > 1) {
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
