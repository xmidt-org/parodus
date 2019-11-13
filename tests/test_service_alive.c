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
#include "../src/service_alive.h"
#include <sys/types.h>
#include <unistd.h>

#define TEST_SERVICE_URL "tcp://127.0.0.1:6688"

static void *client_rcv_task();
static void *keep_alive_thread();
static void add_client();
int sock1;
pthread_t threadId;
pthread_mutex_t crud_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t crud_con=PTHREAD_COND_INITIALIZER;

pthread_cond_t *get_global_crud_con(void)
{
    return &crud_con;
}

pthread_mutex_t *get_global_crud_mut(void)
{
    return &crud_mut;
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void addCRUDmsgToQueue(wrp_msg_t *crudMsg)
{
	(void)crudMsg;
	return;
}

void *CRUDHandlerTask()
{
	return NULL;
}

static void add_client()
{
	const wrp_msg_t reg = { .msg_type = WRP_MSG_TYPE__SVC_REGISTRATION,
	  .u.reg.service_name = "service_client",
	  .u.reg.url = TEST_SERVICE_URL};
	  
	pthread_t test_tid;
	void *bytes;
	int size =0;
	int rv;
	wrp_msg_t  *message;
	int status = -1;
	  
	// msgpack encode
	size = wrp_struct_to( &reg, WRP_BYTES, &bytes );
	
	/*** decode **/
		
	rv = wrp_to_struct(bytes, size, WRP_BYTES, &message);
	ParodusPrint("rv is %d\n", rv);
	ParodusPrint("decoded msgType:%d\n", message->msg_type);
	ParodusPrint("decoded service_name:%s\n", message->u.reg.service_name);
	ParodusPrint("decoded dest:%s\n", message->u.reg.url);
	
	StartThread(client_rcv_task, &test_tid);
	status = addToList(&message);
	ParodusPrint("addToList status is %d\n", status);
	
	CU_ASSERT_EQUAL( status, 0 );
	wrp_free_struct(message);
	free(bytes);
	
}

static void *client_rcv_task()
{
	int byte =0;
	int rv1=0;
	int t=25000;
	int rc = -1;
	int bind;
	wrp_msg_t  *msg1;

	sock1 = nn_socket (AF_SP, NN_PULL);
	bind = nn_bind(sock1, TEST_SERVICE_URL);
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

	CU_ASSERT_EQUAL( rv1, byte );
	
	ParodusPrint("msg1->msg_type for client 1 = %d \n", msg1->msg_type);
	ParodusPrint("msg1->status for client 1 = %d \n", msg1->u.auth.status);
	CU_ASSERT_EQUAL(msg1->msg_type, 2);
	CU_ASSERT_EQUAL(msg1->u.auth.status, 200);

	nn_freemsg(buf);	

	wrp_free_struct(msg1);
	return 0;	
}

static void handler(int signum)
{
	UNUSED(signum);
	pthread_exit(NULL);
}

static void *keep_alive_thread()
{
	//ParodusPrint("keep_alive threadId is %d\n", threadId);
	sleep(2);
	ParodusPrint("Starting serviceAliveTask..\n");
	while (true) {
	  serviceAliveTask();
	  sleep (30);
	}
	return 0;
}

void test_keep_alive()
{
	int byte =0;
	int rv1=0;
	wrp_msg_t  *msg1;
	int err = 0;

	void *buf = NULL;
	
	ParodusPrint("adding client to list\n");
	add_client();
	
	err = pthread_create(&threadId, NULL, keep_alive_thread, NULL);
	if (err != 0) 
	{
		ParodusError("Error creating thread :[%s]\n", strerror(err));
                exit(1);
	}
	else
	{
		ParodusPrint("Thread created Successfully %p\n", threadId);
	}    
	sleep(3);
	signal(SIGUSR1, handler);
	
	while( 1 ) 
	{
		ParodusPrint("service waiting for keep alive msg \n");
		byte = nn_recv(sock1, &buf, NN_MSG, 0);
		
		if(byte >0)
		{
			ParodusInfo("Received keep alive msg!!! : %s \n", (char * )buf);
            pthread_kill(threadId, SIGUSR1);
			
			ParodusInfo("keep_alive_thread with tid %p is stopped\n", threadId);
			break;
		}
		else
		{
			 sleep(1);
		}

	}	
	rv1 = wrp_to_struct((void *)buf, byte, WRP_BYTES, &msg1);
	CU_ASSERT_EQUAL( rv1, byte );

	ParodusPrint("msg1->msg_type for service = %d \n", msg1->msg_type);

	CU_ASSERT_EQUAL(msg1->msg_type, 10);

	nn_freemsg(buf);	
	wrp_free_struct(msg1);
	nn_shutdown(sock1, 0);
	ParodusPrint("!!!!!test_keep_alive done..\n");
	
}


void add_suites( CU_pSuite *suite )
{
    ParodusInfo("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_keep_alive );
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
