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

struct token_data test_data;

int curl_easy_perform(CURL *curl)
{
	UNUSED(curl);
	char *msg = "response";
	int rtn;

	function_called();
	rtn = (int) mock();
	if (0 == rtn)
	  write_callback_fn (msg, 1, strlen(msg), &test_data);
	return rtn;
}
int g_response_code=0;
void setGlobalResponseCode (int response_code)
{
	g_response_code = response_code;
}

int curl_easy_getinfo(CURL *curl, CURLINFO CURLINFO_RESPONSE_CODE, long response_code)
{
	UNUSED(curl);
	UNUSED(CURLINFO_RESPONSE_CODE);
	if (0 != g_response_code)
	{
		response_code= g_response_code;
		ParodusInfo("response_code is %ld\n", response_code);
	}
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
	assert_int_equal (0, (int) cfg.webpa_auth_token[0]);
}

void getAuthToken_MacNull()
{
    ParodusCfg cfg;
    memset(&cfg,0,sizeof(cfg));
    cfg.client_cert_path = NULL;
    getAuthToken(&cfg);
    set_parodus_cfg(&cfg);
    assert( cfg.client_cert_path == NULL);
    assert_int_equal (0, (int) cfg.webpa_auth_token[0]);
}

#if 0
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
#endif

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

	test_data.size = 0;
	test_data.data = token;
	will_return (curl_easy_perform, -1);
	expect_function_calls (curl_easy_perform, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	will_return (curl_easy_getinfo, -1);
	expect_function_calls (curl_easy_getinfo, 1);

	requestNewAuthToken (token, sizeof(token), 2);
	assert_int_equal (output, -1);
	assert_int_equal (0, (int) token[0]);
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

  test_data.size = 0;
  test_data.data = token;
  setGlobalResponseCode(200);
  will_return (curl_easy_perform, 0);
  expect_function_calls (curl_easy_perform, 1);
  will_return (curl_easy_getinfo, 0);
  expect_function_calls (curl_easy_getinfo, 1);

  will_return (curl_easy_getinfo, 0);
  expect_function_calls (curl_easy_getinfo, 1);

  output = requestNewAuthToken (token, sizeof(token), 1);
  assert_int_equal (output, 0);
  assert_string_equal (token, "response");
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
	test_data.size = 0;
	test_data.data = cfg.webpa_auth_token;
	setGlobalResponseCode(200);
	will_return (curl_easy_perform, 0);
	expect_function_calls (curl_easy_perform, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	getAuthToken(&cfg);
	assert_string_equal (cfg.webpa_auth_token, "response");
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
	ParodusCfg *cfg;
	size_t max_data_size = sizeof (cfg->webpa_auth_token);
	char *buffer1 = "response1";
	size_t buf1len = strlen(buffer1);
        char *buffer2 = "R2";
	size_t buf2len = strlen(buffer2);
	char buffer3[max_data_size];
	int out_len=0;
	struct token_data data;
	data.size = 0;

	data.data = (char *) malloc(max_data_size);
	data.data[0] = '\0';

	out_len = write_callback_fn(buffer1, 1, buf1len, &data);
	assert_string_equal(data.data, buffer1);
	assert_int_equal (out_len, buf1len);
	assert_int_equal (data.size, buf1len);

	out_len = write_callback_fn(buffer2, 1, buf2len, &data);
	assert_string_equal(data.data, "response1R2");
	assert_int_equal (out_len, buf2len);
	assert_int_equal (data.size, buf1len+buf2len);

	memset (buffer3, 'x', max_data_size);
	out_len = write_callback_fn(buffer3, 1, max_data_size, &data);
	assert_int_equal (out_len, 0);
	assert_int_equal (data.size, 0);

	free(data.data);
}

void test_requestNewAuthToken_non200 ()
{
  char token[1024];
  ParodusCfg cfg;
  int output;
  memset(&cfg,0,sizeof(cfg));

  cfg.token_server_url = strdup("https://dev.comcast.net/token");
  parStrncpy(cfg.cert_path , "/etc/ssl/certs/ca-certificates.crt", sizeof(cfg.cert_path));
  parStrncpy(cfg.webpa_interface_used , "eth0", sizeof(cfg.webpa_interface_used));
  parStrncpy(cfg.hw_serial_number, "Fer23u948590", sizeof(cfg.hw_serial_number));
  parStrncpy(cfg.hw_mac , "123567892366", sizeof(cfg.hw_mac));
  set_parodus_cfg(&cfg);

  test_data.size = 0;
  test_data.data = token;
  setGlobalResponseCode(404);
  will_return (curl_easy_perform, 0);
  expect_function_calls (curl_easy_perform, 1);
  will_return (curl_easy_getinfo, 0);
  expect_function_calls (curl_easy_getinfo, 1);

  will_return (curl_easy_getinfo, 0);
  expect_function_calls (curl_easy_getinfo, 1);

  output = requestNewAuthToken (token, sizeof(token), 1);
  assert_int_equal (output, -1);
  assert_string_equal (token, "");
  ParodusInfo("requestNewAuthToken output: %d token empty len: %lu\n", output, strlen(token));
  free(cfg.token_server_url);
}

void test_getAuthTokenFailure_non200 ()
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

	will_return (curl_easy_perform, 0);
	expect_function_calls (curl_easy_perform, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	will_return (curl_easy_perform, 0);
	expect_function_calls (curl_easy_perform, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	will_return (curl_easy_perform, 0);
	expect_function_calls (curl_easy_perform, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);

	will_return (curl_easy_getinfo, 0);
	expect_function_calls (curl_easy_getinfo, 1);
	setGlobalResponseCode(504);
	getAuthToken(&cfg);

	assert_string_equal( cfg.webpa_auth_token, "");

	free(cfg.client_cert_path);
	free(cfg.token_server_url);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(test_write_callback_fn),
        cmocka_unit_test(test_requestNewAuthToken),
	// cmocka_unit_test(test_requestNewAuthToken_init_fail),
	cmocka_unit_test(test_requestNewAuthToken_failure),
	cmocka_unit_test(getAuthToken_Null),
	cmocka_unit_test(getAuthToken_MacNull),
	cmocka_unit_test(test_getAuthToken),
	cmocka_unit_test(test_getAuthTokenFailure),
	cmocka_unit_test(test_requestNewAuthToken_non200),
	cmocka_unit_test(test_getAuthTokenFailure_non200)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
