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
static void *send_hub();

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static test_t tests[] = {
    {   // 0 
        .d = HUB,
        .n = "Some binary",
        .nsz = 11,
    },
    {   // 1 
        .d = SPOKE,
        .n = "Some other binary",
        .nsz = 17,
    },
};

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_pub_sub()
{
    pthread_t t;
    int pipeline_sock, pubsub_sock;
    char *msg = NULL;
    ssize_t msg_sz = 0;

    pthread_create(&t, NULL, send_hub, NULL);

    spoke_setup( SPOKE, HUB, NULL, &pipeline_sock, &pubsub_sock );
    for( ;; ) {
        msg_sz = check_inbox(pubsub_sock, (void **) &msg);
        if( 0 < msg_sz ) {
            printf("check spoke - msg_sz = %zd\n", msg_sz);
            CU_ASSERT_EQUAL( (tests[1].nsz), msg_sz );
            CU_ASSERT_STRING_EQUAL( tests[1].n, msg );
            free_msg(msg);
            break;
        }
    }
    sock_cleanup(pipeline_sock);
    sock_cleanup(pubsub_sock);
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
        CU_add_test( suite, "Test Pub/Sub",   test_pub_sub );

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

/*----------------------------------------------------------------------------*/
/*                             Internal Functions                             */
/*----------------------------------------------------------------------------*/
static void *send_hub()
{
    int pipeline_sock, pubsub_sock;
    bool result;

    hub_setup( SPOKE, HUB, &pipeline_sock, &pubsub_sock );
    sleep(5);
    for( ;; ) {
        result = send_msg(pubsub_sock, tests[1].n, tests[1].nsz);
        if( 0 < result ) {
            printf("send hub - result = %d\n", result);
            CU_ASSERT(true == result);
            break;
        }
    }
    sock_cleanup(pipeline_sock);
    sock_cleanup(pubsub_sock);
    return NULL;
}
