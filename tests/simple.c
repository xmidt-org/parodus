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

#define WEBPA_UPSTREAM "tcp://127.0.0.1:6666"


void test_1()
{
    int argc =1;
    char * command[]={'\0'};
     
    
    /*command[0] = "--hw-model=TG1682";
    command[1] = "--hw-serial-number=Fer23u948590";
    command[2] = "--hw-manufacturer=ARRISGroup,Inc.";
    command[3] = "--hw-mac=14cfe2142142";
    command[4] = "--hw-last-reboot-reason=unknown";
    command[5] = "--fw-name=TG1682_DEV_master_2016000000sdy";
    command[6] = "--webpa-ping-time=180";
    command[7] = "--webpa-inteface-used=eth0";
    command[8] = "--webpa-url=fabric.webpa.comcast.net";
    command[9] = "--webpa-backoff-max=0";
    
    ParodusCfg parodusCfg;
    memset(&parodusCfg,0,sizeof(parodusCfg));
    printf("call parseCommand\n");
    parseCommandLine(argc,command,&parodusCfg);
    printf("hw-model is %s\n",parodusCfg.hw_model);
    printf("hw_serial_number is %s\n",parodusCfg.hw_serial_number);
    printf("hw_manufacturer is %s\n",parodusCfg.hw_manufacturer);
    printf("hw_mac is %s\n",parodusCfg.hw_mac);
    printf("hw_last_reboot_reason is %s\n",parodusCfg.hw_last_reboot_reason);
    /////
    
    /**************Testing purpose*************/
    printf("Checking for nanomsg client msg\n");
    
    /*****Start of svc registation for upstream - nanomsg client1 ****/
    
       const wrp_msg_t reg = { .msg_type = WRP_MSG_TYPE__SVC_REGISTRATION,
          .u.reg.service_name = "iot",
          .u.reg.url = "tcp://127.0.0.1:6667"};
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
	printf("decoded msgType:%d\n", message->msg_type);
	printf("decoded service_name:%s\n", message->u.reg.service_name);
	printf("decoded dest:%s\n", message->u.reg.url);
	
        wrp_free_struct(message);
          
    //nanomsg socket 
    	int sock = nn_socket (AF_SP, NN_PUSH);
 	assert (nn_connect (sock, WEBPA_UPSTREAM) >= 0);
    	sleep(5);


	int byte = nn_send (sock, (const)bytes, size, NN_DONTWAIT);
	assert (byte == size);
	printf("msg sent\n");
	free(bytes);
	sleep(10);
   

	
   /*****Start of svc registation for upstream - nanomsg client2 ****/
   
    	byte =0;
    	rv =0;
    	size =0;
    	void *bytes1;
    	wrp_msg_t *message1;
    
     	const wrp_msg_t reg1 = { .msg_type = WRP_MSG_TYPE__SVC_REGISTRATION,
          .u.reg.service_name = "iot2",
          .u.reg.url = "tcp://127.0.0.1:6668"};
       
               
     // msgpack encode
        printf("msgpack encode\n");
	size = wrp_struct_to( &reg1, WRP_BYTES, &bytes1 );
	
     // msgpck decode
	rv = wrp_to_struct(bytes1, size, WRP_BYTES, &message1);
		
	printf("decoded msgType:%d\n", message1->msg_type);
	printf("decoded service_name:%s\n", message1->u.reg.service_name);
	printf("decoded dest:%s\n", message1->u.reg.url);
	wrp_free_struct(message1);
          
    //nanomsg socket 
          
	byte = nn_send (sock,(const)bytes1, size,NN_DONTWAIT);
	assert (byte == size);
	printf("msg sent\n");
	free(bytes1);
	sleep(10);
    
    
    /*****Start of svc registation for upstream - nanomsg client3 ****/
   
    
    	byte =0;
    	rv =0;
    	size =0;
     	void *bytes2;
     	
     	const wrp_msg_t reg2 = { .msg_type = WRP_MSG_TYPE__SVC_REGISTRATION,
          .u.reg.service_name = "iot3",
          .u.reg.url = "tcp://127.0.0.1:6669"};
       
       
       wrp_msg_t *message2;
          
     // msgpack encode
        printf("msgpack encode\n");
	size = wrp_struct_to( &reg2, WRP_BYTES, &bytes2 );
	
     // msgpck decode
	rv = wrp_to_struct(bytes2, size, WRP_BYTES, &message2);
	
	printf("decoded msgType:%d\n", message2->msg_type);
	printf("decoded service_name:%s\n", message2->u.reg.service_name);
	printf("decoded dest:%s\n", message2->u.reg.url);
	wrp_free_struct(message2);
          
    //nanomsg socket 
          
	byte = nn_send (sock, (const)bytes2, size, NN_DONTWAIT);
	assert (byte == size);
	printf("msg sent\n");
	free(bytes2);
	sleep(10);
	
	
	/*****Start of svc registation for upstream - nanomsg client4 ****/
	
    	byte =0;
    	rv =0;
    	size =0;    
     	void *bytes3;
     	
     	const wrp_msg_t reg3 = { .msg_type = WRP_MSG_TYPE__SVC_REGISTRATION,
          .u.reg.service_name = "iot1",
          .u.reg.url = "tcp://127.0.0.1:6670"};
       
       
        wrp_msg_t *message3;
          
     // msgpack encode
        printf("msgpack encode\n");
	size = wrp_struct_to( &reg3, WRP_BYTES, &bytes3 );
	
     // msgpck decode
     
	rv = wrp_to_struct(bytes3, size, WRP_BYTES, &message3);	
	printf("decoded msgType:%d\n", message3->msg_type);
	printf("decoded service_name:%s\n", message3->u.reg.service_name);
	printf("decoded dest:%s\n", message3->u.reg.url);
	wrp_free_struct(message3);
          
    //nanomsg socket 
	
	byte = nn_send (sock, (const)bytes3, size, NN_DONTWAIT);
	assert (byte == size);
	printf("msg sent\n");
	free(bytes3);
	sleep(10);
	
      /**********Nanomsg clients receiving msgs downstream******/
	
	printf("********************Clients are Receiving\n");
	int sock1 = nn_socket (AF_SP, NN_PULL);
	nn_bind (sock1, "tcp://127.0.0.1:6667");
	char *buf =NULL;
	
	int bit = nn_recv (sock1, &buf, NN_MSG, 0);
	assert (bit >= 0);
	printf ("Server Response \"%s\"\n", buf);
	nn_freemsg (buf);
	
	sleep(10);
	
	
	//Sending after recv
	
	void *bytes4;
     	const wrp_msg_t reg4 = { .msg_type = WRP_MSG_TYPE__SVC_REGISTRATION,
          .u.reg.service_name = "iot4",
          .u.reg.url = "tcp://127.0.0.1:6671"};
       wrp_msg_t *message4;
          
     // msgpack encode
        printf("msgpack encode\n");
	size = wrp_struct_to( &reg4, WRP_BYTES, &bytes4 );
	
	
     // msgpck decode
	rv = wrp_to_struct(bytes4, size, WRP_BYTES, &message4);
	
	printf("decoded msgType:%d\n", message4->msg_type);
	printf("decoded service_name:%s\n", message4->u.reg.service_name);
	printf("decoded dest:%s\n", message4->u.reg.url);
	
        wrp_free_struct(message4);
          
    //nanomsg socket 
	
	byte = nn_send (sock, (const)bytes4, size, NN_DONTWAIT);
	assert (byte == size);
	printf("msg sent\n");
	nn_shutdown (sock, 0);
	printf("Passed upstream and downstream testing\n");
    
    /**************Testing purpose*************/
        
}

void add_suites( CU_pSuite *suite )
{
    *suite = CU_add_suite( "tests", NULL, NULL );
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
            printf("Creating NanoMsg Socket\n");
            int sock = nn_socket (AF_SP, NN_BUS);
            assert (sock >= 0);
            printf("Socket creation successful sock=%d \n", sock);
        }

        CU_cleanup_registry();
    }

    if( 0 != rv ) {
        return 1;
    }
    return 0;
}
