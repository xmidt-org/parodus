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
#include "../src/config.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

bool close_retry;
bool LastReasonStatus;
volatile unsigned int heartBeatTimer;
pthread_mutex_t close_mut;

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
noPollConn * nopoll_conn_tls_new (noPollCtx  * ctx, noPollConnOpts * options, const char * host_ip, const char * host_port, const char * host_name, const char * get_url, const char * protocols, const char * origin)
{
    UNUSED(options); UNUSED(host_port); UNUSED(host_name); UNUSED(get_url); UNUSED(protocols); 
    UNUSED(origin);
    
    function_called();
    check_expected((intptr_t)ctx);
    check_expected((intptr_t)host_ip);
    return (noPollConn *) (intptr_t)mock();
}

noPollConn * nopoll_conn_new (noPollCtx  * ctx, const char * host_ip, const char * host_port, const char * host_name, const char * get_url, const char * protocols, const char * origin)
{
    UNUSED(host_port); UNUSED(host_name); UNUSED(get_url); UNUSED(protocols); UNUSED(origin);
    
    function_called();
    check_expected((intptr_t)ctx);
    check_expected((intptr_t)host_ip);
    return (noPollConn *)(intptr_t)mock();
}

nopoll_bool nopoll_conn_is_ok (noPollConn * conn)
{
    UNUSED(conn);
    function_called();
    return (nopoll_bool) mock();
}

nopoll_bool nopoll_conn_wait_until_connection_ready (noPollConn * conn, int timeout, char * message)
{
    UNUSED(timeout); UNUSED(message);
    UNUSED(conn);
    function_called();
    return (nopoll_bool) mock();
}

char* getWebpaConveyHeader()
{
    function_called();
    return (char*) (intptr_t)mock();
}

int checkHostIp(char * serverIP)
{
    (void) serverIP;
    function_called();
    return (int) mock();
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

void nopoll_conn_close(noPollConn *conn)	
{
    UNUSED(conn);
    function_called();
}

int nopoll_conn_ref_count(noPollConn * conn)
{
    UNUSED(conn);
    function_called();
    return (int) mock();
}

void nopoll_conn_unref(	noPollConn * conn)	
{
    UNUSED(conn);
    function_called();
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    UNUSED(s1); UNUSED(s2); UNUSED(n);
    function_called();
    return (int) mock();
}

char *strtok(char *str, const char *delim)
{
    UNUSED(str); UNUSED(delim); 
    function_called();
    return (char*) (intptr_t)mock();
}

void setMessageHandlers()
{
    function_called();
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_createSecureConnection()
{
    noPollConn *gNPConn;
    noPollCtx *ctx = nopoll_ctx_new();
    ParodusCfg *cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    memset(cfg, 0, sizeof(ParodusCfg));
    
    cfg->secureFlag = 1;
    strcpy(cfg->webpa_url , "localhost");
    set_parodus_cfg(cfg);
    
    assert_non_null(ctx);
    will_return(getWebpaConveyHeader, (intptr_t)"WebPA-1.6 (TG1682)");
    expect_function_call(getWebpaConveyHeader);
    expect_value(nopoll_conn_tls_new, (intptr_t)ctx, (intptr_t)ctx);
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, "localhost");
    will_return(nopoll_conn_tls_new, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_tls_new);
    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);
    will_return(nopoll_conn_wait_until_connection_ready, nopoll_true);
    expect_function_call(nopoll_conn_wait_until_connection_ready);
    expect_function_call(setMessageHandlers);
    int ret = createNopollConnection(ctx);
    assert_int_equal(ret, nopoll_true);
    free(cfg);
    nopoll_ctx_unref (ctx);
}

void test_createConnection()
{
    noPollConn *gNPConn;
    noPollCtx *ctx = nopoll_ctx_new();
    ParodusCfg *cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    memset(cfg, 0, sizeof(ParodusCfg));
    assert_non_null(cfg);
    
    cfg->secureFlag = 0;
    strcpy(cfg->webpa_url , "localhost");
    set_parodus_cfg(cfg);
    assert_non_null(ctx);
    will_return(getWebpaConveyHeader, (intptr_t)"WebPA-1.6 (TG1682)");
    expect_function_call(getWebpaConveyHeader);
    expect_value(nopoll_conn_new, (intptr_t)ctx, (intptr_t)ctx);
    expect_string(nopoll_conn_new, (intptr_t)host_ip, "localhost");
    will_return(nopoll_conn_new, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_new);
    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);
    will_return(nopoll_conn_wait_until_connection_ready, nopoll_true);
    expect_function_call(nopoll_conn_wait_until_connection_ready);
    expect_function_call(setMessageHandlers);
    int ret = createNopollConnection(ctx);
    assert_int_equal(ret, nopoll_true);
    free(cfg);
    nopoll_ctx_unref (ctx);
}

void test_createConnectionConnNull()
{
    noPollConn *gNPConn;
    noPollCtx *ctx = nopoll_ctx_new();
    ParodusCfg *cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    memset(cfg, 0, sizeof(ParodusCfg));
    
    cfg->secureFlag = 1;
    cfg->webpa_backoff_max = 2;
    strcpy(cfg->webpa_url , "localhost");
    set_parodus_cfg(cfg);
    
    assert_non_null(ctx);
    will_return(getWebpaConveyHeader, (intptr_t)"");
    expect_function_call(getWebpaConveyHeader);
    expect_value(nopoll_conn_tls_new, (intptr_t)ctx, (intptr_t)ctx);
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, "localhost");
    will_return(nopoll_conn_tls_new, (intptr_t)NULL);
    expect_function_call(nopoll_conn_tls_new);
    will_return(checkHostIp, -2);
    expect_function_call(checkHostIp);
    expect_function_call(getCurrentTime);
    
    expect_value(nopoll_conn_tls_new, (intptr_t)ctx, (intptr_t)ctx);
    expect_string(nopoll_conn_tls_new,(intptr_t)host_ip, "localhost");
    will_return(nopoll_conn_tls_new, (intptr_t)NULL);
    expect_function_call(nopoll_conn_tls_new);
    will_return(checkHostIp, -2);
    expect_function_call(checkHostIp);
    expect_function_call(getCurrentTime);
    will_return(timeValDiff, 15*60*1000);
    expect_function_call(timeValDiff);
    will_return(timeValDiff, 15*60*1000);
    expect_function_call(timeValDiff);
    will_return(kill, 1);
    expect_function_call(kill);
    
    expect_value(nopoll_conn_tls_new, (intptr_t)ctx, (intptr_t)ctx);
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, "localhost");
    will_return(nopoll_conn_tls_new, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_tls_new);
    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);
    will_return(nopoll_conn_wait_until_connection_ready, nopoll_true);
    expect_function_call(nopoll_conn_wait_until_connection_ready);
    expect_function_call(setMessageHandlers);
    createNopollConnection(ctx);
    free(cfg);
    nopoll_ctx_unref (ctx);
}

void test_createConnectionConnNotOk()
{
    noPollConn *gNPConn;
    noPollCtx *ctx = nopoll_ctx_new();
    ParodusCfg *cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    memset(cfg, 0, sizeof(ParodusCfg));
    assert_non_null(cfg);
    
    cfg->secureFlag = 0;
    strcpy(cfg->webpa_url , "localhost");
    set_parodus_cfg(cfg);
    assert_non_null(ctx);
    will_return(getWebpaConveyHeader, (intptr_t)"WebPA-1.6 (TG1682)");
    expect_function_call(getWebpaConveyHeader);
    expect_value(nopoll_conn_new, (intptr_t)ctx, (intptr_t)ctx);
    expect_string(nopoll_conn_new, (intptr_t)host_ip, "localhost");
    will_return(nopoll_conn_new, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_new);
    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);
    expect_function_call(nopoll_conn_close);
    will_return(nopoll_conn_ref_count, 1);
    expect_function_call(nopoll_conn_ref_count);
    expect_function_call(nopoll_conn_unref);
    
    expect_value(nopoll_conn_new, (intptr_t)ctx, (intptr_t)ctx);
    expect_string(nopoll_conn_new, (intptr_t)host_ip, "localhost");
    will_return(nopoll_conn_new, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_new);
    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);
    will_return(nopoll_conn_wait_until_connection_ready, nopoll_false);
    expect_function_call(nopoll_conn_wait_until_connection_ready);
    will_return(strncmp, 12);
    expect_function_call(strncmp);
    expect_function_call(nopoll_conn_close);
    will_return(nopoll_conn_ref_count, 0);
    expect_function_call(nopoll_conn_ref_count);

    expect_value(nopoll_conn_new, (intptr_t)ctx, (intptr_t)ctx);
    expect_string(nopoll_conn_new, (intptr_t)host_ip, "localhost");
    will_return(nopoll_conn_new, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_new);
    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);
    will_return(nopoll_conn_wait_until_connection_ready, nopoll_false);
    expect_function_call(nopoll_conn_wait_until_connection_ready);
    will_return(strncmp, 0);
    expect_function_call(strncmp);
    will_return(strtok, (intptr_t)"");
    will_return(strtok, (intptr_t)"");
    will_return(strtok, (intptr_t)"p.10.0.0.12");
    will_return(strtok, (intptr_t)"8080");
    expect_function_calls(strtok, 4);
    expect_function_call(nopoll_conn_close);
    will_return(nopoll_conn_ref_count, 1);
    expect_function_call(nopoll_conn_ref_count);
    expect_function_call(nopoll_conn_unref);
    
    expect_value(nopoll_conn_new, (intptr_t)ctx, (intptr_t)ctx);
    expect_string(nopoll_conn_new, (intptr_t)host_ip, "10.0.0.12");
    will_return(nopoll_conn_new, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_new);
    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);
    will_return(nopoll_conn_wait_until_connection_ready, nopoll_true);
    expect_function_call(nopoll_conn_wait_until_connection_ready);
    expect_function_call(setMessageHandlers);
    int ret = createNopollConnection(ctx);
    assert_int_equal(ret, nopoll_true);
    free(cfg);
    nopoll_ctx_unref (ctx);
}

void err_createConnectionCtxNull()
{
    noPollCtx *ctx = NULL;
    assert_null(ctx);
    int ret = createNopollConnection(ctx);
    assert_int_equal(ret, nopoll_false);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_createSecureConnection),
        cmocka_unit_test(test_createConnection),
        cmocka_unit_test(test_createConnectionConnNull),
        cmocka_unit_test(test_createConnectionConnNotOk),
        cmocka_unit_test(err_createConnectionCtxNull),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
