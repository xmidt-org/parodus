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
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <CUnit/Basic.h>
#include <stdbool.h>

#include "../src/libparodus.h"
#include <pthread.h>

static void initEndKeypressHandler();
static void *endKeypressHandlerTask();

static pthread_t endKeypressThreadId;

//static const char *service_name = "iot";
static const char *service_name = "config";

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

void test_1()
{
	int rtn;
	wrp_msg_t *wrp_msg;

	CU_ASSERT (libparodus_init (service_name, NULL) == 0);
	initEndKeypressHandler ();
	while (true) {
		rtn = libparodus_receive (&wrp_msg, 2000);
		if (rtn == 1) {
			printf ("Timed out waiting for msg\n");
			continue;
		}
		if (rtn != 0)
			break;
		show_wrp_msg (wrp_msg);
		if (wrp_msg->msg_type == WRP_MSG_TYPE__REQ)
			send_reply (wrp_msg);
		wrp_free_struct (wrp_msg);
	}
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
int main( void )
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;

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
