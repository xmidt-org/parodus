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
#include <assert.h>
#include <stdarg.h>

#include <CUnit/Basic.h>
#include <nopoll.h>
#include <nopoll_private.h>

#include "../src/ParodusInternal.h"
#include "../src/connection.h"

extern void listenerOnCloseMessage(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data);

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void *a(void *in)
{
    char str[] = "SSL_Socket_Close";
    (void) in;

    LastReasonStatus = false;
    listenerOnCloseMessage(NULL, NULL, NULL);
    
    LastReasonStatus = false;
    listenerOnCloseMessage(NULL, NULL, (noPollPtr) str);

    LastReasonStatus = true;
    listenerOnCloseMessage(NULL, NULL, NULL);

    LastReasonStatus = true;
    listenerOnCloseMessage(NULL, NULL, (noPollPtr) str);

    pthread_exit(0);
    return NULL;
}

void *b(void *in)
{
    char str[] = "SSL_Socket_Close";
    (void) in;

    LastReasonStatus = false;
    listenerOnCloseMessage(NULL, NULL, NULL);
    
    LastReasonStatus = false;
    listenerOnCloseMessage(NULL, NULL, (noPollPtr) str);

    LastReasonStatus = true;
    listenerOnCloseMessage(NULL, NULL, NULL);

    LastReasonStatus = true;
    listenerOnCloseMessage(NULL, NULL, (noPollPtr) str);

    pthread_exit(0);
    return NULL;
}

void test_listenerOnCloseMessage()
{
    pthread_t thread_a, thread_b;

    terminated = true;
    pthread_create(&thread_a, NULL, a, NULL);
    terminated = false;
    pthread_create(&thread_b, NULL, b, NULL);

    pthread_join(thread_a, NULL);
    pthread_join(thread_b, NULL);
}

void add_suites( CU_pSuite *suite )
{
    ParodusInfo("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_listenerOnCloseMessage );
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( void )
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;

    if( CUE_SUCCESS == CU_initialize_registry() ) {
        add_suites( &suite );

        if( NULL != suite ) {
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            ParodusPrint( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            ParodusPrint( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }

        CU_cleanup_registry();

    }

    return rv;
}
