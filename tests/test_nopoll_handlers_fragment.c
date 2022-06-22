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
#include <stdbool.h>
#include <CUnit/Basic.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <nopoll.h>
#include <nopoll_private.h>
#include <pthread.h>

#include "../src/nopoll_handlers.h"
#include "../src/parodus_log.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
bool LastReasonStatus;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

void set_global_reconnect_reason(char *reason)
{
    (void) reason;
}

bool get_global_reconnect_status()
{
    return LastReasonStatus;
}

bool get_interface_down_event()
{
     return false;
}

void set_global_reconnect_status(bool status)
{
    (void) status ;
}

nopoll_bool nopoll_msg_is_fragment(noPollMsg *msg)
{
   (void)msg;
    function_called();
    return (nopoll_bool) mock();
}

nopoll_bool nopoll_msg_is_final(noPollMsg *msg)
{
    (void)msg;
    function_called();
    return (nopoll_bool) mock();
}
long long currentTime()
{
	return 0;
}
const unsigned char *nopoll_msg_get_payload(noPollMsg *msg)
{
	(void)msg;
	function_called();
	
	
	return (const unsigned char *) mock();
}

int nopoll_msg_get_payload_size(noPollMsg *msg)
{
    (void) msg;
    function_called ();
	return (int) mock();
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_listenerOnMessage_queue_fragment()
{
   noPollMsg *msg, *msg1;
   msg = nopoll_msg_new ();
   //1st Fragment
   msg->payload_size = strlen("hello");   
   msg->payload = nopoll_new (char, msg->payload_size + 1);
   
    will_return(nopoll_msg_is_fragment, nopoll_true);
    expect_function_call(nopoll_msg_is_fragment);
    will_return(nopoll_msg_is_final, nopoll_false);
    will_return(nopoll_msg_is_final, nopoll_false);
    expect_function_calls(nopoll_msg_is_final, 2);
    
    listenerOnMessage_queue(NULL, NULL, msg, NULL);
    
    //2nd fragment/final message
	msg1 = nopoll_msg_new ();
	msg1->payload_size = strlen("world");   
	msg1->payload = nopoll_new (char, msg->payload_size + 1);
    
    will_return(nopoll_msg_is_fragment, nopoll_true);
    expect_function_call(nopoll_msg_is_fragment);
    will_return(nopoll_msg_is_final, nopoll_true);
    will_return(nopoll_msg_is_final, nopoll_true);
    expect_function_calls(nopoll_msg_is_final, 2);
    will_return(nopoll_msg_get_payload, (intptr_t)"helloworld");
    expect_function_call(nopoll_msg_get_payload);
    will_return(nopoll_msg_get_payload_size, 10);
    
    expect_function_call(nopoll_msg_get_payload_size);
    listenerOnMessage_queue(NULL, NULL, msg1, NULL);
    //release the message
    nopoll_msg_unref(msg);
    nopoll_msg_unref(msg1);    

}


/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( void )
{
     const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_listenerOnMessage_queue_fragment),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
