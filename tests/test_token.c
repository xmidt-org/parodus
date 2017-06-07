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
#include <resolv.h>
#include <cmocka.h>
#include <assert.h>
#include <cjwt/cjwt.h>

#include "../src/ParodusInternal.h"
#include "../src/connection.h"
#include "../src/config.h"

const char *header  = "{	\"alg\":	\"RS256\",	\"typ\":	\"JWT\"}";

time_t exp_time_good = 2147483647;    // 1/18/2038
time_t exp_time_bad  = 1463955372;		// 5/22/2016


const char *payload_good  = "{" \
	"\"iss\":	\"SHA256:jdcRysFunWUAT852huQoIM9GN6k2s5c7iTMTMgujPAk\"," \
	"\"endpoint\":	\"https://fabric.webpa.comcast.net:8080/\"}";

const char *payload_insec  = "{" \
	"\"iss\":	\"SHA256:jdcRysFunWUAT852huQoIM9GN6k2s5c7iTMTMgujPAk\"," \
	"\"endpoint\":	\"http://fabric.webpa.comcast.net:8080/\"}";

// missing endpoint
const char *payload_no_end  = "{" \
	"\"iss\":	\"SHA256:jdcRysFunWUAT852huQoIM9GN6k2s5c7iTMTMgujPAk\"}";

const char *txt_record_id = "aabbccddeeff.test.webpa.comcast.net";

cjwt_t jwt1;	// secure, payload good
cjwt_t jwt2;	// secure, payload good, but expired
cjwt_t jwt3;	// insecure
cjwt_t jwt4;	// missing endpoint

// internal functions in token.c to be tested
extern int analyze_jwt (const cjwt_t *jwt);
extern bool validate_algo(const cjwt_t *jwt);
extern int nquery(const char* dns_txt_record_id,u_char *nsbuf);
extern bool valid_b64_char (char c);
extern const char *strip_rr_data (const char *rr_ptr, int *rrlen);
extern int find_seq_num (const char *rr_ptr, int rrlen);
extern int get_rr_seq_num (const char *rr_ptr, int rrlen);


int setup_test_jwts (void)
{
	memset (&jwt1, 0, sizeof(cjwt_t));
	jwt1.exp.tv_sec = exp_time_good;
	jwt1.private_claims = cJSON_Parse ((char *) payload_good);
	if (NULL == jwt1.private_claims) {
		printf ("Invalid json struct payload_good\n");
		return -1;
	}

	memset (&jwt2, 0, sizeof(cjwt_t));
	jwt2.exp.tv_sec = exp_time_bad;
	jwt2.private_claims = cJSON_Parse ((char *) payload_good);
	if (NULL == jwt2.private_claims) {
		printf ("Invalid json struct payload_good\n");
		return -1;
	}

	memset (&jwt3, 0, sizeof(cjwt_t));
	jwt3.exp.tv_sec = exp_time_good;
	jwt3.private_claims = cJSON_Parse ((char *) payload_insec);
	if (NULL == jwt3.private_claims) {
		printf ("Invalid json struct payload_insec\n");
		return -1;
	}

	memset (&jwt4, 0, sizeof(cjwt_t));
	jwt4.exp.tv_sec = exp_time_good;
	jwt4.private_claims = cJSON_Parse ((char *) payload_no_end);
	if (NULL == jwt4.private_claims) {
		printf ("Invalid json struct payload_good\n");
		return -1;
	}

	return 0;
}



/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

int ns_initparse(const u_char *nsbuf, int l, ns_msg *msg_handle)
{
	UNUSED(nsbuf); UNUSED(l); UNUSED(msg_handle);
	function_called ();
	return (int) mock();
}

int ns_parserr(ns_msg *msg_handle, ns_sect sect, int rec, ns_rr *rr)
{
	UNUSED(msg_handle); UNUSED(sect); UNUSED(rec); UNUSED(rr);
	function_called ();
	return (int) mock();
}

int		__res_ninit (res_state statp)
{
	UNUSED(statp);
	function_called ();
	return (int) mock();
}

int		__res_nquery (res_state statp, const char * txt_record, 
			int class, int type_, u_char * buf, int bufsize)
{
	UNUSED(statp); UNUSED(txt_record); UNUSED(class); UNUSED(type_);
	UNUSED(buf); UNUSED(bufsize);

	function_called ();
	return (int) mock();
}

void		__res_nclose (res_state statp)
{
	UNUSED (statp);
	function_called ();
}

void test_analyze_jwt ()
{
	int ret = setup_test_jwts ();
	assert_int_equal (ret, 0);
	ret = analyze_jwt (&jwt1);
	assert_int_equal (ret, 0);
	ret = analyze_jwt (&jwt2);
	assert_int_equal (ret, -1);
	ret = analyze_jwt (&jwt3);
	assert_int_equal (ret, 1);
	ret = analyze_jwt (&jwt4);
	assert_int_equal (ret, -1);

}

void test_validate_algo ()
{
	bool ret;
	ParodusCfg cfg;
	cfg.jwt_algo = (1<<alg_none) | (1<<alg_rs256);
	set_parodus_cfg (&cfg);
	jwt1.header.alg = alg_rs256;
	ret = validate_algo (&jwt1);
	assert_int_equal (ret, 1);
	jwt1.header.alg = alg_rs512;
	ret = validate_algo (&jwt1);
	assert_int_equal (ret, 0);
}

void test_nquery ()
{
	int len;
	u_char nsbuf[8192];

	will_return (__res_ninit, 0);
	expect_function_call (__res_ninit);

	will_return (__res_nquery, 4096);
	expect_function_call (__res_nquery);

	expect_function_call (__res_nclose);

	len = nquery (txt_record_id, nsbuf);
	assert_int_equal (len, 4096);

	will_return (__res_ninit, -1);
	expect_function_call (__res_ninit);

	len = nquery (txt_record_id, nsbuf);
	assert_int_equal (len, -1);

	will_return (__res_ninit, 0);
	expect_function_call (__res_ninit);

	will_return (__res_nquery, -1);
	expect_function_call (__res_nquery);

	len = nquery (txt_record_id, nsbuf);
	assert_int_equal (len, -1);

}

void test_valid_b64_char()
{
	assert_int_equal (valid_b64_char ('A'), 1);
	assert_int_equal (valid_b64_char ('@'), 0);
}

void test_strip_rrdata ()
{
	const char *s1 = "\"01:this-is-the-test-string-\"\n";
	const char *s2 = "\"01:this-is-the-test-string-\n";
	const char *ss1 =  "01:this-is-the-test-string-";
	int s1len = strlen (ss1);
	const char *result;
  int rlen;

	result = strip_rr_data (s1, &rlen);
	assert_int_equal (rlen, s1len);
	if (rlen == s1len) {
		assert_int_equal (strncmp (result, ss1, rlen), 0);
	}

	result = strip_rr_data (s2, &rlen);
	assert_int_equal (rlen, s1len);
	if (rlen == s1len) {
		assert_int_equal (strncmp (result, ss1, rlen), 0);
	}
}

void test_find_seq_num ()
{
	int pos;
	const char *s1 = "01:this-is-it";
	const char *s2 = "1:this-is-it";
	const char *s3 = ":this-is-it";
	const char *s4 = ".01:this-is-it";
	const char *s5 = "..01:this-is-it";
	const char *s6 = "99999";
	const char *s7 = "xxxxx";

	pos = find_seq_num (s1, strlen(s1));
	assert_int_equal (pos, 0);

	pos = find_seq_num (s2, strlen(s2));
	assert_int_equal (pos, -1);

	pos = find_seq_num (s3, strlen(s3));
	assert_int_equal (pos, -1);

	pos = find_seq_num (s4, strlen(s4));
	assert_int_equal (pos, 1);

	pos = find_seq_num (s5, strlen(s5));
	assert_int_equal (pos, 2);

	pos = find_seq_num (s6, strlen(s6));
	assert_int_equal (pos, -1);

	pos = find_seq_num (s7, strlen(s7));
	assert_int_equal (pos, -1);
}

void test_get_rr_seq_num ()
{
	int result;
	const char *s1 = "01:this-is-it";
	const char *s2 = "1:this-is-it";
	const char *s3 = ":this-is-it";
	const char *s4 = "11:this-is-it";

	result = get_rr_seq_num (s1, strlen(s1));
	assert_int_equal (result, 1);

	result = get_rr_seq_num (s2, strlen(s2));
	assert_int_equal (result, -1);

	result = get_rr_seq_num (s3, strlen(s3));
	assert_int_equal (result, -1);

	result = get_rr_seq_num (s4, strlen(s4));
	assert_int_equal (result, 11);
}


/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_analyze_jwt),
        cmocka_unit_test(test_validate_algo),
        cmocka_unit_test(test_nquery),
				cmocka_unit_test(test_valid_b64_char),
				cmocka_unit_test(test_strip_rrdata),
				cmocka_unit_test(test_find_seq_num),
				cmocka_unit_test(test_get_rr_seq_num),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

