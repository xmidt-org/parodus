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

#include "../src/ParodusInternal.h"
#include "../src/conn_interface.h"
#include "../src/connection.h"
#include "../src/config.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
UpStreamMsg *UpStreamMsgQ;
ParodusMsg *ParodusMsgQ;
extern bool conn_retry;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

void createLWSconnection()
{
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

int lws_service(struct lws_context * context,int timeout)
{
    UNUSED(context); UNUSED(timeout);
    function_called();
    return (int) mock();
}

void set_global_reconnect_reason(char *reason)
{
    UNUSED(reason);
    function_called();
}

void lws_context_destroy(struct lws_context * context)	
{
    UNUSED(context);
    function_called();
}

struct lws_context *get_global_context(void)
{
    function_called();
    return (struct lws_context *) (intptr_t)mock();
}

void set_global_context(struct lws_context *contextRef)
{
    UNUSED(contextRef);
    function_called();
}

void StartThread(void *(*start_routine) (void *))
{
    UNUSED(start_routine);
    function_called(); 
}

void initKeypress()
{
    function_called();
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_createLWSsocket()
{
    ParodusCfg cfg;
    memset(&cfg,0,sizeof(ParodusCfg));
    
    conn_retry = true;
    expect_function_call(createLWSconnection);
    
    expect_function_call(packMetaData);

    expect_function_calls(StartThread, 4);
    expect_function_call(initKeypress);
    will_return(get_global_context, NULL);
    expect_function_call(get_global_context);
    will_return(lws_service, 1);
    expect_function_call(lws_service);
    expect_function_call(createLWSconnection);
    will_return(get_global_context, NULL);
    expect_function_call(get_global_context);
    expect_function_call(lws_context_destroy);
    createLWSsocket(&cfg,initKeypress);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_createLWSsocket),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
