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
extern int init_header_info (header_info_t *header_info);
extern void free_header_info (header_info_t *header_info);
extern char *build_extra_hdrs (header_info_t *header_info);
extern void set_current_server (create_connection_ctx_t *ctx);
extern void set_extra_headers (create_connection_ctx_t *ctx, int reauthorize);
extern void free_extra_headers (create_connection_ctx_t *ctx);
extern void free_server_list (server_list_t *server_list);
extern void free_server (server_t *server);
extern int find_servers (server_list_t *server_list);
extern int nopoll_connect (create_connection_ctx_t *ctx, int is_ipv6);
extern int wait_connection_ready (create_connection_ctx_t *ctx);
extern int connect_and_wait (create_connection_ctx_t *ctx);
extern int keep_trying_to_connect (create_connection_ctx_t *ctx, 
	int max_retry_sleep);


/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

bool close_retry;
bool LastReasonStatus;
pthread_mutex_t close_mut;

// Mock values
char *mock_server_addr;
unsigned int mock_port;
int mock_wait_status;
char *mock_redirect;
noPollConn connection1;
noPollConn connection2;
noPollConn connection3;

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
char* getWebpaConveyHeader()
{
    return (char*) "WebPA-1.6 (TG1682)";
}

noPollConn * nopoll_conn_new_opts (noPollCtx  * ctx, noPollConnOpts  * opts, const char  * host_ip, const char  * host_port, const char  * host_name,const char  * get_url,const char  * protocols, const char * origin)
{
    UNUSED(host_port); UNUSED(host_name); UNUSED(get_url); UNUSED(protocols); 
    UNUSED(origin); UNUSED(opts); UNUSED(ctx); UNUSED(host_ip);
    
    function_called();
    //check_expected((intptr_t)ctx);
    //check_expected((intptr_t)host_ip);
    return (noPollConn *) (intptr_t)mock();
}

noPollConn * nopoll_conn_tls_new (noPollCtx  * ctx, noPollConnOpts * options, const char * host_ip, const char * host_port, const char * host_name, const char * get_url, const char * protocols, const char * origin)
{
    UNUSED(options); UNUSED(host_port); UNUSED(host_name); UNUSED(get_url); UNUSED(protocols); 
    UNUSED(origin); UNUSED(ctx); UNUSED(host_ip);
    
    function_called();
    //check_expected((intptr_t)ctx);
    //check_expected((intptr_t)host_ip);
    return (noPollConn *) (intptr_t)mock();
}

noPollConn * nopoll_conn_tls_new6 (noPollCtx  * ctx, noPollConnOpts * options, const char * host_ip, const char * host_port, const char * host_name, const char * get_url, const char * protocols, const char * origin)
{
    UNUSED(options); UNUSED(host_port); UNUSED(host_name); UNUSED(get_url); UNUSED(protocols); 
    UNUSED(origin); UNUSED(ctx); UNUSED(host_ip);
    
    function_called();
    //check_expected((intptr_t)ctx);
    //check_expected((intptr_t)host_ip);
    return (noPollConn *) (intptr_t)mock();
}

nopoll_bool  nopoll_conn_wait_for_status_until_connection_ready (noPollConn * conn,
	int timeout, int *status, char ** message)
{
    UNUSED(conn); UNUSED(timeout);
    if (mock_wait_status >= 1000) {
      *status = mock_wait_status / 1000;
      mock_wait_status = mock_wait_status % 1000;
    } else {
      *status = mock_wait_status;
    }
    if ((NULL != mock_redirect) &&
        ((*status == 307) || (*status == 302) || 
         (*status == 303)) )
    {
      *message = malloc (strlen(mock_redirect) + 10);
      sprintf (*message, "Redirect:%s", mock_redirect);
    }
    function_called();
    return (nopoll_bool) mock();
}

nopoll_bool nopoll_conn_is_ok (noPollConn * conn)
{
    UNUSED(conn);
    function_called ();
    return (nopoll_bool) mock();
}

void nopoll_conn_close (noPollConn *conn)
{
    UNUSED(conn);
}

int nopoll_conn_ref_count (noPollConn *conn)
{
    UNUSED(conn);
    function_called ();
    return (nopoll_bool) mock();
}

int checkHostIp(char * serverIP)
{
    UNUSED(serverIP);
    function_called();
    return (int) mock();
}

int kill(pid_t pid, int sig)
{
    UNUSED(pid); UNUSED(sig);
    function_called();
    return (int) mock();
}

void setMessageHandlers()
{
}

int allow_insecure_conn (char **server_addr, unsigned int *port)
{
  int rtn;
	
  function_called ();
  rtn = (int) mock();
  if (rtn < 0) {
    *server_addr = NULL;
    return rtn;
  }
  
  if (NULL != mock_server_addr) {
    size_t len = strlen(mock_server_addr) + 1;
    *server_addr = malloc (len);
    if (NULL != *server_addr) {
      strncpy (*server_addr, mock_server_addr, len);
    }
  }

  *port = mock_port;
  return rtn;		
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

void test_set_current_server()
{
  create_connection_ctx_t ctx;
  memset (&ctx, 0xFF, sizeof(ctx));
  set_current_server (&ctx);
  assert_ptr_equal (&ctx.server_list.redirect, ctx.current_server);
  set_server_null (&ctx.server_list.redirect);
  set_current_server (&ctx);
  assert_ptr_equal (&ctx.server_list.jwt, ctx.current_server);
  set_server_null (&ctx.server_list.jwt);
  set_current_server (&ctx);
  assert_ptr_equal (&ctx.server_list.defaults, ctx.current_server);
}

void test_set_extra_headers ()
{
  int rtn;
  create_connection_ctx_t ctx;
  ParodusCfg cfg;
  const char *expected_extra_headers =
      "\r\nAuthorization: Bearer SER_MAC Fer23u948590 123567892366"
      "\r\nX-WebPA-Device-Name: mac:123567892366"
      "\r\nX-WebPA-Device-Protocols: wrp-0.11,getset-0.1"
      "\r\nUser-Agent: WebPA-1.6 (2.364s2; TG1682/ARRISGroup,Inc.;)"
      "\r\nX-WebPA-Convey: WebPA-1.6 (TG1682)";

  memset(&cfg,0,sizeof(cfg));
  memset (&ctx, 0, sizeof(ctx));

  parStrncpy (cfg.token_acquisition_script, "../../tests/return_success.bsh",
	sizeof(cfg.token_acquisition_script));    
  parStrncpy (cfg.token_read_script, "../../tests/return_ser_mac.bsh",
	sizeof(cfg.token_read_script));
  parStrncpy(cfg.hw_serial_number, "Fer23u948590", sizeof(cfg.hw_serial_number));
  parStrncpy(cfg.hw_mac , "123567892366", sizeof(cfg.hw_mac));
  parStrncpy(cfg.hw_model, "TG1682", sizeof(cfg.hw_model));
  parStrncpy(cfg.hw_manufacturer , "ARRISGroup,Inc.", sizeof(cfg.hw_manufacturer));
  parStrncpy(cfg.fw_name , "2.364s2", sizeof(cfg.fw_name));
  parStrncpy(cfg.webpa_protocol , "WebPA-1.6", sizeof(cfg.webpa_protocol));
	
  set_parodus_cfg(&cfg);
  rtn = init_header_info (&ctx.header_info);
  assert_int_equal (rtn, 0);
  assert_string_equal (ctx.header_info.device_id, "mac:123567892366");
  assert_string_equal (ctx.header_info.user_agent,
    "WebPA-1.6 (2.364s2; TG1682/ARRISGroup,Inc.;)");
  assert_string_equal (ctx.header_info.conveyHeader, "WebPA-1.6 (TG1682)");
  set_extra_headers (&ctx, true);
  
  assert_string_equal (get_parodus_cfg()->webpa_auth_token, 
    "SER_MAC Fer23u948590 123567892366");
  assert_string_equal (ctx.extra_headers, expected_extra_headers);
  free (ctx.extra_headers);
  free_header_info (&ctx.header_info);
	
}

void test_find_servers ()
{
  ParodusCfg cfg;
  server_list_t server_list;
  server_t *default_server = &server_list.defaults;
#ifdef FEATURE_DNS_QUERY
  server_t *jwt_server = &server_list.jwt;
#endif
  
  memset(&cfg,0,sizeof(cfg));
  set_server_list_null (&server_list);
  
  parStrncpy (cfg.webpa_url, "mydns.mycom.net:8080", sizeof(cfg.webpa_url));
  set_parodus_cfg(&cfg);
  assert_int_equal (find_servers(&server_list), FIND_INVALID_DEFAULT);
  assert_int_equal (default_server->allow_insecure, -1);
     
  parStrncpy (cfg.webpa_url, "https://mydns.mycom.net:8080", sizeof(cfg.webpa_url));

#ifdef FEATURE_DNS_QUERY
  cfg.acquire_jwt = 0;
  set_parodus_cfg(&cfg);
  assert_int_equal (find_servers(&server_list), FIND_SUCCESS);   
  assert_int_equal (default_server->allow_insecure, 0);  
  assert_string_equal (default_server->server_addr, "mydns.mycom.net");
  assert_int_equal (default_server->port, 8080);
  
  cfg.acquire_jwt = 1;
  mock_server_addr = "mydns.myjwtcom.net";
  mock_port = 80;
  set_parodus_cfg(&cfg);
  will_return (allow_insecure_conn, -1);
  expect_function_call (allow_insecure_conn);
  assert_int_equal (find_servers(&server_list), FIND_JWT_FAIL);   
  assert_int_equal (jwt_server->allow_insecure, -1);  

  will_return (allow_insecure_conn, 0);
  expect_function_call (allow_insecure_conn);
  assert_int_equal (find_servers(&server_list), FIND_SUCCESS);   
  assert_int_equal (jwt_server->allow_insecure, 0);
  assert_string_equal (jwt_server->server_addr, "mydns.myjwtcom.net");
  assert_int_equal (jwt_server->port, 80);
  
#else

  set_parodus_cfg(&cfg);
  assert_int_equal (find_servers(&server_list), FIND_SUCCESS);   
  assert_int_equal (default_server->allow_insecure, 0);
  assert_string_equal (default_server->server_addr, "mydns.mycom.net");
  assert_int_equal (default_server->port, 8080);

  free_server_list (&server_list);
#endif
}

void test_nopoll_connect ()
{
  ParodusCfg cfg;
  create_connection_ctx_t ctx;
  noPollCtx test_nopoll_ctx;
  server_t test_server;
  char *test_extra_headers =
      "\r\nAuthorization: Bearer SER_MAC Fer23u948590 123567892366"
      "\r\nX-WebPA-Device-Name: mac:123567892366"
      "\r\nX-WebPA-Device-Protocols: wrp-0.11,getset-0.1"
      "\r\nUser-Agent: WebPA-1.6 (2.364s2; TG1682/ARRISGroup,Inc.;)"
      "\r\nX-WebPA-Convey: WebPA-1.6 (TG1682)";

  ctx.nopoll_ctx = &test_nopoll_ctx;
  ctx.current_server = &test_server;
  ctx.extra_headers = test_extra_headers;
  memset(&cfg,0,sizeof(cfg));
  parStrncpy (cfg.webpa_url, "http://mydns.mycom.net:8080", sizeof(cfg.webpa_url));
  set_parodus_cfg(&cfg);
  init_expire_timer (&ctx.connect_timer);

  test_server.allow_insecure = 1;
  test_server.server_addr = "mydns.mycom.net";
  test_server.port = 8080;
  will_return (nopoll_conn_new_opts, &connection1);
  expect_function_call (nopoll_conn_new_opts);
  assert_int_equal (nopoll_connect (&ctx, true), 1);
  assert_ptr_equal(&connection1, get_global_conn());
 
  test_server.allow_insecure = 0;
  will_return (nopoll_conn_tls_new6, &connection2);
  expect_function_call (nopoll_conn_tls_new6);
  assert_int_equal (nopoll_connect (&ctx, true), 1);
  assert_ptr_equal(&connection2, get_global_conn());
  
  will_return (nopoll_conn_tls_new, &connection3);
  expect_function_call (nopoll_conn_tls_new);
  assert_int_equal (nopoll_connect (&ctx, false), 1);
  assert_ptr_equal(&connection3, get_global_conn());
  
  test_server.allow_insecure = 1;
  will_return (nopoll_conn_new_opts, NULL);
  expect_function_call (nopoll_conn_new_opts);
  will_return (checkHostIp, 0);
  expect_function_call (checkHostIp);
  assert_int_equal (nopoll_connect (&ctx, true), 0);
  assert_ptr_equal(NULL, get_global_conn());
  
  test_server.allow_insecure = 0;
  will_return (nopoll_conn_tls_new6, NULL);
  expect_function_call (nopoll_conn_tls_new6);
  will_return (checkHostIp, 0);
  expect_function_call (checkHostIp);
  assert_int_equal (nopoll_connect (&ctx, true), 0);
  assert_ptr_equal(NULL, get_global_conn());

  will_return (nopoll_conn_tls_new6, NULL);
  expect_function_call (nopoll_conn_tls_new6);
  will_return (checkHostIp, -2);
  expect_function_call (checkHostIp);
  assert_int_equal (nopoll_connect (&ctx, true), 0);
  assert_ptr_equal(NULL, get_global_conn());

  will_return (nopoll_conn_tls_new6, NULL);
  expect_function_call (nopoll_conn_tls_new6);
  will_return (checkHostIp, -2);
  expect_function_call (checkHostIp);
  ctx.connect_timer.start_time.tv_sec -= (15*60);
  will_return(kill, 1);
  expect_function_call(kill);
  assert_int_equal (nopoll_connect (&ctx, true), 0);
  assert_ptr_equal(NULL, get_global_conn());

  init_expire_timer (&ctx.connect_timer);
  will_return (nopoll_conn_tls_new, NULL);
  expect_function_call (nopoll_conn_tls_new);
  will_return (checkHostIp, 0);
  expect_function_call (checkHostIp);
  assert_int_equal (nopoll_connect (&ctx, false), 0);
  assert_ptr_equal(NULL, get_global_conn());
}

// Return codes for wait_connection_ready
#define WAIT_SUCCESS	0
#define WAIT_ACTION_RETRY	1	// if wait_status is 307, 302, 303 or 403
#define WAIT_FAIL 	2

void test_wait_connection_ready ()
{
  create_connection_ctx_t ctx;
  ParodusCfg Cfg;
  const char *expected_extra_headers =
      "\r\nAuthorization: Bearer Auth---"
      "\r\nX-WebPA-Device-Name: mac:123567892366"
      "\r\nX-WebPA-Device-Protocols: wrp-0.11,getset-0.1"
      "\r\nUser-Agent: WebPA-1.6 (2.364s2; TG1682/ARRISGroup,Inc.;)"
      "\r\nX-WebPA-Convey: WebPA-1.6 (TG1682)";

  memset(&ctx,0,sizeof(ctx));
  set_server_list_null (&ctx.server_list);

  mock_wait_status = 0;
  mock_redirect = NULL;
  
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  assert_int_equal (wait_connection_ready (&ctx), WAIT_SUCCESS);
  
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  assert_int_equal (wait_connection_ready (&ctx), WAIT_FAIL);
  
  mock_wait_status = 503;
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  assert_int_equal (wait_connection_ready (&ctx), WAIT_FAIL);

  mock_wait_status = 307;
  mock_redirect = "mydns.mycom.net";
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  assert_int_equal (wait_connection_ready (&ctx), WAIT_FAIL);
  
  mock_wait_status = 302;
  mock_redirect = "https://mydns.mycom.net:8080";
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  assert_int_equal (wait_connection_ready (&ctx), WAIT_ACTION_RETRY);
  assert_string_equal (ctx.server_list.redirect.server_addr, "mydns.mycom.net");
  assert_int_equal (ctx.server_list.redirect.port, 8080);
  assert_int_equal (0, ctx.server_list.redirect.allow_insecure);
  assert_ptr_equal (ctx.current_server, &ctx.server_list.redirect);
  free_server (&ctx.server_list.redirect);
  
  mock_wait_status = 303;
  mock_redirect = "http://mydns.mycom.net";
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  assert_int_equal (wait_connection_ready (&ctx), WAIT_ACTION_RETRY);
  assert_string_equal (ctx.server_list.redirect.server_addr, "mydns.mycom.net");
  assert_int_equal (ctx.server_list.redirect.port, 80);
  assert_int_equal (1, ctx.server_list.redirect.allow_insecure);
  assert_ptr_equal (ctx.current_server, &ctx.server_list.redirect);
  free_server (&ctx.server_list.redirect);
  
    mock_wait_status = 403;
    memset(&Cfg, 0, sizeof(ParodusCfg));

    parStrncpy (Cfg.webpa_auth_token, "Auth---", sizeof (Cfg.webpa_auth_token));
    parStrncpy(Cfg.hw_model, "TG1682", sizeof(Cfg.hw_model));
    parStrncpy(Cfg.hw_manufacturer , "ARRISGroup,Inc.", sizeof(Cfg.hw_manufacturer));
    parStrncpy(Cfg.hw_mac , "123567892366", sizeof(Cfg.hw_mac));
    parStrncpy(Cfg.fw_name , "2.364s2", sizeof(Cfg.fw_name));
    parStrncpy(Cfg.webpa_protocol , "WebPA-1.6", sizeof(Cfg.webpa_protocol));
    set_parodus_cfg(&Cfg);
  
    init_header_info (&ctx.header_info);
    will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
    expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
    assert_int_equal (wait_connection_ready (&ctx), WAIT_ACTION_RETRY);
    
    assert_string_equal (ctx.extra_headers, expected_extra_headers);
    free_extra_headers (&ctx);
    free_header_info (&ctx.header_info);
}

// Return codes for connect_and_wait
#define CONN_WAIT_SUCCESS	0
#define CONN_WAIT_ACTION_RETRY	1	// if wait_status is 307, 302, 303 or 403
#define CONN_WAIT_RETRY_DNS 	2

void test_connect_and_wait ()
{
  create_connection_ctx_t ctx;
  noPollCtx test_nopoll_ctx;
  server_t test_server;
  ParodusCfg Cfg;
  char *test_extra_headers =
      "\r\nAuthorization: Bearer SER_MAC Fer23u948590 123567892366"
      "\r\nX-WebPA-Device-Name: mac:123567892366"
      "\r\nX-WebPA-Device-Protocols: wrp-0.11,getset-0.1"
      "\r\nUser-Agent: WebPA-1.6 (2.364s2; TG1682/ARRISGroup,Inc.;)"
      "\r\nX-WebPA-Convey: WebPA-1.6 (TG1682)";

  memset(&Cfg, 0, sizeof(ParodusCfg));
  parStrncpy (Cfg.webpa_url, "http://mydns.mycom.net:8080", sizeof(Cfg.webpa_url));

  mock_wait_status = 0;
  
  memset(&ctx,0,sizeof(ctx));
  ctx.nopoll_ctx = &test_nopoll_ctx;
  ctx.current_server = &test_server;
  ctx.extra_headers = test_extra_headers;

  Cfg.flags = FLAGS_IPV4_ONLY;
  set_parodus_cfg(&Cfg);

  test_server.allow_insecure = 1;
  test_server.server_addr = "mydns.mycom.net";
  test_server.port = 8080;
  will_return (nopoll_conn_new_opts, NULL);
  expect_function_call (nopoll_conn_new_opts);
  will_return (checkHostIp, 0);
  expect_function_call (checkHostIp);
  assert_int_equal (connect_and_wait (&ctx), CONN_WAIT_RETRY_DNS);

  mock_wait_status = 503;  
  will_return (nopoll_conn_new_opts, &connection1);
  expect_function_call (nopoll_conn_new_opts);
  will_return (nopoll_conn_is_ok, nopoll_true);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  will_return (nopoll_conn_ref_count, 0);
  expect_function_call (nopoll_conn_ref_count);
  assert_int_equal (connect_and_wait (&ctx), CONN_WAIT_RETRY_DNS);

  Cfg.flags = 0;
  set_parodus_cfg(&Cfg);

  will_return (nopoll_conn_new_opts, &connection1);
  expect_function_call (nopoll_conn_new_opts);
  will_return (nopoll_conn_is_ok, nopoll_false);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_ref_count, 0);
  expect_function_call (nopoll_conn_ref_count);
  assert_int_equal (connect_and_wait (&ctx), CONN_WAIT_RETRY_DNS);

  will_return (nopoll_conn_new_opts, &connection1);
  expect_function_call (nopoll_conn_new_opts);
  will_return (nopoll_conn_is_ok, nopoll_true);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  assert_int_equal (connect_and_wait (&ctx), CONN_WAIT_SUCCESS);

  Cfg.flags = FLAGS_IPV6_ONLY;
  set_parodus_cfg(&Cfg);

  test_server.allow_insecure = 0;
  will_return (nopoll_conn_tls_new6, NULL);
  expect_function_call (nopoll_conn_tls_new6);
  will_return (checkHostIp, 0);
  expect_function_call (checkHostIp);
  assert_int_equal (connect_and_wait (&ctx), CONN_WAIT_RETRY_DNS);
  
  Cfg.flags = 0;
  set_parodus_cfg(&Cfg);

  will_return (nopoll_conn_tls_new6, NULL);
  expect_function_call (nopoll_conn_tls_new6);
  will_return (checkHostIp, 0);
  expect_function_call (checkHostIp);
  will_return (nopoll_conn_tls_new, NULL);
  expect_function_call (nopoll_conn_tls_new);
  will_return (checkHostIp, 0);
  expect_function_call (checkHostIp);
  assert_int_equal (connect_and_wait (&ctx), CONN_WAIT_RETRY_DNS);

  will_return (nopoll_conn_tls_new6, &connection1);
  expect_function_call (nopoll_conn_tls_new6);
  will_return (nopoll_conn_is_ok, nopoll_false);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_ref_count, 0);
  expect_function_call (nopoll_conn_ref_count);
  will_return (nopoll_conn_tls_new, NULL);
  expect_function_call (nopoll_conn_tls_new);
  will_return (checkHostIp, 0);
  expect_function_call (checkHostIp);
  assert_int_equal (connect_and_wait (&ctx), CONN_WAIT_RETRY_DNS);

  will_return (nopoll_conn_tls_new6, &connection1);
  expect_function_call (nopoll_conn_tls_new6);
  will_return (nopoll_conn_is_ok, nopoll_false);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_ref_count, 0);
  expect_function_call (nopoll_conn_ref_count);
  will_return (nopoll_conn_tls_new, &connection1);
  expect_function_call (nopoll_conn_tls_new);
  will_return (nopoll_conn_is_ok, nopoll_true);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  assert_int_equal (connect_and_wait (&ctx), CONN_WAIT_SUCCESS);
  
  Cfg.flags = FLAGS_IPV4_ONLY;
  set_parodus_cfg(&Cfg);

  will_return (nopoll_conn_tls_new, &connection1);
  expect_function_call (nopoll_conn_tls_new);
  will_return (nopoll_conn_is_ok, nopoll_true);
  expect_function_call (nopoll_conn_is_ok);
  mock_wait_status = 307;
  mock_redirect = "mydns.mycom.net";
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  will_return (nopoll_conn_ref_count, 0);
  expect_function_call (nopoll_conn_ref_count);
  assert_int_equal (connect_and_wait (&ctx), CONN_WAIT_RETRY_DNS);

  will_return (nopoll_conn_tls_new, &connection1);
  expect_function_call (nopoll_conn_tls_new);
  will_return (nopoll_conn_is_ok, nopoll_true);
  expect_function_call (nopoll_conn_is_ok);
  mock_wait_status = 302;
  mock_redirect = "https://mydns.mycom.net";
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  will_return (nopoll_conn_ref_count, 0);
  expect_function_call (nopoll_conn_ref_count);
  assert_int_equal (connect_and_wait (&ctx), CONN_WAIT_ACTION_RETRY);
}

void test_keep_trying ()
{
  int rtn;	
  create_connection_ctx_t ctx;
  noPollCtx test_nopoll_ctx;
  server_t test_server;
  ParodusCfg Cfg;
  char *test_extra_headers =
      "\r\nAuthorization: Bearer SER_MAC Fer23u948590 123567892366"
      "\r\nX-WebPA-Device-Name: mac:123567892366"
      "\r\nX-WebPA-Device-Protocols: wrp-0.11,getset-0.1"
      "\r\nUser-Agent: WebPA-1.6 (2.364s2; TG1682/ARRISGroup,Inc.;)"
      "\r\nX-WebPA-Convey: WebPA-1.6 (TG1682)";

  memset(&Cfg, 0, sizeof(ParodusCfg));
  parStrncpy (Cfg.webpa_url, "http://mydns.mycom.net:8080", sizeof(Cfg.webpa_url));

  mock_wait_status = 0;

  memset(&ctx,0,sizeof(ctx));
  ctx.nopoll_ctx = &test_nopoll_ctx;
  ctx.current_server = &test_server;
  ctx.extra_headers = test_extra_headers;

  test_server.allow_insecure = 1;
  test_server.server_addr = "mydns.mycom.net";
  test_server.port = 8080;

  Cfg.flags = 0;
  set_parodus_cfg(&Cfg);

  will_return (nopoll_conn_new_opts, &connection1);
  expect_function_call (nopoll_conn_new_opts);
  will_return (nopoll_conn_is_ok, nopoll_true);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  rtn = keep_trying_to_connect (&ctx, 30);
  assert_int_equal (rtn, true);

  test_server.allow_insecure = 0;
  Cfg.flags = FLAGS_IPV4_ONLY;
  set_parodus_cfg(&Cfg);

  will_return (nopoll_conn_tls_new, &connection1);
  expect_function_call (nopoll_conn_tls_new);
  will_return (nopoll_conn_is_ok, nopoll_true);
  expect_function_call (nopoll_conn_is_ok);
  mock_wait_status = 302;
  mock_redirect = "https://mydns.mycom.net";
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  will_return (nopoll_conn_ref_count, 0);
  expect_function_call (nopoll_conn_ref_count);
  will_return (nopoll_conn_tls_new, &connection1);
  expect_function_call (nopoll_conn_tls_new);
  will_return (nopoll_conn_is_ok, nopoll_true);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  rtn = keep_trying_to_connect (&ctx, 30);
  assert_int_equal (rtn, true);

  will_return (nopoll_conn_tls_new, &connection1);
  expect_function_call (nopoll_conn_tls_new);
  will_return (nopoll_conn_is_ok, nopoll_true);
  expect_function_call (nopoll_conn_is_ok);
  mock_wait_status = 302503;
  mock_redirect = "https://mydns.mycom.net";
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  will_return (nopoll_conn_ref_count, 0);
  expect_function_call (nopoll_conn_ref_count);
  will_return (nopoll_conn_tls_new, &connection1);
  expect_function_call (nopoll_conn_tls_new);
  will_return (nopoll_conn_is_ok, nopoll_true);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  will_return (nopoll_conn_ref_count, 0);
  expect_function_call (nopoll_conn_ref_count);
  rtn = keep_trying_to_connect (&ctx, 30);
  assert_int_equal (rtn, false);

  mock_wait_status = 0;
  will_return (nopoll_conn_tls_new, NULL);
  expect_function_call (nopoll_conn_tls_new);
  will_return (checkHostIp, 0);
  expect_function_call (checkHostIp);
  rtn = keep_trying_to_connect (&ctx, 30);
  assert_int_equal (rtn, false);

  mock_wait_status = 302;
  mock_redirect = "mydns.mycom.net";

  will_return (nopoll_conn_tls_new, &connection1);
  expect_function_call (nopoll_conn_tls_new);
  will_return (nopoll_conn_is_ok, nopoll_true);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  will_return (nopoll_conn_ref_count, 0);
  expect_function_call (nopoll_conn_ref_count);
  rtn = keep_trying_to_connect (&ctx, 30);
  assert_int_equal (rtn, false);
}

void test_create_nopoll_connection()
{
  int rtn;
  ParodusCfg cfg;
  noPollCtx test_nopoll_ctx;

  memset(&cfg,0,sizeof(cfg));
  cfg.flags = 0;
  parStrncpy (cfg.webpa_url, "mydns.mycom.net:8080", sizeof(cfg.webpa_url));
  cfg.boot_time = 25;
  parStrncpy (cfg.hw_last_reboot_reason, "Test reason", sizeof(cfg.hw_last_reboot_reason));
  cfg.webpa_backoff_max = 30;
  parStrncpy (cfg.webpa_auth_token, "Auth---", sizeof (cfg.webpa_auth_token));
  parStrncpy(cfg.hw_model, "TG1682", sizeof(cfg.hw_model));
  parStrncpy(cfg.hw_manufacturer , "ARRISGroup,Inc.", sizeof(cfg.hw_manufacturer));
  parStrncpy(cfg.hw_mac , "123567892366", sizeof(cfg.hw_mac));
  parStrncpy(cfg.fw_name , "2.364s2", sizeof(cfg.fw_name));
  parStrncpy(cfg.webpa_protocol , "WebPA-1.6", sizeof(cfg.webpa_protocol));
  set_parodus_cfg(&cfg);
  rtn = createNopollConnection (&test_nopoll_ctx);
  assert_int_equal (rtn, nopoll_false);

  parStrncpy (cfg.webpa_url, "http://mydns.mycom.net:8080", sizeof(cfg.webpa_url));
  set_parodus_cfg(&cfg);

  mock_wait_status = 0;
  
  will_return (nopoll_conn_new_opts, &connection1);
  expect_function_call (nopoll_conn_new_opts);
  will_return (nopoll_conn_is_ok, nopoll_true);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  rtn = createNopollConnection (&test_nopoll_ctx);
  assert_int_equal (rtn, nopoll_true);

  parStrncpy (cfg.webpa_url, "https://mydns.mycom.net:8080", sizeof(cfg.webpa_url));
  cfg.flags = 0;
  set_parodus_cfg(&cfg);

  will_return (nopoll_conn_tls_new6, &connection1);
  expect_function_call (nopoll_conn_tls_new6);
  will_return (nopoll_conn_is_ok, nopoll_false);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_ref_count, 0);
  expect_function_call (nopoll_conn_ref_count);
  will_return (nopoll_conn_tls_new, &connection1);
  expect_function_call (nopoll_conn_tls_new);
  will_return (nopoll_conn_is_ok, nopoll_true);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  rtn = createNopollConnection (&test_nopoll_ctx);
  assert_int_equal (rtn, nopoll_true);

  will_return (nopoll_conn_tls_new6, &connection1);
  expect_function_call (nopoll_conn_tls_new6);
  will_return (nopoll_conn_is_ok, nopoll_false);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_ref_count, 0);
  expect_function_call (nopoll_conn_ref_count);
  will_return (nopoll_conn_tls_new, &connection1);
  expect_function_call (nopoll_conn_tls_new);
  will_return (nopoll_conn_is_ok, nopoll_true);
  expect_function_call (nopoll_conn_is_ok);
  mock_wait_status = 302;
  mock_redirect = "https://mydns.mycom.net";
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  will_return (nopoll_conn_ref_count, 0);
  expect_function_call (nopoll_conn_ref_count);

  will_return (nopoll_conn_tls_new6, &connection1);
  expect_function_call (nopoll_conn_tls_new6);
  will_return (nopoll_conn_is_ok, nopoll_false);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_ref_count, 0);
  expect_function_call (nopoll_conn_ref_count);
  will_return (nopoll_conn_tls_new, &connection1);
  expect_function_call (nopoll_conn_tls_new);
  will_return (nopoll_conn_is_ok, nopoll_true);
  expect_function_call (nopoll_conn_is_ok);
  mock_wait_status = 0;
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  rtn = createNopollConnection (&test_nopoll_ctx);
  assert_int_equal (rtn, nopoll_true);

#ifdef FEATURE_DNS_QUERY
  cfg.acquire_jwt = 1;
  cfg.flags = FLAGS_IPV4_ONLY;
  set_parodus_cfg(&cfg);
  
  will_return (allow_insecure_conn, -1);
  expect_function_call (allow_insecure_conn);
  will_return (nopoll_conn_tls_new, NULL);
  expect_function_call (nopoll_conn_tls_new);
  will_return (checkHostIp, 0);
  expect_function_call (checkHostIp);
  mock_server_addr = "mydns.myjwtcom.net";
  mock_port = 80;
  will_return (allow_insecure_conn, 0);
  expect_function_call (allow_insecure_conn);
  will_return (nopoll_conn_tls_new, &connection1);
  expect_function_call (nopoll_conn_tls_new);
  will_return (nopoll_conn_is_ok, nopoll_true);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  rtn = createNopollConnection (&test_nopoll_ctx);
  assert_int_equal (rtn, nopoll_true);
    
  cfg.flags = 0;
  set_parodus_cfg(&cfg);

  will_return (allow_insecure_conn, -1);
  expect_function_call (allow_insecure_conn);
  will_return (nopoll_conn_tls_new6, &connection1);
  expect_function_call (nopoll_conn_tls_new6);
  will_return (nopoll_conn_is_ok, nopoll_false);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_ref_count, 0);
  expect_function_call (nopoll_conn_ref_count);
  mock_wait_status = 0;
  will_return (nopoll_conn_tls_new, &connection1);
  expect_function_call (nopoll_conn_tls_new);
  will_return (nopoll_conn_is_ok, nopoll_true);
  expect_function_call (nopoll_conn_is_ok);
  will_return (nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
  expect_function_call (nopoll_conn_wait_for_status_until_connection_ready);
  rtn = createNopollConnection (&test_nopoll_ctx);
  assert_int_equal (rtn, nopoll_true);
#endif

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
        cmocka_unit_test(test_extra_headers),
        cmocka_unit_test(test_set_current_server),
        cmocka_unit_test(test_set_extra_headers),
        cmocka_unit_test(test_find_servers),
        cmocka_unit_test(test_nopoll_connect),
        cmocka_unit_test(test_wait_connection_ready),
        cmocka_unit_test(test_connect_and_wait),
        cmocka_unit_test(test_keep_trying),
	cmocka_unit_test(test_create_nopoll_connection)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
