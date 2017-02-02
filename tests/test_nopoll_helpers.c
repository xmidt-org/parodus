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
#include <cimplog.h>

#include "../src/wss_mgr.h"
#include "../src/ParodusInternal.h"
#include "wrp-c.h"

#include<errno.h>

/* Nanomsg related Macros */
#define ENDPOINT "tcp://127.0.0.1:6666"
#define CLIENT1_URL "tcp://127.0.0.1:6667"
#define CLIENT2_URL "tcp://127.0.0.1:6668"
#define CLIENT3_URL "tcp://127.0.0.1:6669"

#define UNUSED(x) (void)(x)

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
nopoll_bool nopoll_conn_is_ok__rv = nopoll_false;
nopoll_bool nopoll_conn_is_ok( noPollConn *conn )
{
	UNUSED(conn);
    return nopoll_conn_is_ok__rv;
}

nopoll_bool nopoll_conn_is_ready__rv = nopoll_false;
nopoll_bool nopoll_conn_is_ready( noPollConn *conn )
{
	UNUSED(conn);
    return nopoll_conn_is_ready__rv;
}

int nopoll_conn_send_binary_rv = 0;
int nopoll_conn_send_binary( noPollConn *conn, const char *content, long length ) 	
{
	UNUSED(conn);
	UNUSED(content);
	UNUSED(length);
    return nopoll_conn_send_binary_rv;
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_handleUpstreamMessage()
{
    handleUpstreamMessage(NULL, "hello", 6);
}

void add_suites( CU_pSuite *suite )
{
    cimplog_info("PARODUS", "--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test handleUpstreamMessage()", test_handleUpstreamMessage );
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
            cimplog_debug("PARODUS",  "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            cimplog_debug("PARODUS",  "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }

        CU_cleanup_registry();

    }

    return rv;
}
