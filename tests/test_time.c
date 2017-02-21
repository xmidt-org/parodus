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
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>

#include <CUnit/Basic.h>

#include "../src/ParodusInternal.h"
#include "../src/time.h"

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
#define MOCK_RETURN_SEC 1

int clock_gettime(int ID, struct timespec *timer)
{
    (void) ID;
    timer->tv_sec = MOCK_RETURN_SEC;
    timer->tv_nsec = 2;

    return 0;
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_getCurrentTime()
{
    struct timespec timer;
    getCurrentTime(&timer);
}

void test_getCurrentTimeInMicroSeconds(void)
{
    struct timespec timer;
    CU_ASSERT( MOCK_RETURN_SEC * 1000000 == getCurrentTimeInMicroSeconds(&timer) );
}

void test_timeValDiff(void)
{
    struct timespec start, finish;
    long msec_diff;
    start.tv_sec = 1; start.tv_nsec = 1000000;
    finish.tv_sec = 2; finish.tv_nsec = 2000000;
    msec_diff = ((finish.tv_sec - start.tv_sec) * 1000) + ((finish.tv_nsec - start.tv_nsec)/1000000);
    CU_ASSERT( msec_diff == timeValDiff(&start, &finish) );
}

void add_suites( CU_pSuite *suite )
{
    ParodusInfo("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_getCurrentTime );
    CU_add_test( *suite, "Test 2", test_getCurrentTimeInMicroSeconds );
    CU_add_test( *suite, "Test 3", test_timeValDiff );
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
