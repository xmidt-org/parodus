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

CURL *curl_easy_init  ()
{
	function_called();
	return (CURL *) mock();
}

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

	will_return (curl_easy_init, NULL);
	expect_function_calls (curl_easy_init, 1);

	requestNewAuthToken (token, sizeof(token), 2);
	assert_int_equal (output, -1);
	assert_int_equal (0, (int) token[0]);
	free(cfg.token_server_url);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
	cmocka_unit_test(test_requestNewAuthToken_init_fail)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
