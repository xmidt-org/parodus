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
#include "../src/heartBeat.h"
#include "../src/close_retry.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
UpStreamMsg *UpStreamMsgQ;
ParodusMsg *ParodusMsgQ;
pthread_mutex_t g_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_cond=PTHREAD_COND_INITIALIZER;
pthread_mutex_t nano_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t nano_con=PTHREAD_COND_INITIALIZER;
pthread_mutex_t svc_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t svc_con=PTHREAD_COND_INITIALIZER;

 
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

void start_conn_in_progress (void)
{
}   

void stop_conn_in_progress (void)
{
}   

void reset_interface_down_event (void)
{
}

void packMetaData()
{
    function_called();
}


int get_cloud_disconnect_time(void)
{
    function_called();
    return (int) (intptr_t)mock();
}

void set_cloud_disconnect_time(int Time)
{
    UNUSED(Time);
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

int serviceAliveTask()
{
    return 0;
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

void StartThread(void *(*start_routine) (void *), pthread_t *threadId)
{
    UNUSED(start_routine);
    UNUSED(threadId);
    function_called(); 
}

void JoinThread (pthread_t threadId)
{
    UNUSED(threadId);
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

UpStreamMsg * get_global_UpStreamMsgQ(void)
{
    return UpStreamMsgQ;
}

void set_global_UpStreamMsgQ(UpStreamMsg * UpStreamQ)
{
    UpStreamMsgQ = UpStreamQ;
}

pthread_cond_t *get_global_nano_con(void)
{
    return &nano_con;
}

pthread_mutex_t *get_global_nano_mut(void)
{
    return &nano_mut;
}

pthread_cond_t *get_global_svc_con(void)
{
    return &svc_con;
}

pthread_mutex_t *get_global_svc_mut(void)
{
    return &svc_mut;
}

/*
* Mock func to calculate time diff between start and stop time
* This timespec_diff retuns 1 sec as diff time
*/
void timespec_diff(struct timespec *start, struct timespec *stop,
                   struct timespec *diff)
{
   UNUSED(start);
   UNUSED(stop);
   diff->tv_sec = 1;
   diff->tv_nsec = 1000;
}

void deleteAllClients (void)
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
    reset_close_retry();
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
    set_close_retry();
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

void test_PingMissIntervalTime()
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
    //Max ping timeout is 6 sec
    cfg.webpa_ping_timeout = 6;
    set_parodus_cfg(&cfg);
    
    reset_close_retry();
    expect_function_call(nopoll_thread_handlers);
    
    will_return(nopoll_ctx_new, (intptr_t)&ctx);
    expect_function_call(nopoll_ctx_new);
    expect_function_call(nopoll_log_set_handler);
    will_return(createNopollConnection, nopoll_true);
    expect_function_call(createNopollConnection);
    expect_function_call(packMetaData);

    expect_function_calls(StartThread, 5);
    //Increment ping interval time to 1 sec for each nopoll_loop_wait call
    will_return(nopoll_loop_wait, 1);
    will_return(nopoll_loop_wait, 1);
    will_return(nopoll_loop_wait, 1);
    will_return(nopoll_loop_wait, 1);
    will_return(nopoll_loop_wait, 1);
    will_return(nopoll_loop_wait, 1);
    will_return(nopoll_loop_wait, 1);
    expect_function_calls(nopoll_loop_wait, 7);
    //Expecting Ping miss after 6 sec
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
    set_close_retry();
    reset_heartBeatTimer();
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


void test_createSocketConnection_cloud_disconn()
{
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(ParodusCfg));
	cfg.cloud_disconnect = strdup("XPC");
	set_parodus_cfg(&cfg);

	set_close_retry();
	reset_heartBeatTimer();
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


	expect_function_call(set_cloud_disconnect_time);
	will_return(get_cloud_disconnect_time, 0);
	expect_function_call(get_cloud_disconnect_time);
	will_return(get_cloud_disconnect_time, 0);
	expect_function_call(get_cloud_disconnect_time);
	will_return(get_cloud_disconnect_time, 0);
	expect_function_call(get_cloud_disconnect_time);

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
        cmocka_unit_test(test_PingMissIntervalTime),
        cmocka_unit_test(err_createSocketConnection),
        cmocka_unit_test(test_createSocketConnection_cloud_disconn)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
