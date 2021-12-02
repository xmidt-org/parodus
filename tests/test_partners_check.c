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

#include "../src/config.h"
#include "../src/partners_check.h"
#include "../src/ParodusInternal.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
ParodusCfg *get_parodus_cfg(void)
{
    function_called();
    return (ParodusCfg*) (intptr_t)mock();
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_validate_partner_id_for_req()
{
    static partners_t partner_ids = {3,{"shaw","","comcast"}};
    wrp_msg_t *msg = (wrp_msg_t*) malloc(sizeof(wrp_msg_t));
    memset(msg, 0, sizeof(wrp_msg_t));
    msg->msg_type = WRP_MSG_TYPE__REQ;
    msg->u.req.partner_ids = &partner_ids;
    
    ParodusCfg cfg;
    memset(&cfg, 0, sizeof(ParodusCfg));
    parStrncpy(cfg.partner_id, "shaw,bar,comcast", sizeof(cfg.partner_id));
    
    will_return(get_parodus_cfg, (intptr_t)&cfg);
    expect_function_call(get_parodus_cfg);
    int ret = validate_partner_id(msg, NULL); 
    assert_int_equal(ret, 1);
    free(msg);
}

void test_validate_partner_id_for_req_case_insensitive()
{
    static partners_t partner_ids = {2,{"Shaw","Comcast"}};
    wrp_msg_t *msg = (wrp_msg_t*) malloc(sizeof(wrp_msg_t));
    memset(msg, 0, sizeof(wrp_msg_t));
    msg->msg_type = WRP_MSG_TYPE__REQ;
    msg->u.req.partner_ids = &partner_ids;
    
    ParodusCfg cfg;
    memset(&cfg, 0, sizeof(ParodusCfg));
    parStrncpy(cfg.partner_id, "comcast", sizeof(cfg.partner_id));
    
    will_return(get_parodus_cfg, (intptr_t)&cfg);
    expect_function_call(get_parodus_cfg);
    int ret = validate_partner_id(msg, NULL); 
    assert_int_equal(ret, 1);
    free(msg);
}

void test_validate_partner_id_for_req_listNULL()
{
    wrp_msg_t *msg = (wrp_msg_t*) malloc(sizeof(wrp_msg_t));
    memset(msg, 0, sizeof(wrp_msg_t));
    msg->msg_type = WRP_MSG_TYPE__REQ;
    
    ParodusCfg cfg;
    memset(&cfg, 0, sizeof(ParodusCfg));
    parStrncpy(cfg.partner_id, "*,comcast", sizeof(cfg.partner_id));
    
    will_return(get_parodus_cfg, (intptr_t)&cfg);
    expect_function_call(get_parodus_cfg);
    int ret = validate_partner_id(msg, NULL); 
    assert_int_equal(ret, 1);
    free(msg);
}

void test_validate_partner_id_for_req_withoutId()
{
    wrp_msg_t *msg = (wrp_msg_t*) malloc(sizeof(wrp_msg_t));
    memset(msg, 0, sizeof(wrp_msg_t));
    msg->msg_type = WRP_MSG_TYPE__REQ;
    
    ParodusCfg cfg;
    memset(&cfg, 0, sizeof(ParodusCfg));
    
    will_return(get_parodus_cfg, (intptr_t)&cfg);
    expect_function_call(get_parodus_cfg);
    int ret = validate_partner_id(msg, NULL); 
    assert_int_equal(ret, 0);
    free(msg);
}

void err_validate_partner_id_for_req()
{
    static partners_t partner_ids = {1,{"shaw"}};
    wrp_msg_t *msg = (wrp_msg_t*) malloc(sizeof(wrp_msg_t));
    memset(msg, 0, sizeof(wrp_msg_t));
    msg->msg_type = WRP_MSG_TYPE__REQ;
    msg->u.req.partner_ids = &partner_ids;
    
    ParodusCfg cfg;
    memset(&cfg, 0, sizeof(ParodusCfg));
    parStrncpy(cfg.partner_id, "*,,comcast", sizeof(cfg.partner_id));
    
    will_return(get_parodus_cfg, (intptr_t)&cfg);
    expect_function_call(get_parodus_cfg);
    int ret = validate_partner_id(msg, NULL); 
    assert_int_equal(ret, -1);
    free(msg);
}

void test_validate_partner_id_for_event()
{
    static partners_t partner_ids = {4,{"shaw","","*","comcast"}};
    wrp_msg_t *msg = (wrp_msg_t*) malloc(sizeof(wrp_msg_t));
    memset(msg, 0, sizeof(wrp_msg_t));
    msg->msg_type = WRP_MSG_TYPE__EVENT;
    msg->u.event.partner_ids = &partner_ids;
    
    ParodusCfg cfg;
    memset(&cfg, 0, sizeof(ParodusCfg));
    parStrncpy(cfg.partner_id, "abc,*,comcast", sizeof(cfg.partner_id));
    
    will_return(get_parodus_cfg, (intptr_t)&cfg);
    expect_function_call(get_parodus_cfg);
    
    partners_t *list = NULL;
    int ret = validate_partner_id(msg, &list); 
    assert_int_equal(ret, 1);
    free(list);  
    free(msg);
}

void test_validate_partner_id_for_event_case_insensitive()
{
    static partners_t partner_ids = {1,{"Comcast"}};
    wrp_msg_t *msg = (wrp_msg_t*) malloc(sizeof(wrp_msg_t));
    memset(msg, 0, sizeof(wrp_msg_t));
    msg->msg_type = WRP_MSG_TYPE__EVENT;
    msg->u.event.partner_ids = &partner_ids;
    
    ParodusCfg cfg;
    memset(&cfg, 0, sizeof(ParodusCfg));
    parStrncpy(cfg.partner_id, "*,comcast", sizeof(cfg.partner_id));
    
    will_return(get_parodus_cfg, (intptr_t)&cfg);
    expect_function_call(get_parodus_cfg);
    
    partners_t *list = NULL;
    int ret = validate_partner_id(msg, &list); 
    assert_int_equal(ret, 1);
    free(list);  
    free(msg);
}

void test_validate_partner_id_for_event_listNULL()
{
    wrp_msg_t *msg = (wrp_msg_t*) malloc(sizeof(wrp_msg_t));
    memset(msg, 0, sizeof(wrp_msg_t));
    msg->msg_type = WRP_MSG_TYPE__EVENT;
    
    ParodusCfg cfg;
    memset(&cfg, 0, sizeof(ParodusCfg));
    parStrncpy(cfg.partner_id, "comcast", sizeof(cfg.partner_id));
    
    will_return(get_parodus_cfg, (intptr_t)&cfg);
    expect_function_call(get_parodus_cfg);
    partners_t *list = NULL;
    int ret = validate_partner_id(msg, &list); 
    assert_int_equal(ret, 1); 
    assert_int_equal(list->count, 1);
    assert_string_equal(list->partner_ids[0], "comcast");
    int i;
    for(i = 0; i< (int) list->count; i++)
    {
        free(list->partner_ids[i]);
    }
    free(list);
    free(msg);
}

void err_validate_partner_id_for_event()
{
    wrp_msg_t *msg = (wrp_msg_t*) malloc(sizeof(wrp_msg_t));
    memset(msg, 0, sizeof(wrp_msg_t));
    msg->msg_type = WRP_MSG_TYPE__EVENT;
    
    ParodusCfg cfg;
    memset(&cfg, 0, sizeof(ParodusCfg));
    
    will_return(get_parodus_cfg, (intptr_t)&cfg);
    expect_function_call(get_parodus_cfg);
    int ret = validate_partner_id(msg, NULL); 
    assert_int_equal(ret, 0);
    free(msg);  
}

void test_validate_partner_id_for_event_withoutId()
{
    partners_t *partner_ids = (partners_t *) malloc(sizeof(partners_t));
    partner_ids->count = 1;
    partner_ids->partner_ids[0] = (char *) malloc(sizeof(char)*64);
    parStrncpy(partner_ids->partner_ids[0], "shaw", 64);

    wrp_msg_t *msg = (wrp_msg_t*) malloc(sizeof(wrp_msg_t));
    memset(msg, 0, sizeof(wrp_msg_t));
    msg->msg_type = WRP_MSG_TYPE__EVENT;
    msg->u.event.partner_ids = partner_ids;
    
    ParodusCfg cfg;
    memset(&cfg, 0, sizeof(ParodusCfg));
    parStrncpy(cfg.partner_id, "comcast", sizeof(cfg.partner_id));
    
    will_return(get_parodus_cfg, (intptr_t)&cfg);
    expect_function_call(get_parodus_cfg);
    partners_t *list = NULL;
    int ret = validate_partner_id(msg, &list); 
    assert_int_equal(ret, 1); 
    assert_int_equal(list->count, 2);
    assert_string_equal(list->partner_ids[0], "shaw");
    assert_string_equal(list->partner_ids[1], "comcast");
    int i;
    for(i = 0; i< (int) list->count; i++)
    {
        free(list->partner_ids[i]);
    }
    free(list);
    free(msg);
    free(partner_ids);
}

void test_partner_validate_partner_id_for_req()
{   
    static partners_t partner_ids = {1,{"comcast"}};
    wrp_msg_t *msg = (wrp_msg_t*) malloc(sizeof(wrp_msg_t));
    memset(msg, 0, sizeof(wrp_msg_t));
    msg->msg_type = WRP_MSG_TYPE__REQ;
    msg->u.req.partner_ids = &partner_ids;
    
    ParodusCfg cfg; 
    memset(&cfg, 0, sizeof(ParodusCfg));
    parStrncpy(cfg.partner_id, "*,test-partner", sizeof(cfg.partner_id));
    
    will_return(get_parodus_cfg, (intptr_t)&cfg);
    expect_function_call(get_parodus_cfg);
    int ret = validate_partner_id(msg, NULL);
    assert_int_equal(ret, 1);
    free(msg);
}

void test_partner_wildcard_validate_partner_id_for_req()
{   
    static partners_t partner_ids = {1,{"*"}};
    wrp_msg_t *msg = (wrp_msg_t*) malloc(sizeof(wrp_msg_t));
    memset(msg, 0, sizeof(wrp_msg_t));
    msg->msg_type = WRP_MSG_TYPE__REQ;
    msg->u.req.partner_ids = &partner_ids;
    
    ParodusCfg cfg;
    memset(&cfg, 0, sizeof(ParodusCfg));
    parStrncpy(cfg.partner_id, "*,test-partner", sizeof(cfg.partner_id));
    
    will_return(get_parodus_cfg, (intptr_t)&cfg);
    expect_function_call(get_parodus_cfg);
    int ret = validate_partner_id(msg, NULL);
    assert_int_equal(ret, 1);
    free(msg);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_validate_partner_id_for_req),
	cmocka_unit_test(test_validate_partner_id_for_req_case_insensitive),
        cmocka_unit_test(test_validate_partner_id_for_req_listNULL),
        cmocka_unit_test(test_validate_partner_id_for_req_withoutId),
        cmocka_unit_test(err_validate_partner_id_for_req),
        cmocka_unit_test(test_validate_partner_id_for_event),
	cmocka_unit_test(test_validate_partner_id_for_event_case_insensitive),
        cmocka_unit_test(test_validate_partner_id_for_event_listNULL),
        cmocka_unit_test(test_validate_partner_id_for_event_withoutId),
        cmocka_unit_test(err_validate_partner_id_for_event),
	cmocka_unit_test(test_partner_validate_partner_id_for_req),
	cmocka_unit_test(test_partner_wildcard_validate_partner_id_for_req),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
