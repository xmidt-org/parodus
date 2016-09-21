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
#include <CUnit/Basic.h>
#include <stdbool.h>

#include <assert.h>
#include <nanomsg/nn.h>
#include <nanomsg/bus.h>
#include "../src/wss_mgr.h"
#include <error.h>

#include <nanomsg/pipeline.h>
#include "wrp-c.h"

#define ENDPOINT "tcp://127.0.0.1:6666"


void test_nanomsg_socket_create()
{
	int sock = 0;
	printf("check for nanomsg_socket_create\n");
	sock = nn_socket (AF_SP, NN_PUSH);
	CU_ASSERT(sock >= 0);
	printf("Socket creation successful sock=%d \n", sock);
	nn_shutdown(sock, 0);

}

void test_nanomsg_socket_connect()
{
	int sock = 0;
	printf("check for nanomsg_socket_connect\n");
	sock = nn_socket (AF_SP, NN_PUSH);
	CU_ASSERT(nn_connect (sock, ENDPOINT) >= 0);
	printf("nanomsg socket connect successful\n");
    	sleep(2);
	nn_shutdown(sock, 0);

}

void test_nanomsg_socket_invalidURL()
{
	int sock = 0;
	printf("check for nanomsg_socket_invalidURL\n");
	sock = nn_socket (AF_SP, NN_PUSH);
	CU_ASSERT(nn_connect (sock, "tcp:xxxxxx:6667") < 0);
	printf("nanomsg socket invalid URL test passed\n");
    	sleep(2);
	nn_shutdown(sock, 0);

}

void test_nanomsg_client_send()
{
		    
	/*****Test svc registation for upstream - nanomsg client1 ****/
	printf("Checking for nanomsg client send\n");

	const wrp_msg_t reg = { .msg_type = WRP_MSG_TYPE__SVC_REGISTRATION,
	  .u.reg.service_name = "iot",
	  .u.reg.url = ENDPOINT};
	void *bytes;
	int size;
	int rv;
	wrp_msg_t *message;
	  
	// msgpack encode
	printf("msgpack encode\n");
	size = wrp_struct_to( &reg, WRP_BYTES, &bytes );

	printf("msgpack decode\n");
	// msgpck decode
	rv = wrp_to_struct(bytes, size, WRP_BYTES, &message);
	
	CU_ASSERT_EQUAL( rv, size );
	CU_ASSERT_EQUAL( message->msg_type, reg.msg_type );
	CU_ASSERT_STRING_EQUAL( message->u.reg.service_name, reg.u.reg.service_name );
	CU_ASSERT_STRING_EQUAL( message->u.reg.url, reg.u.reg.url );
	
	printf("decoded msgType:%d\n", message->msg_type);
	printf("decoded service_name:%s\n", message->u.reg.service_name);
	printf("decoded dest:%s\n", message->u.reg.url);

	wrp_free_struct(message);
	  
	//nanomsg socket 
	int sock = nn_socket (AF_SP, NN_PUSH);
	CU_ASSERT(nn_connect (sock, ENDPOINT) >= 0);
	sleep(5);


	int byte = nn_send (sock, bytes, size, NN_DONTWAIT);
	CU_ASSERT_EQUAL( byte, size );
	printf("nanomsg_client send msg success\n");
	free(bytes);
	sleep(2);
	nn_shutdown(sock, 0);

}

void test_nanomsg_send_receive()
{
	/***Test for nanomsg upstream send and downstream receive ***/
	
	printf("check for nanomsg_send_receive\n");

	const wrp_msg_t reg = { .msg_type = WRP_MSG_TYPE__SVC_REGISTRATION,
	  .u.reg.service_name = "iot",
	  .u.reg.url = "tcp://127.0.0.1:6660"};
	
          
	void *bytes;
	int size;
	int rv;
	wrp_msg_t *message;
	  
	/**********Nanomsg client sending msgs ******/
	
	printf("msgpack encode\n");
	size = wrp_struct_to( &reg, WRP_BYTES, &bytes );

	printf("msgpack decode\n");
	rv = wrp_to_struct(bytes, size, WRP_BYTES, &message);
	
	CU_ASSERT_EQUAL( rv, size );
	CU_ASSERT_EQUAL( message->msg_type, reg.msg_type );
	CU_ASSERT_STRING_EQUAL( message->u.reg.service_name, reg.u.reg.service_name );
	CU_ASSERT_STRING_EQUAL( message->u.reg.url, reg.u.reg.url );
	
	printf("decoded msgType:%d\n", message->msg_type);
	printf("decoded service_name:%s\n", message->u.reg.service_name);
	printf("decoded dest:%s\n", message->u.reg.url);
	wrp_free_struct(message);
	  
	
	int sock = nn_socket (AF_SP, NN_PUSH);
	CU_ASSERT(nn_connect (sock, reg.u.reg.url) >= 0);
	sleep(3);


	int byte = nn_send (sock, bytes, size, NN_DONTWAIT);
	sleep(3);
	printf("byte is:%d\n", byte);
	CU_ASSERT_EQUAL( byte, size );
	printf("nanomsg_client send msg success\n");
	sleep(3);
	
	/**********Nanomsg clients receiving msgs ******/
	
	printf("*****client receive****\n");
	int sock1 = nn_socket (AF_SP, NN_PULL);
	nn_bind (sock1, reg.u.reg.url);
	char *buf =NULL;
	wrp_msg_t *receive_message;
	
	int bit_size = nn_recv (sock1, &buf, NN_MSG, 0);
	CU_ASSERT(bit_size >= 0);
	CU_ASSERT_EQUAL( size, bit_size );
	
	printf ("Server Response \"%s\"\n", buf);
	//receive msg decode
	
	rv = wrp_to_struct(buf, bit_size, WRP_BYTES, &receive_message);
	printf("After decoding receive msg\n");
	
	CU_ASSERT_EQUAL( receive_message->msg_type, reg.msg_type );
	CU_ASSERT_STRING_EQUAL( receive_message->u.reg.service_name, reg.u.reg.service_name );
	CU_ASSERT_STRING_EQUAL( receive_message->u.reg.url, reg.u.reg.url );
	printf("nanomsg server received msg succssefully\n");
	wrp_free_struct(receive_message);
	nn_freemsg (buf);
	free(bytes);
	
	sleep(3);	
	nn_shutdown(sock, 0);

}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_nanomsg_socket_create);
    CU_add_test( *suite, "Test 2", test_nanomsg_socket_connect);
    CU_add_test( *suite, "Test 3", test_nanomsg_client_send );
    CU_add_test( *suite, "Test 4", test_nanomsg_send_receive );
    
    
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
