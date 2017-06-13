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
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <assert.h>

#include "../src/ParodusInternal.h"
#include "../src/connection.h"
#include "../src/config.h"
#include "../src/upstream.h"
#include "../src/lws_handlers.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

bool conn_retry;
volatile unsigned int heartBeatTimer;

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/


int lws_is_final_fragment (struct lws * wsi)
{
	function_called();
	return (int) mock();
}

int lws_callback_on_writable (struct lws * wsi)
{
	return 1;
}
void listenerOnrequest_queue(void *requestMsg,int reqSize)
{
	UNUSED(requestMsg);
	UNUSED(reqSize);
}

int lws_write (struct lws * wsi,unsigned char *  buf,size_t  len,enum lws_write_protocol  protocol)
{
	function_called();
	return (int) mock();
}

char* getWebpaConveyHeader()
{
    function_called();
    return (char*) (intptr_t)mock();
}

int LWS_WARN_UNUSED_RESULT lws_add_http_header_by_name (struct lws * wsi,const unsigned char *name,const unsigned char *value,int length,unsigned char **p,unsigned char *end)
{
	function_called();
	return (int) mock();
}
 	
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_callBack1()
{
    struct lws *info = malloc(10);
    void *user = "connection";
    void *in = "payload";
    size_t size = 0;
    parodus_callback(info,LWS_CALLBACK_CLIENT_ESTABLISHED,user,in,size);
    if(!conn_retry)
    {
	    ParodusInfo("Success: conn_retry status changed %d\n",conn_retry);
	}
	free(info);    
}

void test_callBack2()
{
    struct lws *info = malloc(10);
    void *user = "connection";
    void *in = "payload";
    size_t size = 0;
    parodus_callback(info,LWS_CALLBACK_CLOSED,user,in,size);
    if(conn_retry)
    {
	    ParodusInfo("Success: conn_retry status changed %d\n",conn_retry);
	}
	free(info); 
}

void test_callBack3()
{
    struct lws *info = malloc(10);
    void *user = "connection";
    size_t size = 0;
	   
    void *bytes;
    const wrp_msg_t msg = { .msg_type = WRP_MSG_TYPE__REQ,
                            .u.req.transaction_uuid = "c07ee5e1-70be-444c-a156-097c767ad8aa",
                            .u.req.content_type = "application/json",
                            .u.req.source = "source-address",
                            .u.req.dest = "dest-address",
                            .u.req.partner_ids = NULL,
                            .u.req.headers = NULL,
                            .u.req.include_spans = false,
                            .u.req.spans.spans = NULL,
                            .u.req.spans.count = 0,
                            .u.req.payload = "Test libwebsocket reciever callback",
                            .u.req.payload_size = 35
};
	
	size = wrp_struct_to( &msg, WRP_BYTES, &bytes );
	will_return(lws_is_final_fragment,1);
    expect_function_call(lws_is_final_fragment);
    parodus_callback(info,LWS_CALLBACK_CLIENT_RECEIVE,user,bytes,size);
    free(bytes);
    free(info);
}

void test_callBack4()
{
    struct lws *info = malloc(10);
    void *user = "connection";
    size_t size = 0;
	   
    void *bytes;
    ParodusInfo( "\n Inside test_encode_decode()....\n" );
    const wrp_msg_t msg = { .msg_type = WRP_MSG_TYPE__REQ,
                            .u.req.transaction_uuid = "c07ee5e1-70be-444c-a156-097c767ad8aa",
                            .u.req.content_type = "application/json",
                            .u.req.source = "source-address",
                            .u.req.dest = "dest-address",
                            .u.req.partner_ids = NULL,
                            .u.req.headers = NULL,
                            .u.req.include_spans = false,
                            .u.req.spans.spans = NULL,
                            .u.req.spans.count = 0,
                            .u.req.payload = "Test libwebsocket reciever callback",
                            .u.req.payload_size = 35
};
	
	size = wrp_struct_to( &msg, WRP_BYTES, &bytes );
	will_return(lws_is_final_fragment,0);
    expect_function_call(lws_is_final_fragment);
    parodus_callback(info,LWS_CALLBACK_CLIENT_RECEIVE,user,bytes,size);
    will_return(lws_is_final_fragment,0);
    expect_function_call(lws_is_final_fragment);
    parodus_callback(info,LWS_CALLBACK_CLIENT_RECEIVE,user,bytes,size);
    will_return(lws_is_final_fragment,1);
    expect_function_call(lws_is_final_fragment);
    parodus_callback(info,LWS_CALLBACK_CLIENT_RECEIVE,user,bytes,size);
    free(bytes);
    free(info);
}

void test_callBack5()
{
    struct lws *info = malloc(10);
    void *user = "connection";
    void *in = "payload";
    size_t size = 0;
    parodus_callback(info,LWS_CALLBACK_CLIENT_CONNECTION_ERROR,user,in,size);
    if(conn_retry)
    {
	    ParodusInfo("Success: conn_retry status changed %d\n",conn_retry);
	}
	free(info);
}

void test_callBack6()
{
    struct lws *info = malloc(10);
    void *user = "connection";
    void *in = "payload";
    size_t size = 0;
    UpStreamMsg *message = (UpStreamMsg *)malloc(sizeof(UpStreamMsg));
    message->msg = (char*)malloc(sizeof(char)*10);
    strcpy(message->msg,"payload");
    message->len = 7;
    message->next = NULL;
    ResponseMsgQ = message;
    will_return(lws_write,7);
    expect_function_call(lws_write);
    parodus_callback(info,LWS_CALLBACK_CLIENT_WRITEABLE,user,in,size);
    free(message->msg);
    free(message);
    free(info);
}

void test_callBack7()
{
    struct lws *info = malloc(10);
    void *user = "connection";
    void *in = "payload";
    size_t size = 0;
    UpStreamMsg *message = (UpStreamMsg *)malloc(sizeof(UpStreamMsg));
    message->msg = (char*)malloc(sizeof(char)*10);
    strcpy(message->msg,"payload");
    message->len = 7;
    message->next = NULL;
    ResponseMsgQ = message;
    will_return(lws_write,-1);
    expect_function_call(lws_write);
    parodus_callback(info,LWS_CALLBACK_CLIENT_WRITEABLE,user,in,size);
    free(message->msg);
    free(message);
    free(info);
}

void test_callBack8()
{
    struct lws *info = malloc(10);
    void *user = "connection";
    void *in = "payload";
    size_t size = 0;
    UpStreamMsg *message = (UpStreamMsg *)malloc(sizeof(UpStreamMsg));
    message->msg = (char*)malloc(sizeof(char)*10);
    strcpy(message->msg,"payload");
    message->len = 7;
    message->next = NULL;
    ResponseMsgQ = message;
    will_return(lws_write,5);
    expect_function_call(lws_write);
    parodus_callback(info,LWS_CALLBACK_CLIENT_WRITEABLE,user,in,size);
    free(message->msg);
    free(message);
    free(info);
}

void test_callBack9()
{
    struct lws *info = malloc(10);
    void *user = "connection";
    void *in = "payload";
    size_t size = 0;
    will_return(getWebpaConveyHeader, (intptr_t)"WebPA-1.6 (TG1682)");
    expect_function_call(getWebpaConveyHeader);
    will_return(lws_add_http_header_by_name,0);
    expect_function_call(lws_add_http_header_by_name);
    will_return(lws_add_http_header_by_name,0);
    expect_function_call(lws_add_http_header_by_name);
    will_return(lws_add_http_header_by_name,0);
    expect_function_call(lws_add_http_header_by_name);
    will_return(lws_add_http_header_by_name,0);
    expect_function_call(lws_add_http_header_by_name);
    parodus_callback(info,LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER,user,in,size);
    free(info);
}

void test_callBack10()
{
    struct lws *info = malloc(10);
    void *user = "connection";
    void *in = "payload";
    size_t size = 0;
    will_return(getWebpaConveyHeader, (intptr_t)"");
    expect_function_call(getWebpaConveyHeader);
    will_return(lws_add_http_header_by_name,1);
    expect_function_call(lws_add_http_header_by_name);
    parodus_callback(info,LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER,user,in,size);
    free(info);
}
void test_callBack11()
{
    struct lws *info = malloc(10);
    void *user = "connection";
    void *in = "payload";
    size_t size = 0;
    will_return(getWebpaConveyHeader, (intptr_t)"");
    expect_function_call(getWebpaConveyHeader);
    will_return(lws_add_http_header_by_name,0);
    expect_function_call(lws_add_http_header_by_name);
    will_return(lws_add_http_header_by_name,1);
    expect_function_call(lws_add_http_header_by_name);
    parodus_callback(info,LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER,user,in,size);
    free(info);
}
void test_callBack12()
{
    struct lws *info = malloc(10);
    void *user = "connection";
    void *in = "payload";
    size_t size = 0;
    will_return(getWebpaConveyHeader, (intptr_t)"");
    expect_function_call(getWebpaConveyHeader);
    will_return(lws_add_http_header_by_name,0);
    expect_function_call(lws_add_http_header_by_name);
    will_return(lws_add_http_header_by_name,0);
    expect_function_call(lws_add_http_header_by_name);
    will_return(lws_add_http_header_by_name,1);
    expect_function_call(lws_add_http_header_by_name);
    parodus_callback(info,LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER,user,in,size);
    free(info);
}

void test_callBack13()
{
    struct lws *info = malloc(10);
    void *user = "connection";
    void *in = "payload";
    size_t size = 0;
    will_return(getWebpaConveyHeader, (intptr_t)"");
    expect_function_call(getWebpaConveyHeader);
    will_return(lws_add_http_header_by_name,0);
    expect_function_call(lws_add_http_header_by_name);
    will_return(lws_add_http_header_by_name,0);
    expect_function_call(lws_add_http_header_by_name);
    will_return(lws_add_http_header_by_name,0);
    expect_function_call(lws_add_http_header_by_name);
    parodus_callback(info,LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER,user,in,size);
    free(info);
}

void test_callBack14()
{
    struct lws *info = malloc(10);
    void *user = "connection";
    void *in = "payload";
    size_t size = 0;
    will_return(getWebpaConveyHeader, (intptr_t)"WebPA-1.6 (TG1682)");
    expect_function_call(getWebpaConveyHeader);
    will_return(lws_add_http_header_by_name,0);
    expect_function_call(lws_add_http_header_by_name);
    will_return(lws_add_http_header_by_name,0);
    expect_function_call(lws_add_http_header_by_name);
    will_return(lws_add_http_header_by_name,0);
    expect_function_call(lws_add_http_header_by_name);
    will_return(lws_add_http_header_by_name,1);
    expect_function_call(lws_add_http_header_by_name);
    parodus_callback(info,LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER,user,in,size);
    free(info);
}

void test_callBack15()
{
    struct lws *info = malloc(10);
    void *in = "payload";
    size_t size = 0;
    X509_STORE_CTX *ctx;
	ctx = X509_STORE_CTX_new();
    parodus_callback(info,LWS_CALLBACK_OPENSSL_PERFORM_SERVER_CERT_VERIFICATION,ctx,in,size);
    free(info);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_callBack1),
        cmocka_unit_test(test_callBack2),
        cmocka_unit_test(test_callBack3),
        cmocka_unit_test(test_callBack4),
        cmocka_unit_test(test_callBack5),
        cmocka_unit_test(test_callBack6),
        cmocka_unit_test(test_callBack7),
        cmocka_unit_test(test_callBack8),
        cmocka_unit_test(test_callBack9),
        cmocka_unit_test(test_callBack10),
        cmocka_unit_test(test_callBack11),
        cmocka_unit_test(test_callBack12),
        cmocka_unit_test(test_callBack13),
        cmocka_unit_test(test_callBack14),
        cmocka_unit_test(test_callBack15),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
