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
UpStreamMsg *UpStreamMsgQ;
ParodusMsg *ParodusMsgQ;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
void __report_log (noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data)
{
    UNUSED(ctx); UNUSED(level); UNUSED(log_msg); UNUSED(user_data);
}

void packMetaData(){
}

void setMessageHandlers(){
}

void getParodusUrl(){
}

void *handle_upstream(){
    return NULL;
}

void *processUpstreamMessage(){
    return NULL;
}
void *messageHandlerTask(){
    return NULL;
}
void *serviceAliveTask(){
    return NULL;
}
char* getWebpaConveyHeader()
{
    return "";
}
int checkHostIp(char * serverIP)
{
   (void) serverIP;
   return 0;
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
    expect_value(nopoll_conn_tls_new, ctx, ctx);
    expect_string(nopoll_conn_tls_new, host_ip, "localhost");
    will_return(nopoll_conn_tls_new, &gNPConn);
    expect_function_call(nopoll_conn_tls_new);
    expect_function_call(nopoll_conn_wait_until_connection_ready);
    int ret = createNopollConnection(ctx);
    
    assert_int_equal(ret, nopoll_true);
    assert_non_null(get_global_conn());
    assert_true(nopoll_conn_is_ok(get_global_conn()));
    free(cfg);
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
    
    expect_value(nopoll_conn_new, ctx, ctx);
    expect_string(nopoll_conn_new, host_ip, "localhost");
    will_return(nopoll_conn_new, &gNPConn);
    expect_function_call(nopoll_conn_new);
    expect_function_call(nopoll_conn_wait_until_connection_ready);
    int ret = createNopollConnection(ctx);
    assert_int_equal(ret, nopoll_true);
    assert_non_null(get_global_conn());
    assert_true(nopoll_conn_is_ok(get_global_conn()));
    free(cfg);
}

void err_createConnection()
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
        cmocka_unit_test(err_createConnection),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
