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
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>

#include <CUnit/Basic.h>
#include "../src/ParodusInternal.h"
#include "../src/spin_thread.h"
#include "../src/client_list.h"

#define TEST_CLIENT1_URL "tcp://127.0.0.1:6677"
#define TEST_CLIENT2_URL "tcp://127.0.0.1:6655"

static void *client_rcv_task();
static void *client2_rcv_task();

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/


void test_client_addtolist()
{
	void *bytes;
	int size =0;
	int rv;
	wrp_msg_t  *message;
	int status = -1;
	reg_list_item_t *temp =NULL;
	
	const wrp_msg_t reg = { .msg_type = WRP_MSG_TYPE__SVC_REGISTRATION,
	  .u.reg.service_name = "test_client1",
	  .u.reg.url = TEST_CLIENT1_URL};
	
	/*** msgpack encode **/
	ParodusPrint("msgpack encode\n");
	size = wrp_struct_to( &reg, WRP_BYTES, &bytes );
	
	/*** decode **/
	rv = wrp_to_struct(bytes, size, WRP_BYTES, &message);
	ParodusPrint("rv is %d\n", rv);
	ParodusPrint("decoded msgType:%d\n", message->msg_type);
	ParodusPrint("decoded service_name:%s\n", message->u.reg.service_name);
	ParodusPrint("decoded dest:%s\n", message->u.reg.url);
	
	StartThread(client_rcv_task, NULL);
	
	status = addToList(&message);
	ParodusPrint("addToList status is %d\n", status);

	
	CU_ASSERT_EQUAL( status, 0 );
	temp = get_global_node();
				
	if (NULL != temp)
	{
		ParodusPrint("node is pointing to service_name %s \n",temp->service_name);
		CU_ASSERT_STRING_EQUAL( temp->service_name, message->u.reg.service_name );
		CU_ASSERT_STRING_EQUAL( temp->url, message->u.reg.url );
	}

	wrp_free_struct(message);
	free(bytes);
	ParodusInfo("test_client_addtolist done..\n");	
	
}

static void *client_rcv_task()
{
	//nanomsg socket
	
	int byte =0;
	int rv1=0;
	int t=25000;
	int rc = -1;
	int bind;
	wrp_msg_t  *msg1;

	int sock1 = nn_socket (AF_SP, NN_PULL);
	bind = nn_bind(sock1, TEST_CLIENT1_URL);
	if(bind < 0)
        {
            ParodusError("Unable to bind socket (errno=%d, %s)\n",errno, strerror(errno));
        }
	
	void *buf = NULL;
	rc = nn_setsockopt(sock1, NN_SOL_SOCKET, NN_RCVTIMEO, &t, sizeof(t));
	if(rc < 0)
        {
        	ParodusError ("Unable to set socket timeout (errno=%d, %s)\n",errno, strerror(errno));
        }
	
	ParodusPrint("Client 1 waiting for acknowledgement \n");
	byte = nn_recv(sock1, &buf, NN_MSG, 0);
	ParodusInfo("Data Received for client 1 : %s \n", (char * )buf);

	rv1 = wrp_to_struct((void *)buf, byte, WRP_BYTES, &msg1);
	ParodusPrint("rv1 %d \n", rv1);

	ParodusPrint("msg1->msg_type for client 1 = %d\n", msg1->msg_type);
	ParodusPrint("msg1->status for client 1 = %d\n", msg1->u.auth.status);

	nn_freemsg(buf);	

	wrp_free_struct(msg1);
	nn_shutdown(sock1, 0);
	return 0;	
}

static void *client2_rcv_task()
{
	//nanomsg socket
	
	int byte =0;
	int rv1=0;
	int t=25000;
	int rc = -1;
	int bind;
	wrp_msg_t  *msg1;

	int sock1 = nn_socket (AF_SP, NN_PULL);
	bind = nn_bind(sock1, TEST_CLIENT2_URL);
	if(bind < 0)
        {
            ParodusError("Unable to bind socket (errno=%d, %s)\n",errno, strerror(errno));
        }
	
	void *buf = NULL;
	rc = nn_setsockopt(sock1, NN_SOL_SOCKET, NN_RCVTIMEO, &t, sizeof(t));
	if(rc < 0)
        {
                ParodusError ("Unable to set socket timeout (errno=%d, %s)\n",errno, strerror(errno));
        }
	
	ParodusPrint("Client 2 waiting for acknowledgement \n");
	byte = nn_recv(sock1, &buf, NN_MSG, 0);
	ParodusInfo("Data Received for client 2 : %s \n", (char * )buf);

	rv1 = wrp_to_struct((void *)buf, byte, WRP_BYTES, &msg1);
	ParodusPrint("rv1 %d \n", rv1);

	ParodusPrint("msg1->msg_type for client 2 = %d\n", msg1->msg_type);
	ParodusPrint("msg1->status for client 2 = %d\n", msg1->u.auth.status);

	nn_freemsg(buf);	

	wrp_free_struct(msg1);
	nn_shutdown(sock1, 0);
	return 0;	
}


void test_addtolist_multiple_clients()
{
	void *bytes;
	int size =0;
	int rv;
	wrp_msg_t  *message;
	int status = -1;
	reg_list_item_t *temp =NULL;
	
	const wrp_msg_t reg = { .msg_type = WRP_MSG_TYPE__SVC_REGISTRATION,
	  .u.reg.service_name = "test_client2",
	  .u.reg.url = TEST_CLIENT2_URL};
	
	/*** msgpack encode **/
	ParodusPrint("msgpack encode\n");
	size = wrp_struct_to( &reg, WRP_BYTES, &bytes );
	
	/*** decode **/
	rv = wrp_to_struct(bytes, size, WRP_BYTES, &message);
	ParodusPrint("rv is %d\n", rv);
	ParodusPrint("decoded msgType:%d\n", message->msg_type);
	ParodusPrint("decoded service_name:%s\n", message->u.reg.service_name);
	ParodusPrint("decoded dest:%s\n", message->u.reg.url);
	
	StartThread(client2_rcv_task, NULL);
	
	status = addToList(&message);
	ParodusPrint("addToList status is %d\n", status);

	
	CU_ASSERT_EQUAL( status, 0 );
	temp = get_global_node();
				
	if (NULL != temp)
	{
		temp= temp->next;
		ParodusPrint("node is pointing to service_name %s \n",temp->service_name);
		CU_ASSERT_STRING_EQUAL( temp->service_name, message->u.reg.service_name );
		CU_ASSERT_STRING_EQUAL( temp->url, message->u.reg.url );
		
	}

	wrp_free_struct(message);
	free(bytes);
	ParodusInfo("test_addtolist_multiple_clients done..\n");	
	
}

void test_client_deleteFromlist()
{
	int ret =-1;
	ret = deleteFromList("test_client1");
	CU_ASSERT_EQUAL( ret, 0 );
	ParodusInfo("test_client_deleteFromlist done..\n");

}

void test_delete_next_client_from_list()
{
	int ret =-1;
	ret = deleteFromList("test_client2");
	CU_ASSERT_EQUAL( ret, 0 );
	ParodusInfo("test_delete_next_client_from_list done..\n");

}

void test_deleteFromlist_failure()
{
	int ret = deleteFromList("sample_client");
	ParodusInfo("test_deleteFromlist_failure returns %d\n", ret);
	CU_ASSERT_EQUAL( ret, -1 );
	ParodusInfo("test_deleteFromlist_failure done..\n");

}

void test_delete_invalid_service()
{
	int ret = deleteFromList(NULL);
	ParodusInfo("test_delete_invalid_service returns %d\n", ret);
	CU_ASSERT_EQUAL( ret, -1 );
	ParodusInfo("test_delete_invalid_service done..\n");

}

void add_suites( CU_pSuite *suite )
{
    ParodusInfo("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_client_addtolist );
    CU_add_test( *suite, "Test 2", test_addtolist_multiple_clients );
    CU_add_test( *suite, "Test 3", test_delete_next_client_from_list );
    CU_add_test( *suite, "Test 4", test_client_deleteFromlist );
    CU_add_test( *suite, "Test 5", test_deleteFromlist_failure );
    CU_add_test( *suite, "Test 6", test_delete_invalid_service );
    
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
