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

#include <assert.h>
#include <nopoll.h>

//#include <nanomsg/bus.h>

#include "../src/ParodusInternal.h"
#include "wrp-c.h"

#include<errno.h>

/* Nanomsg related Macros */
#define ENDPOINT "tcp://127.0.0.1:6666"
#define CLIENT1_URL "tcp://127.0.0.1:6667"
#define CLIENT2_URL "tcp://127.0.0.1:6668"
#define CLIENT3_URL "tcp://127.0.0.1:6669"

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_checkHostIp()
{
    CU_ASSERT_EQUAL(0, checkHostIp("www.python.org"))
}

void add_suites( CU_pSuite *suite )
{
    ParodusPrint("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test checkHostIp()", test_checkHostIp );
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
