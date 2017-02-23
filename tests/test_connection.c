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

#include "../src/wss_mgr.h"
#include "../src/ParodusInternal.h"
#include "../src/connection.h"
#include "wrp-c.h"

noPollCtx *ctx = NULL;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_set_global_conn()
{
    noPollConn *conn;
    noPollConnOpts * opts;
    int headerCount;
    const char * headerNames[HTTP_CUSTOM_HEADER_COUNT] = {"X-WebPA-Device-Name","X-WebPA-Device-Protocols","User-Agent", "X-WebPA-Convey"};
    const char * headerValues[HTTP_CUSTOM_HEADER_COUNT];

    headerValues[0] = "123567892366";
    headerValues[1] = "wrp-0.11,getset-0.1";  
    headerValues[2] = "WebPA-1.6 (TG1682_DEV_master_2016000000sdy;TG1682/ARRISGroup,Inc.;)";
    headerValues[3] = "zacbvfxcvglodfjdigjkdshuihgkvn";

    headerCount = HTTP_CUSTOM_HEADER_COUNT;

    opts = nopoll_conn_opts_new ();
    nopoll_conn_opts_ssl_peer_verify (opts, nopoll_false);
    nopoll_conn_opts_set_ssl_protocol (opts, NOPOLL_METHOD_TLSV1_2); 
    conn = nopoll_conn_tls_new(ctx, opts, "fabric-cd.webpa.comcast.net", "8080", NULL, "/api/v2/device", NULL, NULL, "eth0", headerNames, headerValues, headerCount);
    set_global_conn(conn);
    CU_ASSERT(conn == get_global_conn());
    close_and_unref_connection(get_global_conn());
}

void test_set_parodus_cfg()
{
    ParodusCfg *cfg;

    cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    strcpy(cfg->hw_model, "TG1682");
    strcpy(cfg->hw_serial_number, "Fer23u948590");
    strcpy(cfg->hw_manufacturer , "ARRISGroup,Inc.");
    strcpy(cfg->hw_mac , "123567892366");
    strcpy(cfg->hw_last_reboot_reason , "unknown");
    strcpy(cfg->fw_name , "2.364s2");
    strcpy(cfg->webpa_url , "fabric.webpa.comcast.net");
    strcpy(cfg->webpa_interface_used , "eth0");
    strcpy(cfg->webpa_protocol , "WebPA-1.6");
    strcpy(cfg->webpa_uuid , "1234567-345456546");
    cfg->secureFlag = 1;
    cfg->boot_time = 423457;
    cfg->webpa_ping_timeout = 30;
    cfg->webpa_backoff_max = 255;

    set_parodus_cfg(cfg);

    CU_ASSERT_STRING_EQUAL(cfg->hw_model, get_parodus_cfg()->hw_model);
    CU_ASSERT_STRING_EQUAL(cfg->webpa_uuid, get_parodus_cfg()->webpa_uuid);
    CU_ASSERT_STRING_EQUAL(cfg->hw_serial_number, get_parodus_cfg()->hw_serial_number);
    CU_ASSERT_STRING_EQUAL(cfg->hw_manufacturer , get_parodus_cfg()->hw_manufacturer);
    CU_ASSERT_STRING_EQUAL(cfg->hw_mac, get_parodus_cfg()->hw_mac);
    CU_ASSERT_STRING_EQUAL(cfg->hw_last_reboot_reason,get_parodus_cfg()->hw_last_reboot_reason);
    CU_ASSERT_STRING_EQUAL(cfg->fw_name,get_parodus_cfg()->fw_name);
    CU_ASSERT_STRING_EQUAL(cfg->webpa_url, get_parodus_cfg()->webpa_url);
    CU_ASSERT_STRING_EQUAL(cfg->webpa_interface_used , get_parodus_cfg()->webpa_interface_used);
    CU_ASSERT_STRING_EQUAL(cfg->webpa_protocol, get_parodus_cfg()->webpa_protocol);
    CU_ASSERT_EQUAL(cfg->boot_time, get_parodus_cfg()->boot_time);
    CU_ASSERT_EQUAL(cfg->webpa_ping_timeout, get_parodus_cfg()->webpa_ping_timeout);
    CU_ASSERT_EQUAL(cfg->webpa_backoff_max, get_parodus_cfg()->webpa_backoff_max);
    CU_ASSERT_EQUAL(cfg->secureFlag, get_parodus_cfg()->secureFlag);
}

void test_createSecureConnection()
{
    ctx = nopoll_ctx_new();
    CU_ASSERT_PTR_NOT_NULL(ctx);
    CU_ASSERT_EQUAL(createNopollConnection(ctx), nopoll_true);
    CU_ASSERT_PTR_NOT_NULL(get_global_conn());
    CU_ASSERT(nopoll_conn_is_ok(get_global_conn()));
}

void test_sendResponse()
{
    int bytesWritten;
	bytesWritten = sendResponse(get_global_conn(), "Hello Parodus!", strlen("Hello Parodus!"));
	ParodusPrint("Number of bytes written: %d\n", bytesWritten);
	CU_ASSERT(bytesWritten > 0);
}

void test_closeConnection()
{
    close_and_unref_connection(get_global_conn());
}

void err_createConnection()
{
    noPollCtx *ctx = NULL;
    CU_ASSERT_EQUAL(createNopollConnection(ctx), nopoll_false);
    CU_ASSERT(!nopoll_conn_is_ok(get_global_conn()));
}
void err_sendResponse()
{
    int bytesWritten;
	bytesWritten = sendResponse(NULL, "Hello Parodus!", strlen("Hello Parodus!"));
	ParodusPrint("Number of bytes written: %d\n", bytesWritten);
	CU_ASSERT(bytesWritten <= 0);
}

void add_suites( CU_pSuite *suite )
{
    ParodusInfo("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test Set parodus config", test_set_parodus_cfg );
    CU_add_test( *suite, "Test Create secure nopoll connection", test_createSecureConnection );
    CU_add_test( *suite, "Test send response", test_sendResponse );
    CU_add_test( *suite, "Test close connection", test_closeConnection );
    CU_add_test( *suite, "Test Set global connection", test_set_global_conn );
    CU_add_test( *suite, "Error Create nopoll connection", err_createConnection );
    CU_add_test( *suite, "Error send response", err_sendResponse );
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
            printf( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            printf( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }

        CU_cleanup_registry();

    }

    return rv;
}
