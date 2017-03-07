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
#include <malloc.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <nopoll.h>
#include <wrp-c.h>
#include <nanomsg/nn.h>

#include "../src/upstream.h"
#include "../src/config.h"
#include "../src/client_list.h"
#include "../src/ParodusInternal.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static noPollConn *conn;
static char *reconnect_reason = "webpa_process_starts";
static ParodusCfg parodusCfg;
extern size_t metaPackSize;
static reg_list_item_t * g_head;
static int numOfClients;
extern UpStreamMsg *UpStreamMsgQ;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

noPollConn *get_global_conn()
{
     return conn;   
}

char *get_global_reconnect_reason()
{
    return reconnect_reason;
}

reg_list_item_t * get_global_node(void)
{
    return g_head;
}

int get_numOfClients()
{
    return numOfClients;
}
void sendMessage(noPollConn *conn, void *msg, size_t len)
{
    (void) conn; (void) msg; (void) len;
    function_called();
}

ParodusCfg *get_parodus_cfg(void) 
{
    return &parodusCfg;
}

ssize_t wrp_pack_metadata( const data_t *packData, void **data )
{
    (void) packData; (void) data;
    function_called();
    
    return (ssize_t)mock();
}

size_t appendEncodedData( void **appendData, void *encodedBuffer, size_t encodedSize, void *metadataPack, size_t metadataSize )
{
    (void) encodedBuffer; (void) encodedSize; (void) metadataPack; (void) metadataSize;
    function_called();
    char *data = (char *) malloc (sizeof(char) * 100);
    strcpy(data, "AAAAAAAAYYYYIGkYTUYFJH");
    *appendData = data;
    return (size_t)mock();
}

int sendAuthStatus(reg_list_item_t *new_node)
{
    (void) new_node;
    function_called();
    return (int)mock();
}

int addToList( wrp_msg_t **msg)
{
    (void) msg;
    function_called();
    return (int)mock();
}

int nn_bind (int s, const char *addr)
{
    (void) s; (void) addr;
    function_called();
    return (int)mock();
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_getParodusUrl()
{
    getParodusUrl();
    
    putenv("PARODUS_SERVICE_URL=tcp://10.0.0.1:6000");
    getParodusUrl();
}

void test_packMetaData()
{
    will_return(wrp_pack_metadata, 100);
    expect_function_call(wrp_pack_metadata);
    packMetaData();
}

void err_packMetaData()
{
    will_return(wrp_pack_metadata, 0);
    expect_function_call(wrp_pack_metadata);
    packMetaData();
}

void err_handle_upstream()
{
    will_return(nn_bind, -1);
    expect_function_call(nn_bind);
    handle_upstream();
}

void test_sendUpstreamMsgToServer()
{
    void *bytes = NULL;
    wrp_msg_t msg;
    memset(&msg, 0, sizeof(wrp_msg_t));
    
    msg.msg_type = WRP_MSG_TYPE__EVENT;
    wrp_struct_to( &msg, WRP_BYTES, &bytes );
    metaPackSize = 10;
    
    will_return(appendEncodedData, 100);
    expect_function_call(appendEncodedData);
    expect_function_call(sendMessage);
    sendUpstreamMsgToServer(&bytes, 110);
}

void err_sendUpstreamMsgToServer()
{
    metaPackSize = 0;
    sendUpstreamMsgToServer(NULL, 110);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_getParodusUrl),
        cmocka_unit_test(test_packMetaData),
        cmocka_unit_test(err_packMetaData),
        cmocka_unit_test(err_handle_upstream),
        cmocka_unit_test(test_sendUpstreamMsgToServer),
        cmocka_unit_test(err_sendUpstreamMsgToServer),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
