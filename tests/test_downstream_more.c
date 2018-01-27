/**
 *  Copyright 2017 Comcast Cable Communications Management, LLC
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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wrp-c.h>

#include <CUnit/Basic.h>

#include "../src/downstream.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct {
    wrp_msg_t s;
    wrp_msg_t r;
} test_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static test_t tests[] = {
        {
            .s.msg_type = WRP_MSG_TYPE__CREATE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server1",
            .s.u.crud.dest = "fake-client1/iot",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = "Some binary",
            .s.u.crud.payload_size = 11,

            .r.msg_type = WRP_MSG_TYPE__CREATE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "fake-client1",
            .r.u.crud.dest = "fake-server1",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = "{\"statusCode\":531,\"message\":\"Service Unavailable\"}",
            .r.u.crud.payload_size = 50,
        },

        {
            .s.msg_type = WRP_MSG_TYPE__RETREIVE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server2",
            .s.u.crud.dest = "fake-client2/iot",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = NULL,
            .s.u.crud.payload_size = 0,

            .r.msg_type = WRP_MSG_TYPE__RETREIVE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "fake-client2",
            .r.u.crud.dest = "fake-server2",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = "{\"statusCode\":531,\"message\":\"Service Unavailable\"}",
            .r.u.crud.payload_size = 50,
        },

        {
            .s.msg_type = WRP_MSG_TYPE__UPDATE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server3",
            .s.u.crud.dest = "fake-client3/iot",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = NULL,
            .s.u.crud.payload_size = 0,

            .r.msg_type = WRP_MSG_TYPE__UPDATE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "fake-client3",
            .r.u.crud.dest = "fake-server3",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = "{\"statusCode\":531,\"message\":\"Service Unavailable\"}",
            .r.u.crud.payload_size = 50,
        },

        {
            .s.msg_type = WRP_MSG_TYPE__DELETE,
            .s.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .s.u.crud.source = "fake-server4",
            .s.u.crud.dest = "fake-client4/iot",
            .s.u.crud.partner_ids = NULL,
            .s.u.crud.headers = NULL,
            .s.u.crud.metadata = NULL,
            .s.u.crud.include_spans = false,
            .s.u.crud.spans.spans = NULL,
            .s.u.crud.spans.count = 0,
            .s.u.crud.path = "Some path",
            .s.u.crud.payload = NULL,
            .s.u.crud.payload_size = 0,

            .r.msg_type = WRP_MSG_TYPE__DELETE,
            .r.u.crud.transaction_uuid = "c2bb1f16-09c8-11e7-93ae-92361f002671",
            .r.u.crud.source = "fake-client4",
            .r.u.crud.dest = "fake-server4",
            .r.u.crud.partner_ids = NULL,
            .r.u.crud.headers = NULL,
            .r.u.crud.metadata = NULL,
            .r.u.crud.include_spans = false,
            .r.u.crud.spans.spans = NULL,
            .r.u.crud.spans.count = 0,
            .r.u.crud.path = "Some path",
            .r.u.crud.payload = "{\"statusCode\":531,\"message\":\"Service Unavailable\"}",
            .r.u.crud.payload_size = 50,
        },

        {
            .s.msg_type = WRP_MSG_TYPE__EVENT,
            .s.u.event.source = "fake-server5",
            .s.u.event.dest = "fake-client5/iot",
            .s.u.event.partner_ids = NULL,
            .s.u.event.headers = NULL,
            .s.u.event.metadata = NULL,
            .s.u.event.payload = NULL,
            .s.u.event.payload_size = 0,

            .r = {0},
        },
    };

static uint8_t i;

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
int validate_partner_id(wrp_msg_t *msg, partners_t **partnerIds)
{
    (void) msg; (void) partnerIds;
    return 1;
}

ssize_t wrp_to_struct( const void *bytes, const size_t length, const enum wrp_format fmt,
                       wrp_msg_t **msg )
{
    (void) bytes; (void) length; (void) fmt;
    *msg = (wrp_msg_t*) malloc(sizeof(wrp_msg_t));
    (*msg)->msg_type = tests[i].s.msg_type;
    if( WRP_MSG_TYPE__EVENT == tests[i].s.msg_type ) {
        (*msg)->u.event.source = strdup(tests[i].s.u.event.source);
        (*msg)->u.event.dest = strdup(tests[i].s.u.event.dest);
    }
    else
    {
        (*msg)->u.crud.source = strdup(tests[i].s.u.crud.source);
        (*msg)->u.crud.dest = strdup(tests[i].s.u.crud.dest);
        (*msg)->u.crud.transaction_uuid = strdup(tests[i].s.u.crud.transaction_uuid);
        (*msg)->u.crud.path = strdup(tests[i].s.u.crud.path);
    }
    return (ssize_t) sizeof(tests[i].s);
}

ssize_t wrp_struct_to( const wrp_msg_t *msg, const enum wrp_format fmt, void **bytes )
{
    (void) fmt; 
    *bytes = malloc(1);
    CU_ASSERT(WRP_MSG_TYPE__EVENT != msg->msg_type);
    CU_ASSERT(tests[i].r.msg_type == msg->msg_type);
    CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.transaction_uuid, msg->u.crud.transaction_uuid);
    printf("tests[%d].r.u.crud.source = %s, msg->u.crud.source = %s\n", i, tests[i].r.u.crud.source, msg->u.crud.source);
    CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.source, msg->u.crud.source);
    CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.dest, msg->u.crud.dest);
    CU_ASSERT_STRING_EQUAL(tests[i].r.u.crud.path, msg->u.crud.path);
    CU_ASSERT(tests[i].r.u.crud.payload_size == msg->u.crud.payload_size);
    CU_ASSERT(0 == memcmp(tests[i].r.u.crud.payload, msg->u.crud.payload, msg->u.crud.payload_size));

    return 1;
}

void sendUpstreamMsgToServer(void **resp_bytes, size_t resp_size)
{
    (void) resp_bytes; (void) resp_size;    
}

int get_numOfClients()
{
    return 0;
}

reg_list_item_t *get_global_node(void)
{
    return NULL;
}

void wrp_free_struct( wrp_msg_t *msg )
{
    if( WRP_MSG_TYPE__EVENT == tests[i].s.msg_type ) {
        if( NULL != msg->u.event.source) {
            free(msg->u.event.source);
        }
        if( NULL != msg->u.event.dest) {
            free(msg->u.event.dest);
        }
    }
    else
    {
        if( NULL != msg->u.crud.source) {
            free(msg->u.crud.source);
        }
        if( NULL != msg->u.crud.dest) {
            free(msg->u.crud.dest);
        }
        if( NULL != msg->u.crud.transaction_uuid) {
            free(msg->u.crud.transaction_uuid);
        }
        if( NULL != msg->u.crud.path) {
            free(msg->u.crud.path);
        }
    }    
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_listenerOnMessage()
{
    size_t t_size = sizeof(tests)/sizeof(test_t);
    void *msg = malloc(1);

    for( i = 0; i < t_size; i++ ) {
        listenerOnMessage(msg, 0);
    }
    free(msg);
}

void add_suites( CU_pSuite *suite )
{
    printf("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test 1", test_listenerOnMessage );
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

