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
#include <nopoll.h>

#include "../src/ParodusInternal.h"
#include "../src/connection.h"
#include "wrp-c.h"

#define MAX_SEND_SIZE (60 * 1024)

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_sendResponse()
{
    int len = strlen("Hello Parodus!");
    expect_value(__nopoll_conn_send_common, conn, get_global_conn());
    expect_value(__nopoll_conn_send_common, length, len);
    will_return(__nopoll_conn_send_common, len);
    expect_function_calls(__nopoll_conn_send_common, 1);
    
    int bytesWritten = sendResponse(get_global_conn(), "Hello Parodus!", len);
    ParodusPrint("Number of bytes written: %d\n", bytesWritten);
    assert_int_equal(bytesWritten, len);
}

void test_sendResponseWithFragments()
{
    int len = (MAX_SEND_SIZE*2)+64;
    expect_value(__nopoll_conn_send_common, conn, get_global_conn());
    expect_value(__nopoll_conn_send_common, length, MAX_SEND_SIZE);
    will_return(__nopoll_conn_send_common, MAX_SEND_SIZE);
    expect_value(__nopoll_conn_send_common, conn, get_global_conn());
    expect_value(__nopoll_conn_send_common, length, MAX_SEND_SIZE);
    will_return(__nopoll_conn_send_common, MAX_SEND_SIZE);
    expect_value(__nopoll_conn_send_common, conn, get_global_conn());
    expect_value(__nopoll_conn_send_common, length, 64);
    will_return(__nopoll_conn_send_common, 64);
    expect_function_calls(__nopoll_conn_send_common, 3);
    int bytesWritten = sendResponse(get_global_conn(), "Hello Parodus!", len);
    ParodusPrint("Number of bytes written: %d\n", bytesWritten);
    assert_int_equal(bytesWritten, len);
}

void err_sendResponse()
{
    int len = strlen("Hello Parodus!");
    expect_value(__nopoll_conn_send_common, conn, get_global_conn());
    expect_value(__nopoll_conn_send_common, length, len);
    will_return(__nopoll_conn_send_common, -1);
    expect_function_calls(__nopoll_conn_send_common, 1);
    
    int bytesWritten = sendResponse(get_global_conn(), "Hello Parodus!", len);
    ParodusPrint("Number of bytes written: %d\n", bytesWritten);
    assert_int_equal(bytesWritten, 0);   
}

void err_sendResponseFlushWrites()
{
    int len = strlen("Hello Parodus!");
    expect_value(__nopoll_conn_send_common, conn, get_global_conn());
    expect_value(__nopoll_conn_send_common, length, len);
    will_return(__nopoll_conn_send_common, len-3);
    expect_function_calls(__nopoll_conn_send_common, 1);
    
    expect_value(nopoll_conn_flush_writes, conn, get_global_conn());
    expect_value(nopoll_conn_flush_writes, previous_result, len-3);
    will_return(nopoll_conn_flush_writes, len-3);
    expect_function_calls(nopoll_conn_flush_writes, 1);
    
    int bytesWritten = sendResponse(get_global_conn(), "Hello Parodus!", len);
    ParodusPrint("Number of bytes written: %d\n", bytesWritten);
    assert_int_equal(bytesWritten, 0);   
}
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_sendResponse),
        cmocka_unit_test(test_sendResponseWithFragments),
        cmocka_unit_test(err_sendResponse),
        cmocka_unit_test(err_sendResponseFlushWrites),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
