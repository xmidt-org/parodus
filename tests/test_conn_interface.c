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

#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <assert.h>
#include <nopoll.h>

#include "../src/ParodusInternal.h"
#include "../src/conn_interface.h"
#include "../src/connection.h"
#include "../src/config.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
UpStreamMsg *UpStreamMsgQ;
ParodusMsg *ParodusMsgQ;
extern bool close_retry;
extern pthread_mutex_t close_mut;
extern volatile unsigned int heartBeatTimer;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
int createNopollConnection(noPollCtx *ctx)
{
    UNUSED(ctx);
    function_called();
    return (int) mock();
}

void nopoll_log_set_handler	(noPollCtx *ctx, noPollLogHandler handler, noPollPtr user_data)
{
    UNUSED(ctx); UNUSED(handler); UNUSED(user_data);
    function_called(); 
}

void __report_log (noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data)
{
    UNUSED(ctx); UNUSED(level); UNUSED(log_msg); UNUSED(user_data);
    function_called(); 
}

void nopoll_thread_handlers	(	noPollMutexCreate 	mutex_create,
noPollMutexDestroy 	mutex_destroy,
noPollMutexLock 	mutex_lock,
noPollMutexUnlock 	mutex_unlock 
)
{
    UNUSED(mutex_create); UNUSED(mutex_destroy); UNUSED(mutex_lock); UNUSED(mutex_unlock);
    function_called();
}
void packMetaData()
{
    function_called();
}



void *handle_upstream()
{
    return NULL;
}

void *processUpstreamMessage()
{
    return NULL;
}

void *messageHandlerTask()
{
    return NULL;
}

void *serviceAliveTask()
{
    return NULL;
}

int nopoll_loop_wait(noPollCtx * ctx,long timeout)
{
    UNUSED(ctx); UNUSED(timeout);
    function_called();
    return (int) mock();
}

void set_global_reconnect_reason(char *reason)
{
    UNUSED(reason);
    function_called();
}

void set_global_reconnect_status(bool status)
{
    (void)status;
    function_called();
}

void close_and_unref_connection(noPollConn *conn)
{
    UNUSED(conn);
    function_called();
}

void nopoll_cleanup_library	()	
{
    function_called();
}

void nopoll_ctx_unref(noPollCtx * ctx)
{
    UNUSED(ctx);
    function_called();
}

noPollConn *get_global_conn(void)
{
    function_called();
    return (noPollConn *) (intptr_t)mock();
}

void set_global_conn(noPollConn *conn)
{
    UNUSED(conn);
    function_called();
}

void StartThread(void *(*start_routine) (void *))
{
    UNUSED(start_routine);
    function_called(); 
}

noPollCtx* nopoll_ctx_new(void)
{
    function_called();
    return (noPollCtx*) (intptr_t)mock();
}
void initKeypress()
{
    function_called();
}

void sendToAllRegisteredClients(void **resp_bytes, size_t resp_size)
{
    UNUSED(resp_bytes); UNUSED(resp_size);
}

cJSON* get_Client_Subscriptions(char *service_name)
{
    UNUSED(service_name);
    return NULL;
}

bool add_Client_Subscription(char *service_name, char *regex)
{
    UNUSED(service_name);
    UNUSED(regex);
    return true;
}

void init_subscription_list()
{
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_createSocketConnection()
{
    noPollCtx *ctx;
    ParodusCfg cfg;
    memset(&cfg,0,sizeof(ParodusCfg));
    
    pthread_mutex_lock (&close_mut);
    close_retry = false;
    pthread_mutex_unlock (&close_mut);
    expect_function_call(nopoll_thread_handlers);
    
    will_return(nopoll_ctx_new, (intptr_t)&ctx);
    expect_function_call(nopoll_ctx_new);
    expect_function_call(nopoll_log_set_handler);
    will_return(createNopollConnection, nopoll_true);
    expect_function_call(createNopollConnection);
    expect_function_call(packMetaData);

    expect_function_calls(StartThread, 5);
    expect_function_call(initKeypress);
    will_return(nopoll_loop_wait, 1);
    expect_function_call(nopoll_loop_wait);
    
    expect_function_call(set_global_reconnect_reason);
    will_return(get_global_conn, (intptr_t)NULL);
    expect_function_call(set_global_reconnect_status);
    expect_function_call(get_global_conn);
    expect_function_call(close_and_unref_connection);
    expect_function_call(set_global_conn);
    will_return(createNopollConnection, nopoll_true);
    expect_function_call(createNopollConnection);
    will_return(get_global_conn, (intptr_t)NULL);
    expect_function_call(get_global_conn);
    expect_function_call(close_and_unref_connection);
    expect_function_call(nopoll_ctx_unref);
    expect_function_call(nopoll_cleanup_library);
    createSocketConnection(initKeypress);
}

void test_createSocketConnection1()
{
    noPollCtx *ctx;
    ParodusCfg cfg;
    memset(&cfg,0, sizeof(ParodusCfg));
    pthread_mutex_lock (&close_mut);
    close_retry = true;
    pthread_mutex_unlock (&close_mut);
    expect_function_call(nopoll_thread_handlers);
    
    will_return(nopoll_ctx_new, (intptr_t)&ctx);
    expect_function_call(nopoll_ctx_new);
    expect_function_call(nopoll_log_set_handler);
    will_return(createNopollConnection, nopoll_true);
    expect_function_call(createNopollConnection);
    expect_function_call(packMetaData);

    expect_function_calls(StartThread, 5);
    will_return(nopoll_loop_wait, 1);
    expect_function_call(nopoll_loop_wait);
    
    will_return(get_global_conn, (intptr_t)NULL);
    expect_function_call(get_global_conn);
    expect_function_call(close_and_unref_connection);
    expect_function_call(set_global_conn);
    will_return(createNopollConnection, nopoll_true);
    expect_function_call(createNopollConnection);
    will_return(get_global_conn, (intptr_t)NULL);
    expect_function_call(get_global_conn);
    expect_function_call(close_and_unref_connection);
    expect_function_call(nopoll_ctx_unref);
    expect_function_call(nopoll_cleanup_library);
    createSocketConnection(NULL);
    
}

void test_createSocketConnection2()
{
    noPollCtx *ctx;
    ParodusCfg cfg;
    memset(&cfg,0,sizeof(ParodusCfg));
    parStrncpy(cfg.hw_model, "TG1682", sizeof(cfg.hw_model));
    parStrncpy(cfg.hw_serial_number, "Fer23u948590", sizeof(cfg.hw_serial_number));
    parStrncpy(cfg.hw_manufacturer , "ARRISGroup,Inc.", sizeof(cfg.hw_manufacturer));
    parStrncpy(cfg.hw_mac , "123567892366", sizeof(cfg.hw_mac));
    parStrncpy(cfg.hw_last_reboot_reason , "unknown", sizeof(cfg.hw_last_reboot_reason));
    parStrncpy(cfg.fw_name , "2.364s2", sizeof(cfg.fw_name));
    parStrncpy(cfg.webpa_path_url , "/v1", sizeof(cfg.webpa_path_url));
    parStrncpy(cfg.webpa_url , "localhost", sizeof(cfg.webpa_url));
    parStrncpy(cfg.webpa_interface_used , "eth0", sizeof(cfg.webpa_interface_used));
    parStrncpy(cfg.webpa_protocol , "WebPA-1.6", sizeof(cfg.webpa_protocol));
    parStrncpy(cfg.webpa_uuid , "1234567-345456546", sizeof(cfg.webpa_uuid));
    cfg.webpa_ping_timeout = 1;
    set_parodus_cfg(&cfg);
    
    pthread_mutex_lock (&close_mut);
    close_retry = false;
    pthread_mutex_unlock (&close_mut);
    expect_function_call(nopoll_thread_handlers);
    
    will_return(nopoll_ctx_new, (intptr_t)&ctx);
    expect_function_call(nopoll_ctx_new);
    expect_function_call(nopoll_log_set_handler);
    will_return(createNopollConnection, nopoll_true);
    expect_function_call(createNopollConnection);
    expect_function_call(packMetaData);

    expect_function_calls(StartThread, 5);
    will_return(nopoll_loop_wait, 1);
    will_return(nopoll_loop_wait, 1);
    will_return(nopoll_loop_wait, 1);
    will_return(nopoll_loop_wait, 1);
    will_return(nopoll_loop_wait, 1);
    will_return(nopoll_loop_wait, 1);
    will_return(nopoll_loop_wait, 1);
    expect_function_calls(nopoll_loop_wait, 7);
    
    expect_function_call(set_global_reconnect_reason);
    will_return(get_global_conn, (intptr_t)NULL);
    expect_function_call(set_global_reconnect_status);
    expect_function_call(get_global_conn);
    expect_function_call(close_and_unref_connection);
    expect_function_call(set_global_conn);
    will_return(createNopollConnection, nopoll_true);
    expect_function_call(createNopollConnection);
    will_return(get_global_conn, (intptr_t)NULL);
    expect_function_call(get_global_conn);
    expect_function_call(close_and_unref_connection);
    expect_function_call(nopoll_ctx_unref);
    expect_function_call(nopoll_cleanup_library);
    createSocketConnection(NULL);
}

void err_createSocketConnection()
{
    pthread_mutex_lock (&close_mut);
    close_retry = true;
    pthread_mutex_unlock (&close_mut);
    heartBeatTimer = 0;
    expect_function_call(nopoll_thread_handlers);
    
    will_return(nopoll_ctx_new, (intptr_t)NULL);
    expect_function_call(nopoll_ctx_new);
    expect_function_call(nopoll_log_set_handler);
    will_return(createNopollConnection, nopoll_true);
    expect_function_call(createNopollConnection);
    expect_function_call(packMetaData);

    expect_function_calls(StartThread, 5);
    will_return(nopoll_loop_wait, 1);
    expect_function_call(nopoll_loop_wait);
    
    will_return(get_global_conn, (intptr_t)NULL);
    expect_function_call(get_global_conn);
    expect_function_call(close_and_unref_connection);
    expect_function_call(set_global_conn);
    will_return(createNopollConnection, nopoll_true);
    expect_function_call(createNopollConnection);
    will_return(get_global_conn, (intptr_t)NULL);
    expect_function_call(get_global_conn);
    expect_function_call(close_and_unref_connection);
    expect_function_call(nopoll_ctx_unref);
    expect_function_call(nopoll_cleanup_library);
    createSocketConnection(NULL);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_createSocketConnection),
        cmocka_unit_test(test_createSocketConnection1),
        cmocka_unit_test(test_createSocketConnection2),
        cmocka_unit_test(err_createSocketConnection),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
