/**
 *  Copyright 2018 Comcast Cable Communications Management, LLC
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

#include <CUnit/Basic.h>

#include "../src/ParodusInternal.h"
#include "../src/parodus_interface.h"
#include "../src/config.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define HUB   "tcp://127.0.0.1:7777"
#define SPOKE "tcp://127.0.0.1:8888"

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    char   *d;
    char   *n;
    ssize_t nsz;
} test_t;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* None */

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

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_all_pass()
{
    test_t tests[] = {
        {   // 0
            .d = HUB,
            .n = "Some binary",
            .nsz = 11,
        },
    };
    char *msg;
    ssize_t msg_sz;
    int rv;
    bool result;
    pthread_t t;

    rv = start_listening_to_spokes( &t, HUB );
    CU_ASSERT(0 == rv);

    strncpy(g_config.local_url, SPOKE, sizeof(g_config.local_url));
    result = send_to_hub(tests[0].d, tests[0].n, tests[0].nsz);
    CU_ASSERT(true == result);

    while( true ) {
        msg_sz = receive_from_spoke(SPOKE, &msg);
        if( 0 < msg_sz ) {
            CU_ASSERT_EQUAL( (tests[0].nsz + 1), msg_sz );
            CU_ASSERT_STRING_EQUAL( tests[0].n, msg );
            free(msg);
            break;
        }
        sleep(2);
    }
    stop_listening_to_spokes();
}

void test_send_fail()
{
}

void test_receive_fail()
{
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main(void)
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;

    if( CUE_SUCCESS == CU_initialize_registry() ) {
        printf("--------Start of Test Cases Execution ---------\n");
        suite = CU_add_suite( "tests", NULL, NULL );
        CU_add_test( suite, "Test All Pass", test_all_pass );
        CU_add_test( suite, "Test Send Fail", test_send_fail );
        CU_add_test( suite, "Test Receive Fail", test_receive_fail );

        if( NULL != suite ) {
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            printf( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            printf( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }

        CU_cleanup_registry();

    }

    return rv;
}
