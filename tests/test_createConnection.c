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

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/


struct lws_context* lws_create_context ( struct lws_context_creation_info *  info)
{
	function_called();
	return (struct lws_context *) (intptr_t)mock();	
}	

struct lws* lws_client_connect_via_info (struct lws_client_connect_info *  	ccinfo)
{
	function_called();
	return (struct lws *) (intptr_t)mock();
}

int lws_service(struct lws_context * context,int timeout)
{
    UNUSED(context); UNUSED(timeout);
    function_called();
    return (int) mock();
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_createSecureConnection()
{
    ParodusCfg *cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    memset(cfg, 0, sizeof(ParodusCfg));
    struct lws_context *lwscontext;
    struct lws * info;
    conn_retry = false;
    cfg->secureFlag = 1;
    strcpy(cfg->webpa_url , "fabric.webpa.comcast.net");
    set_parodus_cfg(cfg);
    will_return(lws_create_context, (intptr_t)&lwscontext);
    expect_function_call(lws_create_context);
    will_return(lws_client_connect_via_info, (intptr_t)&info);
    expect_function_call(lws_client_connect_via_info);
    will_return(lws_service, 1);
    expect_function_call(lws_service);
    createLWSconnection();
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_createSecureConnection),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
