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

#include <CUnit/Basic.h>
#include <stdbool.h>

#include <assert.h>
#include <nopoll.h>

//#include <nanomsg/bus.h>

#include "../src/wss_mgr.h"
#include "../src/ParodusInternal.h"
#include "wrp-c.h"

#include<errno.h>

/* Nanomsg related Macros */
#define ENDPOINT "tcp://127.0.0.1:6666"
#define CLIENT1_URL "tcp://127.0.0.1:6667"
#define CLIENT2_URL "tcp://127.0.0.1:6668"
#define CLIENT3_URL "tcp://127.0.0.1:6669"

static void send_nanomsg_upstream(void **buf, int size);
void *handle_testsuites();
headers_t headers = { 2, {"Header 1", "Header 2"}};

void test_nanomsg_client_registration1()
{
		    
	/*****Test svc registation for nanomsg client1 ***/
	cimplog_info("PARODUS", "test_nanomsg_client_registration1\n");

	const wrp_msg_t reg = { .msg_type = WRP_MSG_TYPE__SVC_REGISTRATION,
	  .u.reg.service_name = "iot",
	  .u.reg.url = CLIENT1_URL};
	  
	void *bytes;
	int size =0;
	int rv1;
	wrp_msg_t  *msg1;
	int sock;
	int byte =0;
	int t=25000;
	  
	// msgpack encode
	cimplog_debug("PARODUS", "msgpack encode\n");
	size = wrp_struct_to( &reg, WRP_BYTES, &bytes );

	
	/*** Enable this to decode and verify upstream registration msg **/
	/***	
	rv = wrp_to_struct(bytes, size, WRP_BYTES, &message);
	cimplog_debug("PARODUS", "decoded msgType:%d\n", message->msg_type);
	cimplog_debug("PARODUS", "decoded service_name:%s\n", message->u.reg.service_name);
	cimplog_debug("PARODUS", "decoded dest:%s\n", message->u.reg.url);
	wrp_free_struct(message);	
	***/
	  
	//nanomsg socket
	sock = nn_socket (AF_SP, NN_PUSH);
	int connect = nn_connect (sock, ENDPOINT);
	CU_ASSERT(connect >= 0);
	nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
	byte = nn_send (sock, bytes, size, 0);
	cimplog_info("PARODUS", "----->Expected byte to be sent:%d\n", size);
	cimplog_info("PARODUS", "----->actual byte sent:%d\n", byte);
	cimplog_info("PARODUS", "Nanomsg client1 - Testing Upstream Registration msg send\n");		
	CU_ASSERT_EQUAL( byte, size );

	//************************************************************

	int sock1 = nn_socket (AF_SP, NN_PULL);
	byte = 0;
	nn_bind(sock1, reg.u.reg.url);
	
	void *buf = NULL;
	nn_setsockopt(sock1, NN_SOL_SOCKET, NN_RCVTIMEO, &t, sizeof(t));
	
	cimplog_debug("PARODUS", "Client 1 waiting for acknowledgement \n");
	byte = nn_recv(sock1, &buf, NN_MSG, 0);
	cimplog_info("PARODUS", "Data Received for client 1 : %s \n", (char * )buf);

	rv1 = wrp_to_struct((void *)buf, byte, WRP_BYTES, &msg1);

	CU_ASSERT_EQUAL( rv1, byte );
	
	cimplog_debug("PARODUS", "msg1->msg_type for client 1 = %d \n", msg1->msg_type);
	cimplog_debug("PARODUS", "msg1->status for client 1 = %d \n", msg1->u.auth.status);
	CU_ASSERT_EQUAL(msg1->msg_type, 2);
	CU_ASSERT_EQUAL(msg1->u.auth.status, 200);

	nn_freemsg(buf);	

	free(bytes);	
	wrp_free_struct(msg1);
	nn_shutdown(sock, 0);
	nn_shutdown(sock1, 0);
	

}

void test_nanomsg_client_registration2()
{
		    
	/*****Test svc registation for upstream - nanomsg client2 ***/
	cimplog_info("PARODUS", "test_nanomsg_client_registration2\n");
	
	const wrp_msg_t reg = { .msg_type = WRP_MSG_TYPE__SVC_REGISTRATION,
	  .u.reg.service_name = "iot2",
	  .u.reg.url = CLIENT2_URL};
	  
	void *bytes;
	int size;
	int rv1;
	wrp_msg_t  *msg1;
	int sock;
	int byte =0;
	int t=28000;
	  
	// msgpack encode
	cimplog_debug("PARODUS", "msgpack encode\n");
	size = wrp_struct_to( &reg, WRP_BYTES, &bytes );

	/*** Enable this to decode and verify packed upstream registration msg **/
	/**
	rv = wrp_to_struct(bytes, size, WRP_BYTES, &message);
	cimplog_debug("PARODUS", "decoded msgType:%d\n", message->msg_type);
	cimplog_debug("PARODUS", "decoded service_name:%s\n", message->u.reg.service_name);
	cimplog_debug("PARODUS", "decoded dest:%s\n", message->u.reg.url);
	wrp_free_struct(message);
	***/
	  
	//nanomsg socket 
	sock = nn_socket (AF_SP, NN_PUSH);
	int connect = nn_connect (sock, ENDPOINT);
	CU_ASSERT( connect >= 0);
	nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
	byte = nn_send (sock, bytes, size,0);
	cimplog_info("PARODUS", "----->Expected byte to be sent:%d\n", size);
	cimplog_info("PARODUS", "----->actual byte sent:%d\n", byte);
	cimplog_info("PARODUS", "Nanomsg client2 - Testing Upstream Registration msg send\n");
	CU_ASSERT_EQUAL( byte, size );



	int sock1 = nn_socket (AF_SP, NN_PULL);
	byte = 0;
	
	nn_bind(sock1, reg.u.reg.url);
	
	void *buf1 = NULL;
	
	nn_setsockopt(sock1, NN_SOL_SOCKET, NN_RCVTIMEO, &t, sizeof(t));

	cimplog_debug("PARODUS", "Client 2 waiting for acknowledgement \n");
	
	byte = nn_recv(sock1, &buf1, NN_MSG, 0);
	cimplog_info("PARODUS", "Data Received : %s \n", (char * )buf1);
	

	rv1 = wrp_to_struct((void *)buf1, byte, WRP_BYTES, &msg1);
	CU_ASSERT_EQUAL( rv1, byte );
	cimplog_debug("PARODUS", "msg1->msg_type for client 2 = %d \n", msg1->msg_type);
	cimplog_debug("PARODUS", "msg1->status for client 2 = %d \n", msg1->u.auth.status);
	CU_ASSERT_EQUAL(msg1->msg_type, 2);
	CU_ASSERT_EQUAL(msg1->u.auth.status, 200);
	
	nn_freemsg(buf1);	
	
	free(bytes);
	wrp_free_struct(msg1);
	nn_shutdown(sock, 0);
	nn_shutdown(sock1, 0);

}


void test_nanomsg_client_registration3()
{
		    
	/*****Test svc registation for upstream - nanomsg client2 ***/
	cimplog_info("PARODUS", "test_nanomsg_client_registration3\n");
	
	const wrp_msg_t reg = { .msg_type = WRP_MSG_TYPE__SVC_REGISTRATION,
	  .u.reg.service_name = "iot",
	  .u.reg.url = CLIENT3_URL};
	void *bytes;
	int size;
	int rv1;
	wrp_msg_t *msg1;
	int sock;
	int byte =0;
	int t=35000;
	  
	// msgpack encode
	cimplog_debug("PARODUS", "msgpack encode\n");
	size = wrp_struct_to( &reg, WRP_BYTES, &bytes );

	/*** Enable this to decode and verify packed upstream registration msg **/
	/**
	rv = wrp_to_struct(bytes, size, WRP_BYTES, &message);		
	cimplog_debug("PARODUS", "decoded msgType:%d\n", message->msg_type);
	cimplog_debug("PARODUS", "decoded service_name:%s\n", message->u.reg.service_name);
	cimplog_debug("PARODUS", "decoded dest:%s\n", message->u.reg.url);
	wrp_free_struct(message);
	***/
	  
	//nanomsg socket 
	sock = nn_socket (AF_SP, NN_PUSH);
	int connect = nn_connect (sock, ENDPOINT);
	CU_ASSERT(connect >= 0);
	nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
	byte = nn_send (sock, bytes, size,0);
	cimplog_info("PARODUS", "----->Expected byte to be sent:%d\n", size);
	cimplog_info("PARODUS", "----->actual byte sent:%d\n", byte);
	cimplog_info("PARODUS", "Nanomsg client3 - Testing Upstream Registration msg send\n");
	CU_ASSERT_EQUAL( byte, size );


	int sock1 = nn_socket (AF_SP, NN_PULL);
	byte = 0;
	
	int bind = nn_bind(sock1, reg.u.reg.url);

	cimplog_debug("PARODUS", "Need to close this bind %d \n", bind);
	
	void *buf2 = NULL;
	
	nn_setsockopt(sock1, NN_SOL_SOCKET, NN_RCVTIMEO, &t, sizeof(t));
	cimplog_debug("PARODUS", "Client 3 is waiting for acknowledgement \n");
	byte = nn_recv(sock1, &buf2, NN_MSG, 0);
	
	cimplog_info("PARODUS", "Data Received : %s \n", (char * )buf2);

	
	rv1 = wrp_to_struct((void *)buf2, byte, WRP_BYTES, &msg1);
	CU_ASSERT_EQUAL( rv1, byte );
	cimplog_debug("PARODUS", "msg1->msg_type for client 3 = %d \n", msg1->msg_type);
	cimplog_debug("PARODUS", "msg1->status for client 3 = %d \n", msg1->u.auth.status);
	CU_ASSERT_EQUAL(msg1->msg_type, 2);
	CU_ASSERT_EQUAL(msg1->u.auth.status, 200);
	
	
	nn_freemsg(buf2);
	
	free(bytes);
	wrp_free_struct(msg1);
	nn_shutdown(sock, 0);
	nn_shutdown(sock1, bind);
	
	

}

void test_nanomsg_downstream_success()
{

	cimplog_info("PARODUS", "test_nanomsg_downstream_success\n");
	
	int sock;
	int bit=0;
	wrp_msg_t *message;
	void *buf =NULL;
	char* destVal = NULL;
//	char dest[32] = {'\0'};
	char *dest = NULL;
	//char *temp_ptr;
	int bind = -1;
	
	const wrp_msg_t msg = { .msg_type = WRP_MSG_TYPE__SVC_REGISTRATION,
	  .u.reg.service_name = "iot",
	  .u.reg.url = CLIENT3_URL};
	
	sock = nn_socket (AF_SP, NN_PULL);

	while(bind == -1)
        {
                bind = nn_bind(sock, msg.u.reg.url);
                sleep(3);
        }
	
	cimplog_debug("PARODUS", "Bind returns = %d \n", bind);
	cimplog_debug("PARODUS", "***** Nanomsg client3 in Receiving mode in %s *****\n", msg.u.reg.url);
	bit = nn_recv (sock, &buf, NN_MSG, 0);
	cimplog_info ("----->Received downstream request from server to client3 : \"%s\"\n", (char *)buf);
	cimplog_debug("PARODUS", "Received %d bytes\n", bit);
	CU_ASSERT(bit >= 0);
	
	//Decode and verify downstream request has received by correct registered client
	
	wrp_to_struct(buf, bit, WRP_BYTES, &message);		
	destVal = message->u.req.dest;
	dest = strtok(destVal , "/");
	//temp_ptr = strtok(destVal , "/");
//	cimplog_debug("PARODUS", "temp_ptr = %s \n", temp_ptr);
	strcpy(dest,strtok(NULL , "/"));
	cimplog_info("PARODUS", "------>decoded dest:%s\n", dest);	
	CU_ASSERT_STRING_EQUAL( msg.u.reg.service_name, dest );	
	wrp_free_struct(message);
	
	//To send nanomsg client response upstream
	send_nanomsg_upstream(&buf, bit);
	
	nn_freemsg(buf);
	nn_shutdown(sock, bind);
	
	//Need to wait for parodus to finish it's task.
	sleep(10);


}


void test_nanomsg_downstream_failure()
{
	cimplog_error("PARODUS", "test_nanomsg_downstream_failure\n");
	
	int sock;
	int bit =0;
	char *buf =NULL;
	
	sleep(60);
	sock = nn_socket (AF_SP, NN_PULL);
	nn_bind (sock, CLIENT3_URL);	
	cimplog_debug("PARODUS", "***** Nanomsg client3 in Receiving mode *****\n");
	
	bit = nn_recv (sock, &buf, NN_MSG, 0);
	cimplog_info ("Received downstream request from server for client3 : \"%s\"\n", buf);
	CU_ASSERT(bit >= 0);
	nn_freemsg(buf);
	nn_shutdown(sock, 0);


}


void test_checkHostIp()
{
	cimplog_debug("PARODUS", "**********************************Calling check_host_ip \n");

	int ret;
	
	ret = checkHostIp("fabric.webpa.comcast.net");
	cimplog_debug("PARODUS", "------------------> Ret = %d \n", ret);
	CU_ASSERT_EQUAL(ret, 0);
	
}

void test_handleUpstreamMessage()
{

	cimplog_debug("PARODUS", "**********************************Calling handleUpstreamMessage \n");
	
	noPollConnOpts * opts;
	noPollCtx *ctx = NULL;
	noPollConn *conn = NULL;

	const char * headerNames[HTTP_CUSTOM_HEADER_COUNT] = {"X-WebPA-Device-Name","X-WebPA-Device-Protocols","User-Agent", "X-WebPA-Convey"};
	const char * headerValues[HTTP_CUSTOM_HEADER_COUNT];

	headerValues[0] = "123567892366";
	headerValues[1] = "wrp-0.11,getset-0.1";  
	headerValues[2] = "WebPA-1.6 (TG1682_DEV_master_2016000000sdy;TG1682/ARRISGroup,Inc.;)";
	headerValues[3] = "zacbvfxcvglodfjdigjkdshuihgkvn";

	int headerCount = HTTP_CUSTOM_HEADER_COUNT;

	//ctx = nopoll_ctx_new();

	opts = nopoll_conn_opts_new ();
	nopoll_conn_opts_ssl_peer_verify (opts, nopoll_false);
	nopoll_conn_opts_set_ssl_protocol (opts, NOPOLL_METHOD_TLSV1_2); 
	conn = nopoll_conn_tls_new(ctx, opts, "fabric.webpa.comcast.net", "8080", NULL, "/api/v2/device", NULL, NULL, "eth0",
                                headerNames, headerValues, headerCount);
	/*while(conn == NULL)
	{
		opts = nopoll_conn_opts_new ();
		nopoll_conn_opts_ssl_peer_verify (opts, nopoll_false);
		nopoll_conn_opts_set_ssl_protocol (opts, NOPOLL_METHOD_TLSV1_2); 
		conn = nopoll_conn_tls_new(ctx, opts, "fabric.webpa.comcast.net", 8080, NULL, "/api/v2/device", NULL, NULL, "eth0",
                                headerNames, headerValues, headerCount);
	}*/

	cimplog_debug("PARODUS", "Sending conn as %p \n", conn);
	handleUpstreamMessage(conn, "hello", 6);

}

void test_parseCommandLine()
{
    int argc =11;
    char * command[15]={'\0'};
     
    command[0] = "parodus";
    command[1] = "--hw-model=TG1682";
    command[2] = "--hw-serial-number=Fer23u948590";
    command[3] = "--hw-manufacturer=ARRISGroup,Inc.";
    command[4] = "--hw-mac=123567892366";
    command[5] = "--hw-last-reboot-reason=unknown";
    command[6] = "--fw-name=TG1682_DEV_master_2016000000sdy";
    command[7] = "--webpa-ping-time=180";
    command[8] = "--webpa-inteface-used=eth0";
    command[9] = "--webpa-url=fabric.webpa.comcast.net";
    command[10] = "--webpa-backoff-max=0";
    
    ParodusCfg parodusCfg;
    memset(&parodusCfg,0,sizeof(parodusCfg));
    cimplog_debug("PARODUS", "call parseCommand\n");
    parseCommandLine(argc,command,&parodusCfg);
   
    cimplog_debug("PARODUS", "parodusCfg.webpa_ping_timeout is %d\n", parodusCfg.webpa_ping_timeout);
    cimplog_debug("PARODUS", "parodusCfg.webpa_backoff_max is %d\n", parodusCfg.webpa_backoff_max);
    CU_ASSERT_STRING_EQUAL( parodusCfg.hw_model, "TG1682");
    CU_ASSERT_STRING_EQUAL( parodusCfg.hw_serial_number, "Fer23u948590");
    CU_ASSERT_STRING_EQUAL( parodusCfg.hw_manufacturer, "ARRISGroup,Inc.");
    CU_ASSERT_STRING_EQUAL( parodusCfg.hw_mac, "123567892366");	
    CU_ASSERT_STRING_EQUAL( parodusCfg.hw_last_reboot_reason, "unknown");	
    CU_ASSERT_STRING_EQUAL( parodusCfg.fw_name, "TG1682_DEV_master_2016000000sdy");	
    CU_ASSERT( parodusCfg.webpa_ping_timeout==180);	
    CU_ASSERT_STRING_EQUAL( parodusCfg.webpa_interface_used, "eth0");	
    CU_ASSERT_STRING_EQUAL( parodusCfg.webpa_url, "fabric.webpa.comcast.net");
    CU_ASSERT( parodusCfg.webpa_backoff_max==0);
}


void test_loadParodusCfg()
{
	
	cimplog_debug("PARODUS", "Calling test_loadParodusCfg \n");
	//ParodusCfg parodusCfg, tmpcfg;
	ParodusCfg  tmpcfg;

	ParodusCfg *Cfg;

	Cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
	
	strcpy(Cfg->hw_model, "TG1682");
	strcpy(Cfg->hw_serial_number, "Fer23u948590");
	strcpy(Cfg->hw_manufacturer , "ARRISGroup,Inc.");
	strcpy(Cfg->hw_mac , "123567892366");
	
	
	memset(&tmpcfg,0,sizeof(tmpcfg));
	
	
	loadParodusCfg(Cfg,&tmpcfg);

	cimplog_info("PARODUS", "tmpcfg.hw_model = %s, tmpcfg.hw_serial_number = %s, tmpcfg.hw_manufacturer = %s, tmpcfg.hw_mac = %s, \n", tmpcfg.hw_model,tmpcfg.hw_serial_number, tmpcfg.hw_manufacturer,   tmpcfg.hw_mac);

	CU_ASSERT_STRING_EQUAL( tmpcfg.hw_model, "TG1682");
	CU_ASSERT_STRING_EQUAL( tmpcfg.hw_serial_number, "Fer23u948590");
	CU_ASSERT_STRING_EQUAL( tmpcfg.hw_manufacturer, "ARRISGroup,Inc.");
	CU_ASSERT_STRING_EQUAL( tmpcfg.hw_mac, "123567892366");	

}

void add_suites( CU_pSuite *suite )
{
    cimplog_info("PARODUS", "--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_nanomsg_client_registration1 );
    CU_add_test( *suite, "Test 2", test_nanomsg_client_registration2 );
    CU_add_test( *suite, "Test 3", test_nanomsg_client_registration3 );
    CU_add_test( *suite, "Test 4", test_nanomsg_downstream_success );
    //CU_add_test( *suite, "Test 5", test_nanomsg_downstream_failure );

    cimplog_info("PARODUS", "-------------Integration testing is completed-----------\n");
    cimplog_info("PARODUS", "******************************************************************\n");
    //sleep(10);
    cimplog_info("PARODUS", "-------------Start of Unit Test Cases Execution---------\n");
    CU_add_test( *suite, "UnitTest 1", test_parseCommandLine );
    CU_add_test( *suite, "UnitTest 2", test_checkHostIp );
	
    CU_add_test( *suite, "UnitTest 3", test_handleUpstreamMessage );

    CU_add_test( *suite, "UnitTest 4", test_loadParodusCfg );
    
    
    
}




/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( void )
{
    	pid_t pid, pid1;
    	char value[512] = {'\0'};
  	char* data =NULL;
  	int status;
	char commandUrl[255];
	pid_t curl_pid;
  

	char * command[] = {"parodus","--hw-model=TG1682", "--hw-serial-number=Fer23u948590","--hw-manufacturer=ARRISGroup,Inc.","--hw-mac=123567892366","--hw-last-reboot-reason=unknown","--fw-name=TG1682_DEV_master_2016000000sdy","--boot-time=10","--webpa-ping-time=180","--webpa-inteface-used=eth0","--webpa-url=fabric-cd.webpa.comcast.net","--webpa-backoff-max=9", NULL};


	//int size = sizeof(command)/sizeof(command[0]);
	//int i;
	//cimplog_info("PARODUS", "commad: ");
        //for(i=0;i<size-1;i++)
        //cimplog_info("PARODUS", "command:%s",command);

    	cimplog_info("PARODUS", "Starting parodus process \n");

	const char *s = getenv("WEBPA_AUTH_HEADER");
	

	sprintf(commandUrl, "curl -i -H \"Authorization:Basic %s\" -H \"Accept: application/json\" -w %%{time_total} -k \"https://api-cd.webpa.comcast.net:8090/api/v2/device/mac:123567892366/iot?names=Device.DeviceInfo.Webpa.X_COMCAST-COM_SyncProtocolVersion\"", s);	
	cimplog_debug("PARODUS", "---------------------->>>>Executing system(commandUrl)\n");
	
	curl_pid = getpid();
	cimplog_debug("PARODUS", "child process execution with curl_pid:%d\n", curl_pid);

	

	pid = fork();
	
	if (pid == -1)
	{
		cimplog_error("PARODUS", "fork was unsuccessful for pid (errno=%d, %s)\n",errno, strerror(errno));
		return -1;
		
	}
	else if (pid == 0)
	{
		int err;
		cimplog_debug("PARODUS", "child process created for parodus\n");
		pid = getpid();
		cimplog_debug("PARODUS", "child process execution with pid:%d\n", pid);
		
		err = execv("../src/parodus", command);
		if(errno == 2)
		{
			err = execv("./src/parodus", command);
		}
		cimplog_error("PARODUS", "err is %d, errno is %d\n",err, errno);		
	
	}
	else if (pid > 0)
	{
		int link[2];
		sleep(5);
		
		//Starting test suites execution in new thread
		cimplog_debug("PARODUS", "Creating new thread for test suite execution\n");
	
		pthread_t testId;
		int err1 = 0;
		err1 = pthread_create(&testId,NULL,handle_testsuites,(void *)&pid);
		if(err1 != 0)
			cimplog_error("PARODUS", "Error creating test suite thread %s\n",strerror(err1));
		else
			cimplog_debug("PARODUS", "test suite thread created successfully\n");
			
		
	
	  	if (pipe(link)==-1)
	  	{
	    		cimplog_error("PARODUS", "Failed to create pipe\n");
	    
	  	}
	  	else
	  		cimplog_debug("PARODUS", "Created pipe to read curl output\n");

	
		pid1 = fork();
	
		if (pid1 == -1)
		{
			cimplog_error("PARODUS", "fork was unsuccessful for pid1 (errno=%d, %s)\n",errno, strerror(errno));
			return -1;
		
		}
	
		else if(pid1 == 0) 
		{
									
									
			while(NULL == fopen("/tmp/parodus_ready", "r"))
			{
				
				sleep(5);
			}
	    		dup2 (link[1], STDOUT_FILENO);
	    		close(link[0]);
	    		close(link[1]);
			sleep(40);
			system(commandUrl);			
			cimplog_info("PARODUS", "\n----Executed first Curl request for downstream ------- \n");
			
		
		}
	
		else if(pid1 > 0)
		{
				
			//wait fro child process to finish and read from pipe
			waitpid(pid1, &status, 0);
		
			//reading from pipe
			cimplog_debug("PARODUS", "parent process...:%d\n", pid1);
			close(link[1]);
			int nbytes = read(link[0], value, sizeof(value));
			cimplog_debug("PARODUS", "Read %d \n", nbytes);
			   
		      	if ((data = strstr(value, "message:Success")) !=NULL)
		      	{
		      		cimplog_info("PARODUS", "curl success\n");
		      	}
		      	else
		      	{
		      		cimplog_error("PARODUS", "curl failure..\n");
		      	}
			 
	    		while(1);

		}
	}	

return 0;
}


void *handle_testsuites(void* pid)
{
	unsigned rv = 1;
	CU_pSuite suite = NULL;
	pid_t pid_parodus = *((int *)pid);
	
	cimplog_debug("PARODUS", "Starting handle_testsuites thread\n");
	sleep(25);

    	if( CUE_SUCCESS == CU_initialize_registry() ) {
		add_suites( &suite );
		
		if( NULL != suite ) {
		    CU_basic_set_mode( CU_BRM_VERBOSE );
		    CU_basic_run_tests();
		    cimplog_debug("PARODUS",  "\n" );
		    CU_basic_show_failures( CU_get_failure_list() );
		    cimplog_debug("PARODUS",  "\n\n" );
		    rv = CU_get_number_of_tests_failed();
		   
		}

		CU_cleanup_registry();
		
    	}
    	
    	kill(pid_parodus, SIGKILL);
	cimplog_info("PARODUS", "parodus process with pid %d is stopped\n", pid_parodus);


	
		
    	if( 0 != rv ) 
	{
		_exit(-1);
	}
	
	
	_exit(0);
}


static void send_nanomsg_upstream(void **buf, int size)
{
	/**** To send nanomsg response to server ****/
	
	int rv;		  
	void *bytes;
	int resp_size;
	int sock;
	int byte;
	wrp_msg_t *message;
	
		
	cimplog_info("PARODUS", "Decoding downstream request received from server\n");
	rv = wrp_to_struct(*buf, size, WRP_BYTES, &message);
	cimplog_debug("PARODUS", "after downstream req decode:%d\n", rv);	
		
	
	/**** Preparing Nanomsg client response ****/
	
	wrp_msg_t resp_m;	
	resp_m.msg_type = WRP_MSG_TYPE__REQ;
	cimplog_debug("PARODUS", "resp_m.msg_type:%d\n", resp_m.msg_type);
	
       
        resp_m.u.req.source = message->u.req.dest;        
        cimplog_debug("PARODUS", "------resp_m.u.req.source is:%s\n", resp_m.u.req.source);
        
        resp_m.u.req.dest = message->u.req.source;
        cimplog_debug("PARODUS", "------resp_m.u.req.dest is:%s\n", resp_m.u.req.dest);
        
        resp_m.u.req.transaction_uuid = message->u.req.transaction_uuid;
        cimplog_debug("PARODUS", "------resp_m.u.req.transaction_uuid is:%s\n", resp_m.u.req.transaction_uuid);
        
        resp_m.u.req.headers = NULL;
        resp_m.u.req.payload = "{statusCode:200,message:Success}";
        cimplog_debug("PARODUS", "------resp_m.u.req.payload is:%s\n", (char *)resp_m.u.req.payload);
        resp_m.u.req.payload_size = strlen(resp_m.u.req.payload);
        
        resp_m.u.req.metadata = NULL;
        resp_m.u.req.include_spans = false;
        resp_m.u.req.spans.spans = NULL;
        resp_m.u.req.spans.count = 0;
     		
	cimplog_debug("PARODUS", "Encoding downstream response\n");
	resp_size = wrp_struct_to( &resp_m, WRP_BYTES, &bytes );

	/*** Enable this to verify downstream response by decoding ***/
	/***
	wrp_msg_t *message1;
	rv = wrp_to_struct(bytes, resp_size, WRP_BYTES, &message1);
	cimplog_debug("PARODUS", "after downstream response decode:%d\n", rv);	
	cimplog_debug("PARODUS", "downstream response decoded msgType:%d\n", message1->msg_type);
	cimplog_debug("PARODUS", "downstream response decoded source:%s\n", message1->u.req.source);
	cimplog_debug("PARODUS", "downstream response decoded dest:%s\n", message1->u.req.dest);
	cimplog_debug("PARODUS", "downstream response decoded transaction_uuid:%s\n", message1->u.req.transaction_uuid);
	cimplog_debug("PARODUS", "downstream response decoded payload:%s\n", (char*)message1->u.req.payload);       
        wrp_free_struct(message1);
        ***/
        
        /**** Nanomsg client sending msgs ****/
		
	sock = nn_socket (AF_SP, NN_PUSH);
	int connect = nn_connect (sock, ENDPOINT);
	CU_ASSERT(connect >= 0);	
	sleep(1);
	
	cimplog_info("PARODUS", "nanomsg client sending response upstream\n");
	byte = nn_send (sock, bytes, resp_size,0);
	cimplog_info("PARODUS", "----->Expected byte to be sent:%d\n", resp_size);
	cimplog_info("PARODUS", "----->actual byte sent:%d\n", byte);
	CU_ASSERT(byte==resp_size );
	wrp_free_struct(message);
	
	free(bytes);
	nn_shutdown(sock, 0);
	cimplog_debug("PARODUS", "---- End of send_nanomsg_upstream ----\n");

}


