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

#include "../src/downstream.h"
#include "../src/ParodusInternal.h"
#include "../src/partners_check.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
ParodusMsg *ParodusMsgQ;
pthread_mutex_t g_mutex;
pthread_cond_t g_cond;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
void sendUpstreamMsgToServer(void **resp_bytes, size_t resp_size)
{
    UNUSED(resp_bytes); UNUSED(resp_size);
    function_called();
}

int get_numOfClients()
{
    function_called();
    return (int) mock();
}

reg_list_item_t * get_global_node(void)
{
    function_called();
    return mock_ptr_type(reg_list_item_t *);
}

ssize_t wrp_to_struct( const void *bytes, const size_t length,
                       const enum wrp_format fmt, wrp_msg_t **msg )
{
    UNUSED(bytes); UNUSED(length); UNUSED(fmt);
    function_called();
    *msg = (wrp_msg_t*) malloc(sizeof(wrp_msg_t));
    memset(*msg, 0, sizeof(wrp_msg_t));
    (*msg)->msg_type = WRP_MSG_TYPE__REQ;
    (*msg)->u.req.dest = (char *) malloc(sizeof(char) *100);
    (*msg)->u.req.partner_ids = (partners_t *) malloc(sizeof(partners_t));
    (*msg)->u.req.partner_ids->count = 1;
    (*msg)->u.req.partner_ids->partner_ids[0] = (char *) malloc(sizeof(char) *64);
    strcpy((*msg)->u.req.dest,"mac:1122334455/iot");
    strcpy((*msg)->u.req.partner_ids->partner_ids[0],"comcast");
    return (ssize_t) mock();
}

int nn_send (int s, const void *buf, size_t len, int flags)
{
    UNUSED(s); UNUSED(buf); UNUSED(len); UNUSED(flags);
    function_called();
    return (int) mock();
}

int validate_partner_id(wrp_msg_t *msg, partners_t **partnerIds)
{
    UNUSED(msg); UNUSED(partnerIds);
    function_called();
    return (int) mock();
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_listenerOnMessage()
{
    will_return(wrp_to_struct, 1);
    expect_function_calls(wrp_to_struct, 1);
    reg_list_item_t *head = (reg_list_item_t *) malloc(sizeof(reg_list_item_t));
    memset(head, 0, sizeof(reg_list_item_t));
    strcpy(head->service_name, "iot");
    strcpy(head->url, "tcp://10.0.0.1:6600");

    will_return(get_numOfClients, 1);
    expect_function_call(get_numOfClients);
    will_return(validate_partner_id, 1);
    expect_function_call(validate_partner_id);
    will_return(get_global_node, (intptr_t)head);
    expect_function_call(get_global_node);
    will_return(nn_send, 20);
    expect_function_calls(nn_send, 1);

    listenerOnMessage("Hello", 6);
    free(head);
}

void test_listenerOnMessageMultipleClients()
{
    will_return(wrp_to_struct, 1);
    expect_function_calls(wrp_to_struct, 1);

    reg_list_item_t *head2 = (reg_list_item_t *) malloc(sizeof(reg_list_item_t));
    memset(head2, 0, sizeof(reg_list_item_t));
    strcpy(head2->service_name, "iot");
    strcpy(head2->url, "tcp://10.0.0.1:6622");
    
    reg_list_item_t *head1 = (reg_list_item_t *) malloc(sizeof(reg_list_item_t));
    memset(head1, 0, sizeof(reg_list_item_t));
    strcpy(head1->service_name, "lmlite");
    strcpy(head1->url, "tcp://10.0.0.1:6611");
    head1->next = head2;
    
    reg_list_item_t *head = (reg_list_item_t *) malloc(sizeof(reg_list_item_t));
    memset(head, 0, sizeof(reg_list_item_t));
    strcpy(head->service_name, "config");
    strcpy(head->url, "tcp://10.0.0.1:6600");
    head->next = head1;
    
    will_return(get_numOfClients, 3);
    expect_function_call(get_numOfClients);
    will_return(validate_partner_id, 0);
    expect_function_call(validate_partner_id);
    will_return(get_global_node, (intptr_t)head);
    expect_function_call(get_global_node);
    will_return(nn_send, 20);
    expect_function_calls(nn_send, 1);

    listenerOnMessage("Hello", 6);
    free(head1);
    free(head2);
    free(head);
}

void err_listenerOnMessage()
{
    will_return(wrp_to_struct, 0);
    expect_function_calls(wrp_to_struct, 1);
    
    listenerOnMessage("Hello", 6);
}

void err_listenerOnMessageServiceUnavailable()
{
    will_return(wrp_to_struct, 2);
    expect_function_calls(wrp_to_struct, 1);

    will_return(get_numOfClients, 0);
    expect_function_call(get_numOfClients);
    will_return(validate_partner_id, 0);
    expect_function_call(validate_partner_id);
    will_return(get_global_node, (intptr_t)NULL);
    expect_function_call(get_global_node);
    expect_function_call(sendUpstreamMsgToServer);
    
    listenerOnMessage("Hello", 6);
}

void err_listenerOnMessageInvalidPartnerId()
{
    will_return(wrp_to_struct, 2);
    expect_function_calls(wrp_to_struct, 1);

    will_return(get_numOfClients, 0);
    expect_function_call(get_numOfClients);
    will_return(validate_partner_id, -1);
    expect_function_call(validate_partner_id);
    expect_function_call(sendUpstreamMsgToServer);

    listenerOnMessage("Hello", 6);
}

void err_listenerOnMessageAllNull()
{
    listenerOnMessage(NULL, 0);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_listenerOnMessage),
        cmocka_unit_test(test_listenerOnMessageMultipleClients),
        cmocka_unit_test(err_listenerOnMessage),
        cmocka_unit_test(err_listenerOnMessageServiceUnavailable),
        cmocka_unit_test(err_listenerOnMessageInvalidPartnerId),
        cmocka_unit_test(err_listenerOnMessageAllNull),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
