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
#include "../src/crud_interface.h"
#include "../src/config.h"
#include "../src/client_list.h"
#include "../src/ParodusInternal.h"
#include "../src/partners_check.h"


/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
extern CrudMsg *crudMsgQ;
int numLoops = 1;
wrp_msg_t *temp = NULL;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

char * get_global_UpStreamMsgQ(void)
{
    function_called();
    return (char *)mock();
}

int set_global_UpStreamMsgQ(void)
{
    function_called();
    return (int)mock();
}

int get_global_nano_con(void)
{
    function_called();
    return (int)mock();
}

int get_global_nano_mut(void)
{
    function_called();
    return (int)mock();
}

char *get_global_reconnect_reason()
{
    function_called();
    return (char *)(intptr_t)mock();
}	

ParodusCfg *get_parodus_cfg(void)
{
    function_called();
    return (ParodusCfg*) (intptr_t)mock();
}

int pthread_mutex_lock(pthread_mutex_t *restrict mutex)
{
    UNUSED(mutex);
    function_called();
    return (int)mock();
}

int pthread_mutex_unlock(pthread_mutex_t *restrict mutex)
{
    UNUSED(mutex);
    function_called();
    return (int)mock();
}

int pthread_cond_signal(pthread_cond_t *restrict cond)
{
    UNUSED(cond); 
    function_called();
    return (int)mock();
}

int pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex)
{
    UNUSED(cond); UNUSED(mutex);
    function_called();
    return (int)mock();
}


int processCrudRequest(wrp_msg_t *reqMsg, wrp_msg_t **responseMsg )
{
	UNUSED(reqMsg);
	wrp_msg_t *resp_msg = NULL;
    resp_msg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
    memset(resp_msg, 0, sizeof(wrp_msg_t));
    
    resp_msg->msg_type = 5;
    resp_msg->u.crud.transaction_uuid = strdup("1234");
    resp_msg->u.crud.source = strdup("tag-update");
    resp_msg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
    *responseMsg = resp_msg;
	function_called();
	return (int)mock();
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_addCRUDmsgToQueue()
{

	wrp_msg_t *crud_msg = NULL;
	crud_msg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
	memset(crud_msg, 0, sizeof(wrp_msg_t));

	crud_msg->msg_type = 5;
	crud_msg->u.crud.transaction_uuid = strdup("bd4ad2d1-5c9c-486f-8e25-52c242b38f71");
    crud_msg->u.crud.source = strdup("tag-update");
    crud_msg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
	
    will_return(pthread_mutex_lock, 0);
    expect_function_call(pthread_mutex_lock);
    
    will_return(pthread_mutex_unlock, 0);
    expect_function_call(pthread_mutex_unlock);
    
    crudMsgQ = (CrudMsg *)malloc(sizeof(CrudMsg));
	crudMsgQ->msg = crud_msg;
	
	crudMsgQ->next = (CrudMsg *) malloc(sizeof(CrudMsg));
    crudMsgQ->next->msg = crud_msg;
    crudMsgQ->next->next = NULL;
    
	addCRUDmsgToQueue(crud_msg);
	
	free(crudMsgQ->next);
    free(crudMsgQ);
    crudMsgQ = NULL;
    
    wrp_free_struct(crud_msg);
	
}


void test_addCRUDmsgToQueueNULL()
{
	wrp_msg_t *crud_msg = NULL;
	crud_msg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
	memset(crud_msg, 0, sizeof(wrp_msg_t));

	crud_msg->msg_type = 5;
	crud_msg->u.crud.transaction_uuid = strdup("bd4ad2d1-5c9c-486f-8e25-52c242b38f71");
    crud_msg->u.crud.source = strdup("tag-update");
    crud_msg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
	
    will_return(pthread_mutex_lock, 0);
    expect_function_call(pthread_mutex_lock);
    
    will_return(pthread_cond_signal, 0);
    expect_function_call(pthread_cond_signal);
    
    will_return(pthread_mutex_unlock, 0);
    expect_function_call(pthread_mutex_unlock);
    
	addCRUDmsgToQueue(crud_msg);
	
	free(crudMsgQ->next);
    free(crudMsgQ);
    crudMsgQ = NULL;
    wrp_free_struct(crud_msg);
	
}

void err_CRUDHandlerTask()
{
    numLoops = 1;
    crudMsgQ = NULL;
    
    will_return(pthread_mutex_lock, 0);
    expect_function_call(pthread_mutex_lock);
    
    will_return(pthread_cond_wait, 0);
    expect_function_call(pthread_cond_wait);
    
    will_return(pthread_mutex_unlock, 0);
    expect_function_call(pthread_mutex_unlock);
    
    CRUDHandlerTask();
}


void test_CRUDHandlerTask()
{
	numLoops = 1;
	wrp_msg_t *crud_msg = NULL;
	crud_msg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
	memset(crud_msg, 0, sizeof(wrp_msg_t));

	crud_msg->msg_type = 5;
	crud_msg->u.crud.transaction_uuid = strdup("bd4ad2d1-5c9c-486f-8e25-52c242b38f71");
    crud_msg->u.crud.source = strdup("tag-update");
    crud_msg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
    
    crudMsgQ = (CrudMsg *)malloc(sizeof(CrudMsg));
	crudMsgQ->msg = crud_msg;
	crudMsgQ->next = NULL;
	
	will_return(pthread_mutex_lock, 0);
    expect_function_call(pthread_mutex_lock);
    
    will_return(pthread_mutex_unlock, 0);
    expect_function_call(pthread_mutex_unlock);
    
    will_return(processCrudRequest, 0);
    expect_function_call(processCrudRequest);
    
    will_return(get_global_nano_mut, 0);
    expect_function_call(get_global_nano_mut);
    
    will_return(pthread_mutex_lock, 0);
    expect_function_call(pthread_mutex_lock);
    
    will_return(get_global_UpStreamMsgQ, NULL);
    expect_function_call(get_global_UpStreamMsgQ);
    
    will_return(set_global_UpStreamMsgQ, 0);
    expect_function_call(set_global_UpStreamMsgQ);
    
    will_return(get_global_nano_con, 0);
    expect_function_call(get_global_nano_con);
    
    will_return(pthread_cond_signal, 0);
    expect_function_call(pthread_cond_signal);
    
    will_return(get_global_nano_mut, 0);
    expect_function_call(get_global_nano_mut);
    
    will_return(pthread_mutex_unlock, 0);
    expect_function_call(pthread_mutex_unlock);
    
	CRUDHandlerTask();
	
    free(crudMsgQ);
    wrp_free_struct(crud_msg);
	
}

void test_CRUDHandlerTaskFailure()
{
	numLoops = 1;
	wrp_msg_t *crud_msg = NULL;
	
	crud_msg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
	memset(crud_msg, 0, sizeof(wrp_msg_t));

	crud_msg->msg_type = 5;
	crud_msg->u.crud.transaction_uuid = strdup("bd4ad2d1-5c9c-486f-8e25-52c242b38f71");
    crud_msg->u.crud.source = strdup("tag-update");
    crud_msg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
    
    crudMsgQ = (CrudMsg *)malloc(sizeof(CrudMsg));
	crudMsgQ->msg = crud_msg;
	crudMsgQ->next = NULL;
	
	will_return(pthread_mutex_lock, 0);
    expect_function_call(pthread_mutex_lock);
    
    will_return(pthread_mutex_unlock, 0);
    expect_function_call(pthread_mutex_unlock);
    
    will_return(processCrudRequest, -1);
    expect_function_call(processCrudRequest);
    
	CRUDHandlerTask();
	
	free(crudMsgQ);
    wrp_free_struct(crud_msg);
	
}

void test_addCRUDresponseToUpstreamQ()
{
	numLoops = 1;
	ssize_t resp_size = 0;
	void *resp_bytes;
	
	wrp_msg_t *resp_msg = NULL;
    resp_msg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
    memset(resp_msg, 0, sizeof(wrp_msg_t));
    
    resp_msg->msg_type = 5;
    resp_msg->u.crud.transaction_uuid = strdup("1234");
    resp_msg->u.crud.source = strdup("tag-update");
    resp_msg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
    
    resp_size = wrp_struct_to( resp_msg, WRP_BYTES, &resp_bytes );
	
	will_return(get_global_nano_mut, 0);
    expect_function_call(get_global_nano_mut);
	
	will_return(pthread_mutex_lock, 0);
    expect_function_call(pthread_mutex_lock);
    
    will_return(get_global_UpStreamMsgQ, 0);
    expect_function_call(get_global_UpStreamMsgQ);
    
    will_return(set_global_UpStreamMsgQ, 0);
    expect_function_call(set_global_UpStreamMsgQ);
    
    will_return(get_global_nano_con, 0);
    expect_function_call(get_global_nano_con);
    
    will_return(pthread_cond_signal, 0);
    expect_function_call(pthread_cond_signal);
    
    will_return(get_global_nano_mut, 0);
    expect_function_call(get_global_nano_mut);
    
    will_return(pthread_mutex_unlock, 0);
    expect_function_call(pthread_mutex_unlock);
    
	addCRUDresponseToUpstreamQ(resp_bytes, resp_size);
	
    wrp_free_struct(resp_msg);
	
}


void test_addCRUDmsgToQueueAllocation()
{
	addCRUDmsgToQueue(NULL);
}
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_addCRUDmsgToQueueNULL),
        cmocka_unit_test(test_addCRUDmsgToQueue),
        cmocka_unit_test(test_addCRUDmsgToQueueAllocation),
        cmocka_unit_test(err_CRUDHandlerTask),
        cmocka_unit_test(test_CRUDHandlerTask),
        cmocka_unit_test(test_CRUDHandlerTaskFailure),
        cmocka_unit_test(test_addCRUDresponseToUpstreamQ)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
