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

#include "../src/ParodusInternal.h"
#include "../src/thread_tasks.h"
#include "../src/client_list.h"


/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
ParodusMsg *ParodusMsgQ;
pthread_mutex_t g_mutex;
pthread_cond_t g_cond;
int numLoops;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

int get_numOfClients()
{
    function_called();
    return (int)mock();
}

reg_list_item_t * get_global_node(void)
{
    function_called();
    return (reg_list_item_t *)mock();
}

void listenerOnMessage(void * msg, size_t msgSize, int *numOfClients, reg_list_item_t *head )
{
    check_expected(msg);
    check_expected(numOfClients);
    check_expected(msgSize);
    check_expected(head);
    function_called();
    return;
}

int pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex)
{
    UNUSED(cond); UNUSED(mutex);
    function_called();
    return (int)mock();
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_messageHandlerTask()
{
    int clients = 1;
    ParodusMsgQ = (ParodusMsg *) malloc (sizeof(ParodusMsg));
    ParodusMsgQ->payload = "First message";
    ParodusMsgQ->len = 9;
    ParodusMsgQ->next = NULL;
    
    numLoops = 1;
    reg_list_item_t *head = (reg_list_item_t *) malloc(sizeof(reg_list_item_t));
    memset(head, 0, sizeof(reg_list_item_t));
    strcpy(head->service_name, "iot");
    strcpy(head->url, "tcp://10.0.0.1:6600");

    will_return(get_numOfClients, clients);
    expect_function_call(get_numOfClients);
    will_return(get_global_node, head);
    expect_function_call(get_global_node);
    
    expect_value(listenerOnMessage, msg, ParodusMsgQ->payload);
    expect_value(listenerOnMessage, msgSize, ParodusMsgQ->len);
    expect_any(listenerOnMessage, numOfClients);
    expect_value(listenerOnMessage, head, head);
    expect_function_call(listenerOnMessage);
    
    messageHandlerTask();
}

void err_messageHandlerTask()
{
    numLoops = 1;
    will_return(pthread_cond_wait, 0);
    expect_function_call(pthread_cond_wait);
    
    messageHandlerTask();
}
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_messageHandlerTask),
        cmocka_unit_test(err_messageHandlerTask),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
