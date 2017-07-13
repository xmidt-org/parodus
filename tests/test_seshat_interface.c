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
#include "../src/seshat_interface.h"
#include "../src/config.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define URL "url"
#define LRU "lru"

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
ParodusCfg g_config;

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
ParodusCfg *get_parodus_cfg(void)
{
    return &g_config;
}

void loadParodusCfg(ParodusCfg *config, ParodusCfg *cfg)
{
    UNUSED(config); UNUSED(cfg);
}

void parseCommandLine(int argc,char **argv, ParodusCfg *cfg)
{
    UNUSED(argc); UNUSED(argv); UNUSED(cfg);
}

void set_parodus_cfg(ParodusCfg *cfg)
{
    UNUSED(cfg);
}

int init_lib_seshat (const char *url)
{
    UNUSED(url);
    function_called();
    return (int) mock();
}

int shutdown_seshat_lib (void)
{
    function_called();
    return (int) mock();
}

int seshat_register(const char *service, const char *url)
{
    UNUSED(service); UNUSED(url);
    function_called();
    return (int) mock();
}

char* seshat_discover(const char *service)
{
    UNUSED(service);
    function_called();
    return (char *) mock();
}

int allow_insecure_conn(void)
{
    return 0;
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_all_pass()
{
    strcpy(g_config.local_url, URL);

    will_return(init_lib_seshat, 0);
    expect_function_call(init_lib_seshat);

    will_return(seshat_register, 0);
    expect_function_call(seshat_register);

    char *d_url = malloc(sizeof(g_config.local_url));
    strcpy(d_url, g_config.local_url);
    will_return(seshat_discover, d_url);
    expect_function_call(seshat_discover);

    will_return(shutdown_seshat_lib, 0);
    expect_function_call(shutdown_seshat_lib);

    assert_true(__registerWithSeshat());
    memset(&g_config, '\0', sizeof(g_config));
}

void test_init_fail()
{
    will_return(init_lib_seshat, -1);
    expect_function_call(init_lib_seshat);

    will_return(shutdown_seshat_lib, 0);
    expect_function_call(shutdown_seshat_lib);

    assert_false(__registerWithSeshat());
}

void test_register_fail()
{
    will_return(init_lib_seshat, 0);
    expect_function_call(init_lib_seshat);

    will_return(seshat_register, -1);
    expect_function_call(seshat_register);

    will_return(shutdown_seshat_lib, 0);
    expect_function_call(shutdown_seshat_lib);

    assert_false(__registerWithSeshat());
}

void test_discover_fail()
{
    will_return(init_lib_seshat, 0);
    expect_function_call(init_lib_seshat);

    will_return(seshat_register, 0);
    expect_function_call(seshat_register);

    will_return(seshat_discover, 0);
    expect_function_call(seshat_discover);

    will_return(shutdown_seshat_lib, 0);
    expect_function_call(shutdown_seshat_lib);

    assert_false(__registerWithSeshat());
}

void test_discover_pass_but_lru_expected_fail()
{
    strcpy(g_config.local_url, URL);

    will_return(init_lib_seshat, 0);
    expect_function_call(init_lib_seshat);

    will_return(seshat_register, 0);
    expect_function_call(seshat_register);

    char *d_url = malloc(sizeof(g_config.local_url));
    strcpy(d_url, LRU);
    will_return(seshat_discover, d_url);
    expect_function_call(seshat_discover);

    will_return(shutdown_seshat_lib, 0);
    expect_function_call(shutdown_seshat_lib);

    assert_false(__registerWithSeshat());
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_all_pass),
        cmocka_unit_test(test_init_fail),
        cmocka_unit_test(test_register_fail),
        cmocka_unit_test(test_discover_fail),
        cmocka_unit_test(test_discover_pass_but_lru_expected_fail),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
