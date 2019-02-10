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

#include <CUnit/Basic.h>

#include "../src/config.h"
#include "../src/auth_token.h"
#include "../src/ParodusInternal.h"

extern int requestNewAuthToken(char *newToken, size_t len, int r_count);

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
typedef void CURL;

typedef enum {
  CURLINFO_RESPONSE_CODE    = 2,
  CURLINFO_TOTAL_TIME
} CURLINFO;

int curl_easy_perform(CURL *curl)
{
	UNUSED(curl);
	function_called();
	return (int) mock();
}

int curl_easy_getinfo(CURL *curl, CURLINFO CURLINFO_RESPONSE_CODE, long response_code)
{
	UNUSED(curl);
	UNUSED(CURLINFO_RESPONSE_CODE);
	UNUSED(response_code);
	function_called();
	return (int) mock();
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void getAuthToken_Null()
{
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	parStrncpy(cfg.hw_mac , "123567892366", sizeof(cfg.hw_mac));
	cfg.client_cert_path = NULL;
	getAuthToken(&cfg);
	set_parodus_cfg(&cfg);
	assert( cfg.client_cert_path == NULL);
}

void getAuthToken_MacNull()
{
    ParodusCfg cfg;
    memset(&cfg,0,sizeof(cfg));
    cfg.client_cert_path = NULL;
    getAuthToken(&cfg);
    set_parodus_cfg(&cfg);
    assert( cfg.client_cert_path == NULL);
}

void test_requestNewAuthToken_init_fail ()
{
	char token[32];
	ParodusCfg cfg;
	int output = -1;
	memset(&cfg,0,sizeof(cfg));

	cfg.token_server_url = strdup("https://dev.comcast.net/token");
	parStrncpy(cfg.cert_path , "/etc/ssl/certs/ca-certificates.crt", sizeof(cfg.cert_path));
	parStrncpy(cfg.hw_serial_number, "Fer23u948590", sizeof(cfg.hw_serial_number));
	parStrncpy(cfg.hw_mac , "123567892366", sizeof(cfg.hw_mac));

	set_parodus_cfg(&cfg);

	will_return (curl_easy_perform, -1);
	expect_function_calls (curl_easy_perform, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	will_return (curl_easy_getinfo, -1);
	expect_function_calls (curl_easy_getinfo, 1);

	requestNewAuthToken (token, sizeof(token), 2);
	assert_int_equal (output, -1);
	free(cfg.token_server_url);
}


void test_requestNewAuthToken_failure ()
{
	char token[32];
	ParodusCfg cfg;
	int output = -1;
	memset(&cfg,0,sizeof(cfg));

	cfg.token_server_url = strdup("https://dev.comcast.net/token");
	parStrncpy(cfg.cert_path , "/etc/ssl/certs/ca-certificates.crt", sizeof(cfg.cert_path));
	parStrncpy(cfg.hw_serial_number, "Fer23u948590", sizeof(cfg.hw_serial_number));
	parStrncpy(cfg.hw_mac , "123567892366", sizeof(cfg.hw_mac));
	set_parodus_cfg(&cfg);

	will_return (curl_easy_perform, -1);
	expect_function_calls (curl_easy_perform, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	will_return (curl_easy_getinfo, -1);
	expect_function_calls (curl_easy_getinfo, 1);

	requestNewAuthToken (token, sizeof(token), 2);
	assert_int_equal (output, -1);
	free(cfg.token_server_url);
}


void test_requestNewAuthToken ()
{
  char token[1024];
  ParodusCfg cfg;
  int output = -1;
  memset(&cfg,0,sizeof(cfg));

  cfg.token_server_url = strdup("https://dev.comcast.net/token");
  parStrncpy(cfg.cert_path , "/etc/ssl/certs/ca-certificates.crt", sizeof(cfg.cert_path));
  parStrncpy(cfg.webpa_interface_used , "eth0", sizeof(cfg.webpa_interface_used));
  parStrncpy(cfg.hw_serial_number, "Fer23u948590", sizeof(cfg.hw_serial_number));
  parStrncpy(cfg.hw_mac , "123567892366", sizeof(cfg.hw_mac));
  set_parodus_cfg(&cfg);

  will_return (curl_easy_perform, 0);
  expect_function_calls (curl_easy_perform, 1);
  will_return (curl_easy_getinfo, 0);
  expect_function_calls (curl_easy_getinfo, 1);

  will_return (curl_easy_getinfo, 0);
  expect_function_calls (curl_easy_getinfo, 1);

  output = requestNewAuthToken (token, sizeof(token), 1);
  assert_int_equal (output, 0);
  free(cfg.token_server_url);
}

void test_getAuthToken ()
{
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));

	cfg.token_server_url = strdup("https://dev.comcast.net/token");
	cfg.client_cert_path = strdup("testcert");
	parStrncpy(cfg.cert_path , "/etc/ssl/certs/ca-certificates.crt", sizeof(cfg.cert_path));
	parStrncpy(cfg.webpa_interface_used , "eth0", sizeof(cfg.webpa_interface_used));
	parStrncpy(cfg.hw_serial_number, "Fer23u948590", sizeof(cfg.hw_serial_number));
	parStrncpy(cfg.hw_mac , "123567892366", sizeof(cfg.hw_mac));
	set_parodus_cfg(&cfg);

	/* To test curl failure case and retry on v4 mode */
	will_return (curl_easy_perform, -1);
	expect_function_calls (curl_easy_perform, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	/* To test curl failure case and retry on v6 mode */
	will_return (curl_easy_perform, -1);
	expect_function_calls (curl_easy_perform, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	/* To test curl success case */
	will_return (curl_easy_perform, 0);
	expect_function_calls (curl_easy_perform, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	getAuthToken(&cfg);

	free(cfg.client_cert_path);
	free(cfg.token_server_url);
}

void test_getAuthTokenFailure ()
{
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));

	cfg.token_server_url = strdup("https://dev.comcast.net/token");
	cfg.client_cert_path = strdup("testcert");
	parStrncpy(cfg.cert_path , "/etc/ssl/certs/ca-certificates.crt", sizeof(cfg.cert_path));
	parStrncpy(cfg.webpa_interface_used , "eth0", sizeof(cfg.webpa_interface_used));
	parStrncpy(cfg.hw_serial_number, "Fer23u948590", sizeof(cfg.hw_serial_number));
	parStrncpy(cfg.hw_mac , "123567892366", sizeof(cfg.hw_mac));
	set_parodus_cfg(&cfg);
	will_return (curl_easy_perform, -1);
	expect_function_calls (curl_easy_perform, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	will_return (curl_easy_perform, -1);
	expect_function_calls (curl_easy_perform, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	will_return (curl_easy_perform, -1);
	expect_function_calls (curl_easy_perform, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	getAuthToken(&cfg);

	assert_string_equal( cfg.webpa_auth_token, "");

	free(cfg.client_cert_path);
	free(cfg.token_server_url);
}

void test_write_callback_fn ()
{
	void *buffer;
	size_t size = 1;
	size_t nmemb =8;
	int out_len=0;
	struct token_data data;
	data.size = 0;
	buffer = strdup("response");

	data.data = (char *) malloc(sizeof(char) * 5);
	data.data[0] = '\0';

	out_len = write_callback_fn(buffer, size, nmemb, &data);
	assert_string_equal(data.data, buffer);
	assert_int_equal( out_len, strlen(buffer));
	free(data.data);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_requestNewAuthToken),
	cmocka_unit_test(test_requestNewAuthToken_init_fail),
	cmocka_unit_test(test_requestNewAuthToken_failure),
	cmocka_unit_test(getAuthToken_Null),
	cmocka_unit_test(getAuthToken_MacNull),
	cmocka_unit_test(test_getAuthToken),
	cmocka_unit_test(test_getAuthTokenFailure),
	cmocka_unit_test(test_write_callback_fn),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
