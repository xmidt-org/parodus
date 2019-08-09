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
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <CUnit/Basic.h>
#include <nopoll.h>
#include <nopoll_private.h>
#include <pthread.h>

#include "../src/ParodusInternal.h"
#include "../src/nopoll_handlers.h"
#include "../src/parodus_log.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
volatile unsigned int heartBeatTimer;
bool LastReasonStatus;
int closeReason = 0;
pthread_mutex_t close_mut;
bool close_retry;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

void set_global_reconnect_reason(char *reason)
{
    (void) reason;
}

bool get_global_reconnect_status()
{
    return LastReasonStatus;
}

bool get_interface_down_event()
{
     return false;
}

void set_global_reconnect_status(bool status)
{
    (void) status ;
}

const unsigned char *nopoll_msg_get_payload(noPollMsg *msg)
{
    if( NULL != msg ) {
        return (unsigned char *) "Dummy payload";
    }

    return NULL;
}

noPollOpCode nopoll_msg_opcode (noPollMsg * msg)
{
    if(NULL != msg)
    {
        return NOPOLL_PING_FRAME;
    }
    
    return NOPOLL_UNKNOWN_OP_CODE;
}

nopoll_bool nopoll_msg_is_fragment(noPollMsg *msg)
{
    (void) msg;
    return nopoll_false;
}

int nopoll_msg_get_payload_size(noPollMsg *msg)
{
    (void) msg;
    return 1;
}

int nopoll_conn_get_close_status (noPollConn * conn)
{
	(void) conn;
	if(closeReason)
		return 1006;
	else
		return 0;
}

int nopoll_conn_send_frame (noPollConn * conn, nopoll_bool fin, nopoll_bool masked,
                            noPollOpCode op_code, long length, noPollPtr content, long sleep_in_header)

{
    (void) conn; (void) fin; (void) masked; (void) op_code;
    (void) length; (void) content; (void) sleep_in_header; 
    
    return 0;
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void *a(void *in)
{
    noPollMsg msg;

    (void) in;
    listenerOnMessage_queue(NULL, NULL, &msg, NULL);
    pthread_exit(0);

    return NULL;
}

void *b(void *in)
{
    noPollMsg msg;

    (void) in;
    listenerOnMessage_queue(NULL, NULL, &msg, NULL);
    pthread_exit(0);

    return NULL;
}

void test_listenerOnMessage_queue()
{
    pthread_t thread_a, thread_b;

    pthread_create(&thread_a, NULL, a, NULL);
    pthread_create(&thread_b, NULL, b, NULL);

    pthread_join(thread_a, NULL);
    pthread_join(thread_b, NULL);

}

void *a1(void *in)
{
    char str[] = "SSL_Socket_Close";
    (void) in;

    set_global_reconnect_status(false);
    listenerOnCloseMessage(NULL, NULL, NULL);
    
    set_global_reconnect_status(false);
    listenerOnCloseMessage(NULL, NULL, (noPollPtr) str);

    set_global_reconnect_status(true);
    listenerOnCloseMessage(NULL, NULL, NULL);
    closeReason = 1;
    set_global_reconnect_status(true);
    listenerOnCloseMessage(NULL, NULL, (noPollPtr) str);

    pthread_exit(0);
    return NULL;
}

void test_listenerOnCloseMessage()
{
    pthread_t thread_a;

    pthread_create(&thread_a, NULL, a1, NULL);

    pthread_join(thread_a, NULL);
}

void test_listenerOnPingMessage()
{
    noPollConn c;
    noPollMsg  m;

    listenerOnPingMessage(NULL, &c, NULL, NULL);

    listenerOnPingMessage(NULL, &c, &m, NULL);

    listenerOnPingMessage(NULL, NULL, NULL, NULL);
}

void add_suites( CU_pSuite *suite )
{
    ParodusInfo("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_listenerOnMessage_queue );
    CU_add_test( *suite, "Test 2", test_listenerOnCloseMessage );
    CU_add_test( *suite, "Test 3", test_listenerOnPingMessage );
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
            ParodusPrint( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            ParodusPrint( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }

        CU_cleanup_registry();

    }

    return rv;
}
