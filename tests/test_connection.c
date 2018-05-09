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
#include <stdarg.h>

#include <CUnit/Basic.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <assert.h>
#include <CUnit/Basic.h>
#include <nopoll.h>

#include "../src/ParodusInternal.h"
#include "../src/connection.h"
#include "../src/config.h"

extern void set_server_null (server_t *server);
extern void set_server_list_null (server_list_t *server_list);
extern int server_is_null (server_t *server);
extern server_t *get_current_server (server_list_t *server_list);
extern int parse_server_url (const char *full_url, server_t *server);
extern void init_expire_timer (expire_timer_t *timer);
extern int check_timer_expired (expire_timer_t *timer, long timeout_ms);
extern void init_backoff_timer (backoff_timer_t *timer, int max_delay);
extern int update_backoff_delay (backoff_timer_t *timer);
extern void init_header_info (header_info_t *header_info);
extern void free_header_info (header_info_t *header_info);
extern char *build_extra_hdrs (header_info_t *header_info);

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

bool close_retry;
bool LastReasonStatus;
volatile unsigned int heartBeatTimer;
pthread_mutex_t close_mut;

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
char* getWebpaConveyHeader()
{
    return (char*) "WebPA-1.6 (TG1682)";
}

int checkHostIp(char * serverIP)
{
    UNUSED(serverIP);
    return 0;
}

void setMessageHandlers()
{
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_get_global_conn()
{
    assert_null(get_global_conn());
}

void test_set_global_conn()
{
    static noPollConn *gNPConn;
    set_global_conn(gNPConn);
    assert_ptr_equal(gNPConn, get_global_conn());
}

void test_get_global_reconnect_reason()
{
    assert_string_equal("webpa_process_starts", get_global_reconnect_reason());
}

void test_set_global_reconnect_reason()
{
    char *reason = "Factory-Reset";
    set_global_reconnect_reason(reason);
    assert_string_equal(reason, get_global_reconnect_reason());
}

void test_closeConnection()
{
    close_and_unref_connection(get_global_conn());
}

void test_server_is_null()
{
  server_t test_server;
  memset (&test_server, 0xFF, sizeof(test_server));
  assert_int_equal (0, server_is_null (&test_server));
  set_server_null (&test_server);
  assert_int_equal (1, server_is_null (&test_server));
}

void test_server_list_null()
{
  server_list_t server_list;
  memset (&server_list, 0xFF, sizeof(server_list));
  assert_int_equal (0, server_is_null (&server_list.defaults));
  assert_int_equal (0, server_is_null (&server_list.jwt));
  assert_int_equal (0, server_is_null (&server_list.redirect));
  set_server_list_null (&server_list);
  assert_int_equal (1, server_is_null (&server_list.defaults));
  assert_int_equal (1, server_is_null (&server_list.jwt));
  assert_int_equal (1, server_is_null (&server_list.redirect));
 
}

void test_get_current_server()
{
  server_list_t server_list;
  memset (&server_list, 0xFF, sizeof(server_list));
  assert_ptr_equal (&server_list.redirect, get_current_server (&server_list));
  set_server_null (&server_list.redirect);
  assert_ptr_equal (&server_list.jwt, get_current_server (&server_list));
  set_server_null (&server_list.jwt);
  assert_ptr_equal (&server_list.defaults, get_current_server (&server_list));
}

void test_parse_server_url ()
{
	server_t test_server;
	assert_int_equal (parse_server_url ("mydns.mycom.net:8080",
		&test_server), -1);
	assert_int_equal (-1, test_server.allow_insecure);
	assert_int_equal (parse_server_url ("https://mydns.mycom.net:8080",
		&test_server), 0);
	assert_string_equal (test_server.server_addr, "mydns.mycom.net");
	assert_int_equal (test_server.port, 8080);
	assert_int_equal (0, test_server.allow_insecure);
	assert_int_equal (parse_server_url ("https://mydns.mycom.net/",
		&test_server), 0);
	assert_string_equal (test_server.server_addr, "mydns.mycom.net");
	assert_int_equal (test_server.port, 443);
	assert_int_equal (0, test_server.allow_insecure);
	assert_int_equal (parse_server_url ("http://mydns.mycom.net:8080",
		&test_server), 1);
	assert_string_equal (test_server.server_addr, "mydns.mycom.net");
	assert_int_equal (test_server.port, 8080);
	assert_int_equal (1, test_server.allow_insecure);
	assert_int_equal (parse_server_url ("http://mydns.mycom.net",
		&test_server), 1);
	assert_string_equal (test_server.server_addr, "mydns.mycom.net");
	assert_int_equal (test_server.port, 80);
	assert_int_equal (1, test_server.allow_insecure);
}

void test_expire_timer()
{
  expire_timer_t exp_timer;
  init_expire_timer (&exp_timer);
  assert_int_equal (false, check_timer_expired (&exp_timer, 1000));
  sleep (2);
  assert_int_equal (false, check_timer_expired (&exp_timer, 3000));
  assert_int_equal (true, check_timer_expired (&exp_timer, 1000));
}

void test_backoff_delay_timer()
{
  backoff_timer_t btimer;
  init_backoff_timer (&btimer, 30);
  assert_int_equal (3, update_backoff_delay (&btimer));
  assert_int_equal (7, update_backoff_delay (&btimer));
  assert_int_equal (15, update_backoff_delay (&btimer));
  assert_int_equal (30, update_backoff_delay (&btimer));
  assert_int_equal (30, update_backoff_delay (&btimer));
}


void test_header_info ()
{
    header_info_t hinfo;
    ParodusCfg Cfg;
    memset(&Cfg, 0, sizeof(ParodusCfg));

    parStrncpy(Cfg.hw_model, "TG1682", sizeof(Cfg.hw_model));
    parStrncpy(Cfg.hw_manufacturer , "ARRISGroup,Inc.", sizeof(Cfg.hw_manufacturer));
    parStrncpy(Cfg.hw_mac , "123567892366", sizeof(Cfg.hw_mac));
    parStrncpy(Cfg.fw_name , "2.364s2", sizeof(Cfg.fw_name));
    parStrncpy(Cfg.webpa_protocol , "WebPA-1.6", sizeof(Cfg.webpa_protocol));
    set_parodus_cfg(&Cfg);

    init_header_info (&hinfo);
    assert_string_equal (hinfo.conveyHeader, "WebPA-1.6 (TG1682)");
    assert_string_equal (hinfo.device_id, "mac:123567892366");
    assert_string_equal (hinfo.user_agent,
      "WebPA-1.6 (2.364s2; TG1682/ARRISGroup,Inc.;)");
    free_header_info (&hinfo);
}

void test_extra_headers ()
{
    header_info_t hinfo;
    ParodusCfg Cfg;
    const char *expected_extra_headers =
      "\r\nAuthorization: Bearer Auth---"
      "\r\nX-WebPA-Device-Name: mac:123567892366"
      "\r\nX-WebPA-Device-Protocols: wrp-0.11,getset-0.1"
      "\r\nUser-Agent: WebPA-1.6 (2.364s2; TG1682/ARRISGroup,Inc.;)"
      "\r\nX-WebPA-Convey: WebPA-1.6 (TG1682)";
    char *extra_headers;
    
    memset(&Cfg, 0, sizeof(ParodusCfg));

    parStrncpy (Cfg.webpa_auth_token, "Auth---", sizeof (Cfg.webpa_auth_token));
    parStrncpy(Cfg.hw_model, "TG1682", sizeof(Cfg.hw_model));
    parStrncpy(Cfg.hw_manufacturer , "ARRISGroup,Inc.", sizeof(Cfg.hw_manufacturer));
    parStrncpy(Cfg.hw_mac , "123567892366", sizeof(Cfg.hw_mac));
    parStrncpy(Cfg.fw_name , "2.364s2", sizeof(Cfg.fw_name));
    parStrncpy(Cfg.webpa_protocol , "WebPA-1.6", sizeof(Cfg.webpa_protocol));
    set_parodus_cfg(&Cfg);

    init_header_info (&hinfo);
    extra_headers = build_extra_hdrs (&hinfo);
    assert_string_equal (extra_headers, expected_extra_headers);
    free (extra_headers);
    free_header_info (&hinfo);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_get_global_conn),
        cmocka_unit_test(test_set_global_conn),
        cmocka_unit_test(test_get_global_reconnect_reason),
        cmocka_unit_test(test_set_global_reconnect_reason),
        cmocka_unit_test(test_closeConnection),
        cmocka_unit_test(test_server_is_null),
        cmocka_unit_test(test_server_list_null),
        cmocka_unit_test(test_get_current_server),
        cmocka_unit_test(test_parse_server_url),
        cmocka_unit_test(test_expire_timer),
        cmocka_unit_test(test_backoff_delay_timer),
        cmocka_unit_test(test_header_info),
        cmocka_unit_test(test_extra_headers)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
