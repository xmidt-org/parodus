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
#include <stddef.h>
#include <stdbool.h>
#include <setjmp.h>
#include <cmocka.h>
#include <nopoll.h>

#include "../src/parodus_log.h"
#include "../src/nopoll_helpers.h"
#include "../src/config.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define UNUSED(x) (void )(x)
#define MAX_SEND_SIZE (60 * 1024)
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
 static noPollConn *conn = NULL;
 static ParodusCfg cfg;
 
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
nopoll_bool nopoll_conn_is_ok( noPollConn *conn )
{
    ParodusInfo("function_called : %s\n",__FUNCTION__);
    function_called();
	check_expected((intptr_t)conn);
	
    return (nopoll_bool)mock();
}

nopoll_bool nopoll_conn_is_ready( noPollConn *conn )
{
    ParodusInfo("function_called : %s\n",__FUNCTION__);
    function_called();
	check_expected((intptr_t)conn);
	
    return (nopoll_bool)mock();
}

ParodusCfg *get_parodus_cfg(void)
{
    cfg.cloud_status = CLOUD_STATUS_ONLINE;
    return &cfg;
}

int  __nopoll_conn_send_common (noPollConn * conn, const char * content, long length, nopoll_bool  has_fin, long       sleep_in_header, noPollOpCode frame_type)
{
    UNUSED(has_fin); UNUSED(sleep_in_header); UNUSED(frame_type); UNUSED(content);
    ParodusInfo("function_called : %s\n",__FUNCTION__);
    function_called();
    
    check_expected((intptr_t)conn);
    check_expected(length);
    return (int)mock();
}

int nopoll_conn_flush_writes(noPollConn * conn, long timeout, int previous_result)
{
    UNUSED(timeout);
    
    ParodusInfo("function_called : %s\n",__FUNCTION__);
    function_called();
    
    check_expected((intptr_t)conn);
    check_expected(previous_result);
    return (int)mock();
}

noPollConn *get_global_conn()
{
     return conn;   
}

void listenerOnMessage_queue(noPollCtx * ctx, noPollConn * conn, noPollMsg * msg,noPollPtr user_data)
{
    UNUSED(ctx); UNUSED(conn); UNUSED(msg); UNUSED(user_data);
}

void listenerOnPingMessage (noPollCtx * ctx, noPollConn * conn, noPollMsg * msg, noPollPtr user_data)
{
    UNUSED(ctx); UNUSED(conn); UNUSED(msg); UNUSED(user_data);
}

void listenerOnCloseMessage (noPollCtx * ctx, noPollConn * conn, noPollPtr user_data)
{
    UNUSED(ctx); UNUSED(conn); UNUSED(user_data);
}

void getCurrentTime(struct timespec *timer)
{
    (void) timer;
    function_called();
}

long timeValDiff(struct timespec *starttime, struct timespec *finishtime)
{
    (void) starttime; (void) finishtime;
    function_called();
    return (long) mock();
}

int kill(pid_t pid, int sig)
{
    UNUSED(pid); UNUSED(sig);
    function_called();
    return (int) mock();
}

bool get_interface_down_event()
{
     return false;
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_setMessageHandlers()
{
    setMessageHandlers();
}

void test_sendResponse()
{
    int len = strlen("Hello Parodus!");
    expect_value(__nopoll_conn_send_common, (intptr_t)conn, (intptr_t)conn);
    expect_value(__nopoll_conn_send_common, length,(intptr_t) len);
    will_return(__nopoll_conn_send_common, (intptr_t)len);
    expect_function_calls(__nopoll_conn_send_common, 1);
    
    int bytesWritten = sendResponse(conn, "Hello Parodus!", len);
    ParodusPrint("Number of bytes written: %d\n", bytesWritten);
    assert_int_equal(bytesWritten, len);
}

void test_sendResponseWithFragments()
{
    int len = (MAX_SEND_SIZE*2)+64;
    
    expect_value(__nopoll_conn_send_common, (intptr_t)conn, (intptr_t)conn);
    expect_value(__nopoll_conn_send_common, length, MAX_SEND_SIZE);
    will_return(__nopoll_conn_send_common, MAX_SEND_SIZE);
    expect_value(__nopoll_conn_send_common, (intptr_t)conn, (intptr_t)conn);
    expect_value(__nopoll_conn_send_common, length, MAX_SEND_SIZE);
    will_return(__nopoll_conn_send_common, MAX_SEND_SIZE);
    expect_value(__nopoll_conn_send_common, (intptr_t)conn, (intptr_t)conn);
    expect_value(__nopoll_conn_send_common, length, 64);
    will_return(__nopoll_conn_send_common, 64);
    expect_function_calls(__nopoll_conn_send_common, 3);
    int bytesWritten = sendResponse(conn, "Hello Parodus!", len);
    ParodusPrint("Number of bytes written: %d\n", bytesWritten);
    assert_int_equal(bytesWritten, len);
}

void err_sendResponse()
{
    int len = strlen("Hello Parodus!");
    
    expect_value(__nopoll_conn_send_common, (intptr_t)conn, (intptr_t)conn);
    expect_value(__nopoll_conn_send_common, length, (intptr_t)len);
    will_return(__nopoll_conn_send_common, -1);
    expect_function_calls(__nopoll_conn_send_common, 1);
    
    int bytesWritten = sendResponse(conn, "Hello Parodus!", len);
    ParodusPrint("Number of bytes written: %d\n", bytesWritten);
    assert_int_equal(bytesWritten, 0);   
}

void err_sendResponseFlushWrites()
{
    int len = strlen("Hello Parodus!");
    
    expect_value(__nopoll_conn_send_common, (intptr_t)conn, (intptr_t)conn);
    expect_value(__nopoll_conn_send_common, length, len);
    will_return(__nopoll_conn_send_common, len-3);
    expect_function_calls(__nopoll_conn_send_common, 1);
    
    expect_value(nopoll_conn_flush_writes, (intptr_t)conn, (intptr_t)conn);
    expect_value(nopoll_conn_flush_writes, previous_result, len-3);
    will_return(nopoll_conn_flush_writes, len-3);
    expect_function_calls(nopoll_conn_flush_writes, 1);
    
    int bytesWritten = sendResponse(conn, "Hello Parodus!", len);
    ParodusPrint("Number of bytes written: %d\n", bytesWritten);
    assert_int_equal(bytesWritten, 0);   
}

void err_sendResponseConnNull()
{
    int len = strlen("Hello Parodus!");
    expect_value(__nopoll_conn_send_common, (intptr_t)conn, (intptr_t)NULL);
    expect_value(__nopoll_conn_send_common, length, len);
    will_return(__nopoll_conn_send_common, -1);
    expect_function_calls(__nopoll_conn_send_common, 1);
    
    int bytesWritten = sendResponse(NULL, "Hello Parodus!", len);
    ParodusPrint("Number of bytes written: %d\n", bytesWritten);
    assert_int_equal(bytesWritten, 0);   
}

void test_sendMessage()
{
    int len = strlen("Hello Parodus!");
    
    expect_value(__nopoll_conn_send_common, (intptr_t)conn, (intptr_t)conn);
    expect_value(__nopoll_conn_send_common, length, len);
    will_return(__nopoll_conn_send_common, len);
    expect_function_calls(__nopoll_conn_send_common, 1);
    
    sendMessage(conn, "Hello Parodus!", len);
}

void err_sendMessage()
{
    int len = strlen("Hello Parodus!");
    
    expect_value(__nopoll_conn_send_common, (intptr_t)conn,(intptr_t) conn);
    expect_value(__nopoll_conn_send_common, length, len);
    will_return(__nopoll_conn_send_common, len-2);
    expect_function_calls(__nopoll_conn_send_common, 1);
    
    expect_value(nopoll_conn_flush_writes, (intptr_t)conn, (intptr_t)conn);
    expect_value(nopoll_conn_flush_writes, previous_result, len-2);
    will_return(nopoll_conn_flush_writes, len-3);
    expect_function_calls(nopoll_conn_flush_writes, 1);
    
    sendMessage(conn, "Hello Parodus!", len);
}

void err_sendMessageConnNull()
{
    int len = strlen("Hello Parodus!");
    
    expect_value(__nopoll_conn_send_common, (intptr_t)conn, NULL);
    expect_value(__nopoll_conn_send_common, length, len);
    will_return(__nopoll_conn_send_common, len);
    expect_function_calls(__nopoll_conn_send_common, 1);

    sendMessage(NULL, "Hello Parodus!", len);
}

void test_reportLog()
{
    __report_log(NULL, NOPOLL_LEVEL_DEBUG, "Debug", NULL);
    __report_log(NULL, NOPOLL_LEVEL_INFO, "Info", NULL);
    __report_log(NULL, NOPOLL_LEVEL_WARNING, "Warning", NULL);
    __report_log(NULL, NOPOLL_LEVEL_CRITICAL, "Critical", NULL);
}
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_setMessageHandlers),
        cmocka_unit_test(test_sendResponse),
        cmocka_unit_test(test_sendResponseWithFragments),
        cmocka_unit_test(err_sendResponse),
        cmocka_unit_test(err_sendResponseFlushWrites),
        cmocka_unit_test(err_sendResponseConnNull),
        cmocka_unit_test(test_sendMessage),
        cmocka_unit_test(err_sendMessage),
        cmocka_unit_test(err_sendMessageConnNull),
        cmocka_unit_test(test_reportLog),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
