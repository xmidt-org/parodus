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
#include <stdio.h>
#include <rbus.h>
#include <CUnit/Basic.h>
#include "../src/upstream.h"
#include "../src/ParodusInternal.h"

#define CLOUD_CONN_ONLINE "cloud_conn_online_event"

rbusHandle_t handle;

int validate_partner_id(wrp_msg_t *msg, partners_t **partnerIds)
{
	return;
}

int sendUpstreamMsgToServer(void **resp_bytes, size_t resp_size)
{
	return;
}
//Test case for rbusRegCloudConnectionOnlineEventRbushandle failure 
void test_regConnOnlineEventRbushandle_failure()
{
	int result = regConnOnlineEvent();
	CU_ASSERT_EQUAL(result, -1);
}

//Test case for rbusRegCloudConnectionOnlineEvent success
void test_regConnOnlineEvent_success()
{
	subscribeRBUSevent();
	int result = regConnOnlineEvent();
	CU_ASSERT_EQUAL(result, 0);
}

//Test case for rbusRegCloudConnectionOnlineEvent failure
void test_regConnOnlineEvent_failure()
{
	//Register event two time and it will create error
	int result = regConnOnlineEvent();
	result = regConnOnlineEvent();
	CU_ASSERT_NOT_EQUAL(result, 0);
}

//Test function for CloudConnSubscribeHandler success
void test_CloudConnSubscribeHandler_success()
{
	rbusError_t ret = RBUS_ERROR_BUS_ERROR;
	ret = CloudConnSubscribeHandler(handle, RBUS_EVENT_ACTION_SUBSCRIBE , CLOUD_CONN_ONLINE, NULL, 0, false);
	CU_ASSERT_EQUAL(ret, 0);
}

static void subscribeEventSuccessCallbackHandler(
    rbusHandle_t handle,
    rbusEvent_t const* event,
    rbusEventSubscription_t* subscription)
{
	rbusValue_t new_value;
	new_value = rbusObject_GetValue(event->data, "value");
	int incoming_value = rbusValue_GetInt32(new_value);
    	ParodusInfo("incoming_value is %d\n", incoming_value);

    	if(incoming_value == 1)
    	{
		ParodusInfo("rbusEvent_OnSubscribe callback received successfully\n");
		CU_ASSERT(1);
    	}
    (void)handle;
    (void)subscription;
}

void subscribe_to_event(char * eventname)
{
	int rc = RBUS_ERROR_SUCCESS;

	ParodusInfo("rbus_open for component %s\n", "consumer");
	rc = rbus_open(&handle, "consumer");
	if(rc != RBUS_ERROR_SUCCESS)
	{
		CU_FAIL("rbus_open failed for subscribe_to_event");
	}

	if(strncmp(eventname, CLOUD_CONN_ONLINE, strlen(CLOUD_CONN_ONLINE)) == 0)
	{
		ParodusInfo("Inside subscribe_to_event for %s and eventname is %s\n", CLOUD_CONN_ONLINE, eventname);
		rc = rbusEvent_Subscribe(handle, eventname, subscribeEventSuccessCallbackHandler, NULL, 0);
	}

	if(rc != RBUS_ERROR_SUCCESS)
		CU_FAIL("subscribe_to_event onsubscribe event failed");
}

void rbushandleclose(char * name)
{
	rbusEvent_Unsubscribe(handle, name);
	rbus_close(handle);
}

//Test case for SendRbusEventCloudConnOnline Success
void test_SendConnOnlineEvent_success()
{
    	subscribe_to_event(CLOUD_CONN_ONLINE);
	rbusError_t ret = SendConnOnlineEvent();
	CU_ASSERT_EQUAL(ret, 0);
	rbushandleclose(CLOUD_CONN_ONLINE);
	sleep(1);
}

//Test case for SendRbusEventCloudConnOnline failure
void test_SendConnOnlineEvent_failure()
{
	rbusError_t ret = SendConnOnlineEvent();
	CU_ASSERT_NOT_EQUAL(ret, 0);
}

void add_suites( CU_pSuite *suite )
{
	*suite = CU_add_suite( "tests", NULL, NULL );
	CU_add_test( *suite, "test regConnOnlineEventRbushandle_failure", test_regConnOnlineEventRbushandle_failure);
	CU_add_test( *suite, "test regConnOnlineEvent_success", test_regConnOnlineEvent_success);
	CU_add_test( *suite, "test regConnOnlineEvent_failure", test_regConnOnlineEvent_failure);
	CU_add_test( *suite, "test CloudConnSubscribeHandler_success", test_CloudConnSubscribeHandler_success);
	CU_add_test( *suite, "test SendConnOnlineEvent_success", test_SendConnOnlineEvent_success);
	CU_add_test( *suite, "test SendConnOnlineEvent_failure", test_SendConnOnlineEvent_failure);
}


int main( int argc, char *argv[] )
{
	unsigned rv = 1;
    	CU_pSuite suite = NULL;

    	(void ) argc;
    	(void ) argv;

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

