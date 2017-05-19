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
char* getWebpaConveyHeader()
{
    return NULL;
}


void setMessageHandlers()
{
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/


void test_get_global_reconnect_reason()
{
    assert_string_equal("webpa_process_starts", get_global_reconnect_reason());
}

void test_set_global_reconnect_reason()
{
    char *reason = "Factory-Reset";
    set_global_reconnect_reason(reason);
    assert_string_equal(reason, get_global_reconnect_reason());
}


/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_get_global_reconnect_reason),
        cmocka_unit_test(test_set_global_reconnect_reason),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
