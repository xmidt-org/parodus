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
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <assert.h>
#include <nanomsg/nn.h>
#include <nanomsg/bus.h>
#include <nanomsg/pipeline.h>
#include "../src/wss_mgr.h"
#include "wrp-c.h"

/* Nanomsg related Macros */
#define ENDPOINT "tcp://127.0.0.1:6666"
#define CLIENT1_URL "tcp://127.0.0.1:6667"
#define CLIENT2_URL "tcp://127.0.0.1:6668"
#define CLIENT3_URL "tcp://127.0.0.1:6669"

static void send_nanomsg_upstream(char **buf, int size);
headers_t headers = { 2, {"Header 1", "Header 2"}};

void test_nanomsg_client_registration1()
{
		    
	/*****Test svc registation for nanomsg client1 ***/
	printf("test_nanomsg_client_registration1\n");

	const wrp_msg_t reg = { .msg_type = WRP_MSG_TYPE__SVC_REGISTRATION,
	  .u.reg.service_name = "iot",
	  .u.reg.url = CLIENT1_URL};
	  
	void *bytes;
	int size =0;
	int rv =0;
	wrp_msg_t *message;
	int sock;
	int byte =0;
	int t = 15000;
	  
	// msgpack encode
	printf("msgpack encode\n");
	size = wrp_struct_to( &reg, WRP_BYTES, &bytes );

	
	/*** Enable this to decode and verify upstream registration msg **/
	/***	
	rv = wrp_to_struct(bytes, size, WRP_BYTES, &message);
	printf("decoded msgType:%d\n", message->msg_type);
	printf("decoded service_name:%s\n", message->u.reg.service_name);
	printf("decoded dest:%s\n", message->u.reg.url);
	wrp_free_struct(message);	
	***/
	  
	//nanomsg socket
	sock = nn_socket (AF_SP, NN_PUSH);
	CU_ASSERT(nn_connect (sock, ENDPOINT) >= 0);
	nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
	byte = nn_send (sock, bytes, size, 0);
	printf("----->Expected byte to be sent:%d\n", size);
	printf("----->actual byte sent:%d\n", byte);
	
		
	CU_ASSERT_EQUAL( byte, size );
	printf("Nanomsg client1 - Upstream Registration msg sent successfully\n");		
	free(bytes);
	sleep(2);
	nn_shutdown(sock, 0);

}

void test_nanomsg_client_registration2()
{
		    
	/*****Test svc registation for upstream - nanomsg client2 ***/
	printf("test_nanomsg_client_registration2\n");
	
	const wrp_msg_t reg = { .msg_type = WRP_MSG_TYPE__SVC_REGISTRATION,
	  .u.reg.service_name = "iot2",
	  .u.reg.url = CLIENT2_URL};
	  
	void *bytes;
	int size;
	int rv;
	wrp_msg_t *message;
	int sock;
	int byte =0;
	int t=20000;
	  
	// msgpack encode
	printf("msgpack encode\n");
	size = wrp_struct_to( &reg, WRP_BYTES, &bytes );

	/*** Enable this to decode and verify packed upstream registration msg **/
	/**
	rv = wrp_to_struct(bytes, size, WRP_BYTES, &message);
	printf("decoded msgType:%d\n", message->msg_type);
	printf("decoded service_name:%s\n", message->u.reg.service_name);
	printf("decoded dest:%s\n", message->u.reg.url);
	wrp_free_struct(message);
	***/
	  
	//nanomsg socket 
	sock = nn_socket (AF_SP, NN_PUSH);
	CU_ASSERT(nn_connect (sock, ENDPOINT) >= 0);
	nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
	byte = nn_send (sock, bytes, size,0);
	printf("----->Expected byte to be sent:%d\n", size);
	printf("----->actual byte sent:%d\n", byte);
	CU_ASSERT_EQUAL( byte, size );
	printf("Nanomsg client2 - Upstream Registration msg sent successfully\n");

	
	free(bytes);
	sleep(2);
	nn_shutdown(sock, 0);

}


void test_nanomsg_client_registration3()
{
		    
	/*****Test svc registation for upstream - nanomsg client2 ***/
	printf("test_nanomsg_client_registration3\n");
	
	const wrp_msg_t reg = { .msg_type = WRP_MSG_TYPE__SVC_REGISTRATION,
	  .u.reg.service_name = "iot",
	  .u.reg.url = CLIENT3_URL};
	void *bytes;
	int size;
	int rv;
	wrp_msg_t *message;
	int sock;
	int byte =0;
	int t=20000;
	  
	// msgpack encode
	printf("msgpack encode\n");
	size = wrp_struct_to( &reg, WRP_BYTES, &bytes );

	/*** Enable this to decode and verify packed upstream registration msg **/
	/**
	rv = wrp_to_struct(bytes, size, WRP_BYTES, &message);		
	printf("decoded msgType:%d\n", message->msg_type);
	printf("decoded service_name:%s\n", message->u.reg.service_name);
	printf("decoded dest:%s\n", message->u.reg.url);
	wrp_free_struct(message);
	***/
	  
	//nanomsg socket 
	sock = nn_socket (AF_SP, NN_PUSH);
	CU_ASSERT(nn_connect (sock, ENDPOINT) >= 0);
	nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
	byte = nn_send (sock, bytes, size,0);
	printf("----->Expected byte to be sent:%d\n", size);
	printf("----->actual byte sent:%d\n", byte);
	CU_ASSERT_EQUAL( byte, size );
	printf("Nanomsg client3 - Upstream Registration msg sent successfully\n");
	
	
	free(bytes);
	sleep(2);
	nn_shutdown(sock, 0);

}

void test_nanomsg_downstream_success()
{

	printf("test_nanomsg_downstream_success\n");
	
	int sock;
	int bit=0;
	char *buf =NULL;
	
	sock = nn_socket (AF_SP, NN_PULL);
	nn_bind (sock, CLIENT3_URL);	
	printf("***** Nanomsg client3 in Receiving mode *****\n");
	
	bit = nn_recv (sock, &buf, NN_MSG, 0);
	printf("Received %d bytes:\n", bit);
	assert (bit >= 0);
	printf ("Received downstream request from server to client3 : \"%s\"\n", buf);
	
	//To send nanomsg client response upstream
	send_nanomsg_upstream(&buf, bit);
	
	
	nn_freemsg (buf);
	nn_shutdown(sock, 0);
	sleep(60);


}


void test_nanomsg_downstream_failure()
{
	printf("test_nanomsg_downstream_failure\n");
	
	int sock;
	int bit =0;
	char *buf =NULL;
	
	sleep(60);
	sock = nn_socket (AF_SP, NN_PULL);
	nn_bind (sock, CLIENT3_URL);	
	printf("***** Nanomsg client3 in Receiving mode *****\n");
	
	bit = nn_recv (sock, &buf, NN_MSG, 0);
	assert (bit >= 0);
	printf ("Received downstream request from server for client3 : \"%s\"\n", buf);
	nn_freemsg (buf);
	nn_shutdown(sock, 0);


}


void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_nanomsg_client_registration1 );
    CU_add_test( *suite, "Test 2", test_nanomsg_client_registration2 );
    CU_add_test( *suite, "Test 3", test_nanomsg_client_registration3 );
    CU_add_test( *suite, "Test 4", test_nanomsg_downstream_success );
    //CU_add_test( *suite, "Test 5", test_nanomsg_downstream_failure );
    
    
}



/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( void )
{
    	unsigned rv = 1;
    	CU_pSuite suite = NULL;
	
    	pid_t pid, pid1;

	const char *s = getenv("WEBPA_AUTH_HEADER");

	printf("****************** WEBPA_AUTH_HEADER = %s \n", s);
         
	char * command[] = {"parodus","--hw-model=TG1682", "--hw-serial-number=Fer23u948590","--hw-manufacturer=ARRISGroup,Inc.","--hw-mac=bccab5f17962","--hw-last-reboot-reason=unknown","--fw-name=TG1682_DEV_master_2016000000sdy","--webpa-ping-time=180","--webpa-inteface-used=eth0","--webpa-url=fabric-cd.webpa.comcast.net","--webpa-backoff-max=0"};

	char * command_curl[] = {"/usr/bin/curl", "-i", "-H", "\"Authorization:Basic", s, "-H", "\"Accept: application/json\"",  "-w",  "%{time_total}",  "-k",  "\"https://api-cd.webpa.comcast.net:8090/api/v2/device/mac:bccab5f17962/iot?names=Device.DeviceInfo.Webpa.X_COMCAST-COM_SyncProtocolVersion\"", "NULL"};


	for(int i=0;i<11;i++)
	{

		printf("%s ", command_curl[i]);
	}
    	printf("Starting parodus process \n");

	pid = fork();
	
	if (pid == -1)
	{
		printf("fork was unsuccessful for pid (errno=%d, %s)\n",errno, strerror(errno));
		return -1;
		
	}
	else if (pid == 0)
	{
		printf("child process created for parodus\n");
		pid = getpid();
		printf("child process execution with pid:%d\n", pid);
		printf("Before parodus \n");
		execv("../src/parodus", command);
	}
	else if (pid > 0)
	{
		printf("Inside main from first fork \n");
	}	


	pid1 = fork();
	

	if (pid1 == -1)
	{
		printf("fork was unsuccessful for pid1 (errno=%d, %s)\n",errno, strerror(errno));
		return -1;
		
	}
	
	else if(pid1 == 0) 
	{
		printf("child process created for Downstream\n");
		pid1 = getpid();
		printf("child process execution with pid1:%d\n", pid1);
	
		sleep(120);		
		system("curl -i -H \"Authorization:Basic d2VicGFfcmVsZWFzZTEuMA==\" -H \"Accept: application/json\" -w %{time_total} -k \"https://api-cd.webpa.comcast.net:8090/api/v2/device/mac:bccab5f17962/iot?names=Device.DeviceInfo.Webpa.X_COMCAST-COM_SyncProtocolVersion\"");
		
		printf("\n----Executed first Curl request for downstream ------- \n");
		sleep(2);
		
		system("curl -i -H \"Authorization:Basic d2VicGFfcmVsZWFzZTEuMA==\" -H \"Accept: application/json\" -w %{time_total} -k \"https://api-cd.webpa.comcast.net:8090/api/v2/device/mac:bccab5f17962/config?names=Device.DeviceInfo.Webpa.X_COMCAST-COM_SyncProtocolVersion\"");
		printf("----Executed second Curl request for downstream ------- \n");
		//exit(1); 
	}
	
	else if(pid1 > 0)
	{
		
		
		sleep(40);

	    	if( CUE_SUCCESS == CU_initialize_registry() ) {
			add_suites( &suite );
			printf("Done add_suites\n");
			if( NULL != suite ) {
			    CU_basic_set_mode( CU_BRM_VERBOSE );
			    CU_basic_run_tests();
			    printf( "\n" );
			    CU_basic_show_failures( CU_get_failure_list() );
			    printf( "\n\n" );
			    rv = CU_get_number_of_tests_failed();
			   
			}

			CU_cleanup_registry();
			printf("cu registry clean done\n");
	    	}
	    	//stop parodus after all the tests run
    		sleep(5);
    	
		printf("child process execution with pid:%d\n", pid);
		kill(pid, SIGKILL);
		printf("parodus is stopped\n");
	
	  
    		if( 0 != rv ) 
    		{
        		return 1;
    		}
    		return 0;
	}

}


static void send_nanomsg_upstream(char **buf, int size)
{
	/**** To send nanomsg response to server ****/
	
	int rv;		  
	void *bytes;
	int resp_size;
	int sock;
	int byte;
	wrp_msg_t *message;
	
		
	printf("Decoding downstream request received from server\n");
	rv = wrp_to_struct((void*)(*buf), size, WRP_BYTES, &message);
	printf("after downstream req decode:%d\n", rv);	
	printf("downstream req decoded msgType:%d\n", message->msg_type);
	printf("downstream req decoded source:%s\n", message->u.req.source);
	printf("downstream req decoded dest:%s\n", message->u.req.dest);
	printf("downstream req decoded transaction_uuid:%s\n", message->u.req.transaction_uuid);
	printf("downstream req decoded payload:%s\n", (char*)message->u.req.payload); 	
	
	/**** Preparing Nanomsg client response ****/
	
	wrp_msg_t resp_m;	
	resp_m.msg_type = WRP_MSG_TYPE__REQ;
	printf("resp_m.msg_type:%d\n", resp_m.msg_type);
	
       
        resp_m.u.req.source = message->u.req.dest;        
        printf("------resp_m.u.req.source is:%s\n", resp_m.u.req.source);
        
        resp_m.u.req.dest = message->u.req.source;
        printf("------resp_m.u.req.dest is:%s\n", resp_m.u.req.dest);
        
        resp_m.u.req.transaction_uuid = message->u.req.transaction_uuid;
        printf("------resp_m.u.req.transaction_uuid is:%s\n", resp_m.u.req.transaction_uuid);
        
        resp_m.u.req.headers = NULL;
        resp_m.u.req.payload = "{statusCode:200,message:Success}";
        resp_m.u.req.payload_size = strlen(resp_m.u.req.payload);
        
        resp_m.u.req.metadata = NULL;
        resp_m.u.req.include_spans = false;
        resp_m.u.req.spans.spans = NULL;
        resp_m.u.req.spans.count = 0;
     		
	printf("Encoding downstream response\n");
	resp_size = wrp_struct_to( &resp_m, WRP_BYTES, &bytes );

	/*** Enable this to verify downstream response by decoding ***/
	/***
	wrp_msg_t *message1;
	rv = wrp_to_struct(bytes, resp_size, WRP_BYTES, &message1);
	printf("after downstream response decode:%d\n", rv);	
	printf("downstream response decoded msgType:%d\n", message1->msg_type);
	printf("downstream response decoded source:%s\n", message1->u.req.source);
	printf("downstream response decoded dest:%s\n", message1->u.req.dest);
	printf("downstream response decoded transaction_uuid:%s\n", message1->u.req.transaction_uuid);
	printf("downstream response decoded payload:%s\n", (char*)message1->u.req.payload);       
        wrp_free_struct(message1);
        ***/
        
        /**** Nanomsg client sending msgs ****/
		
	sock = nn_socket (AF_SP, NN_PUSH);
	CU_ASSERT(nn_connect (sock, ENDPOINT) >= 0);	
	sleep(1);
	
	printf("nanomsg client sending response upstream\n");
	byte = nn_send (sock, bytes, resp_size,0);
	printf("----->Expected byte to be sent:%d\n", resp_size);
	printf("----->actual byte sent:%d\n", byte);
	CU_ASSERT(byte==resp_size );
	printf("***** Nanomsg client send Upstream response successfully *****\n");
	free(bytes);
	nn_shutdown(sock, 0);
	printf("---- End of send_nanomsg_upstream ----\n");

}

