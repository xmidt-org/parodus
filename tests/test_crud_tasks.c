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
#include <string.h> 
#include <unistd.h> 

#include <wrp-c.h>
#include "../src/crud_tasks.h"
#include "../src/config.h"
#include "../src/client_list.h"
#include "../src/ParodusInternal.h"
#include "../src/partners_check.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
wrp_msg_t *response = NULL;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

cJSON * cJSON_Parse(const char *payload)
{
	UNUSED(payload);
    function_called();
    return (cJSON *) mock();
}

int createObject(wrp_msg_t *reqMsg , wrp_msg_t **response)
{
    UNUSED(reqMsg); UNUSED(response); 
    function_called();
    return (int) mock();
}

int retrieveObject(wrp_msg_t *reqMsg , wrp_msg_t **response)
{
    UNUSED(reqMsg); UNUSED(response);
    function_called();
    return (int) mock();
}

int updateObject(wrp_msg_t *reqMsg , wrp_msg_t **response)
{
    UNUSED(reqMsg); UNUSED(response);
    function_called();
    return (int) mock();
}

int deleteObject(wrp_msg_t *reqMsg , wrp_msg_t **response)
{
    UNUSED(reqMsg); UNUSED(response);
    function_called();
    return (int) mock();
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_processCrudRequestCreate()
{
	int ret = -1;
	wrp_msg_t *reqMsg = NULL;
    reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
    memset(reqMsg, 0, sizeof(wrp_msg_t));
    
    reqMsg->msg_type = 5;
    reqMsg->u.crud.transaction_uuid = strdup("1234");
    reqMsg->u.crud.source = strdup("tag-update");
    reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
    will_return(createObject, 0);
    expect_function_call(createObject);
    
    will_return(cJSON_Parse, 0);
    expect_function_call(cJSON_Parse);
    ret = processCrudRequest(reqMsg, &response);
    assert_int_equal(ret, 0);

	wrp_free_struct(reqMsg);

}

void test_processCrudRequestCreateFailure()
{
	int ret = -2;
	wrp_msg_t *reqMsg = NULL;
    reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
    memset(reqMsg, 0, sizeof(wrp_msg_t));
    
    reqMsg->msg_type = 5;
    reqMsg->u.crud.transaction_uuid = strdup("1234");
    reqMsg->u.crud.source = strdup("tag-update");
    reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
    will_return(createObject, -1);
    expect_function_call(createObject);
    ret = processCrudRequest(reqMsg, &response);
    assert_int_equal(ret, -1);

	wrp_free_struct(reqMsg);

}

void test_processCrudRequestRetrieve()
{
	int ret = -2;
	wrp_msg_t *reqMsg = NULL;
    reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
    memset(reqMsg, 0, sizeof(wrp_msg_t));
    
    reqMsg->msg_type = 6;
    reqMsg->u.crud.transaction_uuid = strdup("1234");
    reqMsg->u.crud.source = strdup("tag-update");
    reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
    will_return(retrieveObject, 0);
    expect_function_call(retrieveObject);
    will_return(cJSON_Parse, 0);
    expect_function_call(cJSON_Parse);
    ret = processCrudRequest(reqMsg, &response);
    assert_int_equal(ret, 0);

	wrp_free_struct(reqMsg);

}

void test_processCrudRequestRetrieveFailure()
{
	int ret = -2;
	wrp_msg_t *reqMsg = NULL;
    reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
    memset(reqMsg, 0, sizeof(wrp_msg_t));
    
    reqMsg->msg_type = 6;
    reqMsg->u.crud.transaction_uuid = strdup("1234");
    reqMsg->u.crud.source = strdup("tag-update");
    reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
    will_return(retrieveObject, -1);
    expect_function_call(retrieveObject);
    
    ret = processCrudRequest(reqMsg, &response);
    assert_int_equal(ret, -1);

	wrp_free_struct(reqMsg);

}

void test_processCrudRequestUpdate()
{
	int ret = -2;
	wrp_msg_t *reqMsg = NULL;
    reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
    memset(reqMsg, 0, sizeof(wrp_msg_t));
    
    reqMsg->msg_type = 7;
    reqMsg->u.crud.transaction_uuid = strdup("1234");
    reqMsg->u.crud.source = strdup("tag-update");
    reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
    will_return(updateObject, 0);
    expect_function_call(updateObject);
    ret = processCrudRequest(reqMsg, &response);
    assert_int_equal(ret, 0);

	wrp_free_struct(reqMsg);

}

void test_processCrudRequestUpdateFailure()
{
	int ret = -2;
	wrp_msg_t *reqMsg = NULL;
    reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
    memset(reqMsg, 0, sizeof(wrp_msg_t));
    
    reqMsg->msg_type = 7;
    reqMsg->u.crud.transaction_uuid = strdup("1234");
    reqMsg->u.crud.source = strdup("tag-update");
    reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
    will_return(updateObject, -1);
    expect_function_call(updateObject);
    
    ret = processCrudRequest(reqMsg, &response);
    assert_int_equal(ret, -1);

	wrp_free_struct(reqMsg);

}

void test_processCrudRequestDelete()
{
	int ret = -2;
	wrp_msg_t *reqMsg = NULL;
    reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
    memset(reqMsg, 0, sizeof(wrp_msg_t));
    
    reqMsg->msg_type = 8;
    reqMsg->u.crud.transaction_uuid = strdup("1234");
    reqMsg->u.crud.source = strdup("tag-update");
    reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
    will_return(deleteObject, 0);
    expect_function_call(deleteObject);
    ret = processCrudRequest(reqMsg, &response);
    assert_int_equal(ret, 0);

	wrp_free_struct(reqMsg);

}

void test_processCrudRequestDeleteFailure()
{
	int ret = -2;
	wrp_msg_t *reqMsg = NULL;
    reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
    memset(reqMsg, 0, sizeof(wrp_msg_t));
    
    reqMsg->msg_type = 8;
    reqMsg->u.crud.transaction_uuid = strdup("1234");
    reqMsg->u.crud.source = strdup("tag-update");
    reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
    will_return(deleteObject, -1);
    expect_function_call(deleteObject);
    
    ret = processCrudRequest(reqMsg, &response);
    assert_int_equal(ret, -1);

	wrp_free_struct(reqMsg);

}

void test_processCrudRequestFailure()
{
	int ret = -2;
	wrp_msg_t *reqMsg = NULL;
    reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
    memset(reqMsg, 0, sizeof(wrp_msg_t));
    
    reqMsg->msg_type = 3;
    reqMsg->u.crud.transaction_uuid = strdup("1234");
    reqMsg->u.crud.source = strdup("tag-update");
    reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
    
    ret = processCrudRequest(reqMsg, &response);
    assert_int_equal(ret, 0);

	wrp_free_struct(reqMsg);

}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_processCrudRequestCreate),
        cmocka_unit_test(test_processCrudRequestCreateFailure),
        cmocka_unit_test(test_processCrudRequestRetrieve),
        cmocka_unit_test(test_processCrudRequestRetrieveFailure),
        cmocka_unit_test(test_processCrudRequestUpdate),
        cmocka_unit_test(test_processCrudRequestUpdateFailure),
        cmocka_unit_test(test_processCrudRequestDelete),
        cmocka_unit_test(test_processCrudRequestDeleteFailure),
        cmocka_unit_test(test_processCrudRequestFailure),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
