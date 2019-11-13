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
#include <wrp-c.h>

#include "../src/downstream.h"
#include "../src/ParodusInternal.h"
#include "../src/partners_check.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
ParodusMsg *ParodusMsgQ;
pthread_mutex_t g_mutex;
pthread_cond_t g_cond;
int crud_test = 0;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
void addCRUDmsgToQueue(wrp_msg_t *crudMsg)
{
	UNUSED(crudMsg) ;
	function_called();
}

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

void release_global_node (void)
{
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
	parStrncpy((*msg)->u.req.dest,"mac:1122334455/iot", 100);
	parStrncpy((*msg)->u.req.partner_ids->partner_ids[0],"comcast", 64);
	if(crud_test)
	{
			(*msg)->msg_type = WRP_MSG_TYPE__CREATE;
			parStrncpy((*msg)->u.crud.dest,"mac:1122334455/parodus", 100);
                        (*msg)->u.crud.source = (char *) malloc(sizeof(char) *40);
			parStrncpy ((*msg)->u.crud.source, "tag-update", 40);
                        (*msg)->u.crud.transaction_uuid = (char *) malloc(sizeof(char) *40);
			parStrncpy ((*msg)->u.crud.transaction_uuid, "1234", 40);
	}
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
    parStrncpy(head->service_name, "iot", sizeof(head->service_name));
    parStrncpy(head->url, "tcp://10.0.0.1:6600", sizeof(head->url));

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
    parStrncpy(head2->service_name, "iot", sizeof(head2->service_name));
    parStrncpy(head2->url, "tcp://10.0.0.1:6622", sizeof(head2->url));
    
    reg_list_item_t *head1 = (reg_list_item_t *) malloc(sizeof(reg_list_item_t));
    memset(head1, 0, sizeof(reg_list_item_t));
    parStrncpy(head1->service_name, "lmlite", sizeof(head1->service_name));
    parStrncpy(head1->url, "tcp://10.0.0.1:6611", sizeof(head1->url));
    head1->next = head2;
    
    reg_list_item_t *head = (reg_list_item_t *) malloc(sizeof(reg_list_item_t));
    memset(head, 0, sizeof(reg_list_item_t));
    parStrncpy(head->service_name, "config", sizeof(head->service_name));
    parStrncpy(head->url, "tcp://10.0.0.1:6600", sizeof(head->url));
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


void test_listenerOnMessageCRUD()
{
	crud_test = 1;
    will_return(wrp_to_struct, 2);
    expect_function_calls(wrp_to_struct, 1);

    will_return(get_numOfClients, 0);
    expect_function_call(get_numOfClients);
    will_return(validate_partner_id, 0);
    expect_function_call(validate_partner_id);
    will_return(get_global_node, (intptr_t)NULL);
    expect_function_call(get_global_node);
    expect_function_call(addCRUDmsgToQueue);
    listenerOnMessage("Hello", 6);
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
        cmocka_unit_test(test_listenerOnMessageCRUD),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
