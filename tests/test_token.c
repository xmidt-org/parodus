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
#include <wrp-c.h>

#include "../src/token.h"
#include "../src/ParodusInternal.h"
#include "../src/connection.h"
#include "../src/config.h"

const char *header  = "{	\"alg\":	\"RS256\",	\"typ\":	\"JWT\"}";

time_t exp_time_good = 2147483647;    // 1/18/2038
time_t exp_time_bad  = 1463955372;		// 5/22/2016


const char *payload_good  = "{" \
	"\"iss\":	\"SHA256:jdcRysFunWUAT852huQoIM9GN6k2s5c7iTMTMgujPAk\"," \
	"\"endpoint\":	\"https://mydns.mycom.net:8080/\"}";

const char *payload_insec  = "{" \
	"\"iss\":	\"SHA256:jdcRysFunWUAT852huQoIM9GN6k2s5c7iTMTMgujPAk\"," \
	"\"endpoint\":	\"http://mydns.mycom.net:8080/\"}";

// missing endpoint
const char *payload_no_end  = "{" \
	"\"iss\":	\"SHA256:jdcRysFunWUAT852huQoIM9GN6k2s5c7iTMTMgujPAk\"}";

const char *txt_record_id = "aabbccddeeff.test.webpa.comcast.net";

#define MAX_RR_RECS 10

/*
expiration = 2147483647 #1/18/2038
endpoint = "https://mydns.mycom.net:8080/"

dns_rec_claims = {
	'iss': u'SHA256:jdcRysFunWUAT852huQoIM9GN6k2s5c7iTMTMgujPAk', 
	'endpoint': endpoint, 
	'exp': expiration
	}
*/
const char *dns_recs_test =
	"\"03:iJmBr4pGROCCbRlbL3GpGDcSFJcaqBQFOriTRk0xKQ\"\n"
	"\"01:eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJTSEEyNTY6amRjUnlzRnVuV1VBVDg1Mmh1UW9JTTlHTjZrMnM1YzdpVE1UTWd1alBBayIsImVuZHBvaW50IjoiaHR0cHM6Ly9teWRucy5teWNvbS5uZXQ6ODA4MC8iLCJleHAiOjIxNDc0ODM2NDd9.VjG2PzPws2J707PiEaUGOTXD9NWnrGyfAUvvfyXFw-Px3gx5wg1\"\n"
	"\"02:EXGyJQXrxMYuepAiFPT0sWxvydHvZKxcx7S5MCmlOhRpWZRrzgFceNWMwFcRPAPxovOoc4aYvx2DqGMdckmAD1q3y3Gjjw1qyxd4503jmrXKfjrdW3eFZD_Q454iRVf4c0CMWA5tl0ElOlX9u_HC3G_dPiKGUBU5PpWONgHpg2-ewrQcUe4-J4hBQmqZmRSB9n-6zB1XD80cDQO7aHeJ9aysJS1vgmwaSuT9y6PhPsp05NUC0TDzZVJUX\"\n"
;

const char *dns_recs_extra =
	"\"03:iJmBr4pGROCCbRlbL3GpGDcSFJcaqBQFOriTRk0xKQ\"\n"
	"\"01:eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJTSEEyNTY6amRjUnlzRnVuV1VBVDg1Mmh1UW9JTTlHTjZrMnM1YzdpVE1UTWd1alBBayIsImVuZHBvaW50IjoiaHR0cHM6Ly9teWRucy5teWNvbS5uZXQ6ODA4MC8iLCJleHAiOjIxNDc0ODM2NDd9.VjG2PzPws2J707PiEaUGOTXD9NWnrGyfAUvvfyXFw-Px3gx5wg1\"\n"
	"\n" // non-txt record type
	"\"02:EXGyJQXrxMYuepAiFPT0sWxvydHvZKxcx7S5MCmlOhRpWZRrzgFceNWMwFcRPAPxovOoc4aYvx2DqGMdckmAD1q3y3Gjjw1qyxd4503jmrXKfjrdW3eFZD_Q454iRVf4c0CMWA5tl0ElOlX9u_HC3G_dPiKGUBU5PpWONgHpg2-ewrQcUe4-J4hBQmqZmRSB9n-6zB1XD80cDQO7aHeJ9aysJS1vgmwaSuT9y6PhPsp05NUC0TDzZVJUX\"\n"
;

char *rr_recs_test[] = {
	"eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJTSEEyNTY6amRjUnlzRnVuV1VBVDg1Mmh1UW9JTTlHTjZrMnM1YzdpVE1UTWd1alBBayIsImVuZHBvaW50IjoiaHR0cHM6Ly9teWRucy5teWNvbS5uZXQ6ODA4MC8iLCJleHAiOjIxNDc0ODM2NDd9.VjG2PzPws2J707PiEaUGOTXD9NWnrGyfAUvvfyXFw-Px3gx5wg1",
	"EXGyJQXrxMYuepAiFPT0sWxvydHvZKxcx7S5MCmlOhRpWZRrzgFceNWMwFcRPAPxovOoc4aYvx2DqGMdckmAD1q3y3Gjjw1qyxd4503jmrXKfjrdW3eFZD_Q454iRVf4c0CMWA5tl0ElOlX9u_HC3G_dPiKGUBU5PpWONgHpg2-ewrQcUe4-J4hBQmqZmRSB9n-6zB1XD80cDQO7aHeJ9aysJS1vgmwaSuT9y6PhPsp05NUC0TDzZVJUX",
	"iJmBr4pGROCCbRlbL3GpGDcSFJcaqBQFOriTRk0xKQ"
};

char *dns_jwt_test = 
	"eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJTSEEyNTY6amRjUnlzRnVuV1VBVDg1Mmh1UW9JTTlHTjZrMnM1YzdpVE1UTWd1alBBayIsImVuZHBvaW50IjoiaHR0cHM6Ly9teWRucy5teWNvbS5uZXQ6ODA4MC8iLCJleHAiOjIxNDc0ODM2NDd9.VjG2PzPws2J707PiEaUGOTXD9NWnrGyfAUvvfyXFw-Px3gx5wg1"
	"EXGyJQXrxMYuepAiFPT0sWxvydHvZKxcx7S5MCmlOhRpWZRrzgFceNWMwFcRPAPxovOoc4aYvx2DqGMdckmAD1q3y3Gjjw1qyxd4503jmrXKfjrdW3eFZD_Q454iRVf4c0CMWA5tl0ElOlX9u_HC3G_dPiKGUBU5PpWONgHpg2-ewrQcUe4-J4hBQmqZmRSB9n-6zB1XD80cDQO7aHeJ9aysJS1vgmwaSuT9y6PhPsp05NUC0TDzZVJUX"
	"iJmBr4pGROCCbRlbL3GpGDcSFJcaqBQFOriTRk0xKQ"
;


/*
expiration = 2147483647 #1/18/2038
endpoint = "https://mydns.mycom.net:8080/"

dns_rec2_claims = {
	'endpoint': endpoint
	}
*/
// no exp in it
const char *dns_recs2_test =
	"\"eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJlbmRwb2ludCI6Imh0dHBzOi8vbXlkbnMubXljb20ubmV0OjgwODAvIn0.YkudDhp7Ifr6wTsCfrYUUeRf1CMKannC6Bx994OrcBVwqNQ1G5uBXEgsslUnbqV_7FTGPp1Vs058qncmIXtEYxVlHt-mn1UEm6hyC_QGi9FfMHYBS7QHwcCIs467XUFMZayQ3upzZNXObhzJNF0-RR72S61cjSGqf1z5KgyBtoANtW6xCdWB5VV6CqCxlmJNj6d4N8vKiUvN346-UgB_H4AuaXCNdXJ2NP2DxRec-oNTCjhNzRn-6MaB-UwW_gD9CYfQ0vrw2Nv8dO1Sk5Ku94cdIfvUEft-kyqPGKmK8soIggjvmvW0JEPYwrWYY9Ry5SQf80-dLOPXCQSQ3Rzy7Q\""
;

// no exp in it
char *rr_recs2_test[] = {
	"eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJlbmRwb2ludCI6Imh0dHBzOi8vbXlkbnMubXljb20ubmV0OjgwODAvIn0.YkudDhp7Ifr6wTsCfrYUUeRf1CMKannC6Bx994OrcBVwqNQ1G5uBXEgsslUnbqV_7FTGPp1Vs058qncmIXtEYxVlHt-mn1UEm6hyC_QGi9FfMHYBS7QHwcCIs467XUFMZayQ3upzZNXObhzJNF0-RR72S61cjSGqf1z5KgyBtoANtW6xCdWB5VV6CqCxlmJNj6d4N8vKiUvN346-UgB_H4AuaXCNdXJ2NP2DxRec-oNTCjhNzRn-6MaB-UwW_gD9CYfQ0vrw2Nv8dO1Sk5Ku94cdIfvUEft-kyqPGKmK8soIggjvmvW0JEPYwrWYY9Ry5SQf80-dLOPXCQSQ3Rzy7Q"
};

const char *dns_recs_err1 = // missing seq
	"\"03:iJmBr4pGROCCbRlbL3GpGDcSFJcaqBQFOriTRk0xKQ\"\n"
	"\"eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJTSEEyNTY6amRjUnlzRnVuV1VBVDg1Mmh1UW9JTTlHTjZrMnM1YzdpVE1UTWd1alBBayIsImVuZHBvaW50IjoiaHR0cHM6Ly9teWRucy5teWNvbS5uZXQ6ODA4MC8iLCJleHAiOjIxNDc0ODM2NDd9.VjG2PzPws2J707PiEaUGOTXD9NWnrGyfAUvvfyXFw-Px3gx5wg1\"\n"
	"\"02:EXGyJQXrxMYuepAiFPT0sWxvydHvZKxcx7S5MCmlOhRpWZRrzgFceNWMwFcRPAPxovOoc4aYvx2DqGMdckmAD1q3y3Gjjw1qyxd4503jmrXKfjrdW3eFZD_Q454iRVf4c0CMWA5tl0ElOlX9u_HC3G_dPiKGUBU5PpWONgHpg2-ewrQcUe4-J4hBQmqZmRSB9n-6zB1XD80cDQO7aHeJ9aysJS1vgmwaSuT9y6PhPsp05NUC0TDzZVJUX\"\n"
;

const char *dns_recs_err2 = // invalid seq
	"\"03:iJmBr4pGROCCbRlbL3GpGDcSFJcaqBQFOriTRk0xKQ\"\n"
	"\"0:eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJTSEEyNTY6amRjUnlzRnVuV1VBVDg1Mmh1UW9JTTlHTjZrMnM1YzdpVE1UTWd1alBBayIsImVuZHBvaW50IjoiaHR0cHM6Ly9teWRucy5teWNvbS5uZXQ6ODA4MC8iLCJleHAiOjIxNDc0ODM2NDd9.VjG2PzPws2J707PiEaUGOTXD9NWnrGyfAUvvfyXFw-Px3gx5wg1\"\n"
	"\"02:EXGyJQXrxMYuepAiFPT0sWxvydHvZKxcx7S5MCmlOhRpWZRrzgFceNWMwFcRPAPxovOoc4aYvx2DqGMdckmAD1q3y3Gjjw1qyxd4503jmrXKfjrdW3eFZD_Q454iRVf4c0CMWA5tl0ElOlX9u_HC3G_dPiKGUBU5PpWONgHpg2-ewrQcUe4-J4hBQmqZmRSB9n-6zB1XD80cDQO7aHeJ9aysJS1vgmwaSuT9y6PhPsp05NUC0TDzZVJUX\"\n"
;

const char *dns_recs_err3 = // invalid seq too high
	"\"03:iJmBr4pGROCCbRlbL3GpGDcSFJcaqBQFOriTRk0xKQ\"\n"
	"\"99:eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJTSEEyNTY6amRjUnlzRnVuV1VBVDg1Mmh1UW9JTTlHTjZrMnM1YzdpVE1UTWd1alBBayIsImVuZHBvaW50IjoiaHR0cHM6Ly9teWRucy5teWNvbS5uZXQ6ODA4MC8iLCJleHAiOjIxNDc0ODM2NDd9.VjG2PzPws2J707PiEaUGOTXD9NWnrGyfAUvvfyXFw-Px3gx5wg1\"\n"
	"\"02:EXGyJQXrxMYuepAiFPT0sWxvydHvZKxcx7S5MCmlOhRpWZRrzgFceNWMwFcRPAPxovOoc4aYvx2DqGMdckmAD1q3y3Gjjw1qyxd4503jmrXKfjrdW3eFZD_Q454iRVf4c0CMWA5tl0ElOlX9u_HC3G_dPiKGUBU5PpWONgHpg2-ewrQcUe4-J4hBQmqZmRSB9n-6zB1XD80cDQO7aHeJ9aysJS1vgmwaSuT9y6PhPsp05NUC0TDzZVJUX\"\n"
;

const char *dns_recs_err4 = // duplicate seq number
	"\"03:iJmBr4pGROCCbRlbL3GpGDcSFJcaqBQFOriTRk0xKQ\"\n"
	"\"03:iJmBr4pGROCCbRlbL3GpGDcSFJcaqBQFOriTRk0xKQ\"\n"
	"\"01:eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJTSEEyNTY6amRjUnlzRnVuV1VBVDg1Mmh1UW9JTTlHTjZrMnM1YzdpVE1UTWd1alBBayIsImVuZHBvaW50IjoiaHR0cHM6Ly9teWRucy5teWNvbS5uZXQ6ODA4MC8iLCJleHAiOjIxNDc0ODM2NDd9.VjG2PzPws2J707PiEaUGOTXD9NWnrGyfAUvvfyXFw-Px3gx5wg1\"\n"
	"\"02:EXGyJQXrxMYuepAiFPT0sWxvydHvZKxcx7S5MCmlOhRpWZRrzgFceNWMwFcRPAPxovOoc4aYvx2DqGMdckmAD1q3y3Gjjw1qyxd4503jmrXKfjrdW3eFZD_Q454iRVf4c0CMWA5tl0ElOlX9u_HC3G_dPiKGUBU5PpWONgHpg2-ewrQcUe4-J4hBQmqZmRSB9n-6zB1XD80cDQO7aHeJ9aysJS1vgmwaSuT9y6PhPsp05NUC0TDzZVJUX\"\n"
;

const char *dns_recs_err5 = // missing rec 1
	"\"03:iJmBr4pGROCCbRlbL3GpGDcSFJcaqBQFOriTRk0xKQ\"\n"
	"\n" // non-txt record type
	"\"02:EXGyJQXrxMYuepAiFPT0sWxvydHvZKxcx7S5MCmlOhRpWZRrzgFceNWMwFcRPAPxovOoc4aYvx2DqGMdckmAD1q3y3Gjjw1qyxd4503jmrXKfjrdW3eFZD_Q454iRVf4c0CMWA5tl0ElOlX9u_HC3G_dPiKGUBU5PpWONgHpg2-ewrQcUe4-J4hBQmqZmRSB9n-6zB1XD80cDQO7aHeJ9aysJS1vgmwaSuT9y6PhPsp05NUC0TDzZVJUX\"\n"
;

cjwt_t jwt1;	// secure, payload good
cjwt_t jwt2;	// secure, payload good, but expired
cjwt_t jwt3;	// insecure
cjwt_t jwt4;	// missing endpoint

// internal functions in token.c to be tested
extern int analyze_jwt (const cjwt_t *jwt, char **url_buf, unsigned int *port);
extern bool validate_algo(const cjwt_t *jwt);
extern int nquery(const char* dns_txt_record_id,u_char *nsbuf);
extern bool valid_b64_char (char c);
extern const char *strip_rr_data (const char *rr_ptr, int *rrlen);
extern int find_seq_num (const char *rr_ptr, int rrlen);
extern int get_rr_seq_num (const char *rr_ptr, int rrlen);
extern int get_rr_seq_table (ns_msg *msg_handle, int num_rr_recs, rr_rec_t *seq_table);
extern int assemble_jwt_from_dns (ns_msg *msg_handle, int num_rr_recs, char *jwt_ans);
extern int query_dns(const char* dns_txt_record_id,char *jwt_ans);
extern void read_key_from_file (const char *fname, char *buf, size_t buflen);
extern const char *get_tok (const char *src, int delim, char *result, int resultsize);
extern unsigned int get_algo_mask (const char *algo_str);

pthread_mutex_t crud_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t crud_con=PTHREAD_COND_INITIALIZER;
int numLoops;

pthread_cond_t *get_global_crud_con(void)
{
    return &crud_con;
}

pthread_mutex_t *get_global_crud_mut(void)
{
    return &crud_mut;
}

void addCRUDmsgToQueue(wrp_msg_t *crudMsg)
{
	(void)crudMsg;
	return;
}

void *CRUDHandlerTask()
{
	return NULL;
}

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

static int get_dns_text (const char *dns_rec_id, u_char *nsbuf, int bufsize)
{
	const char *rec = NULL;

	if (strstr (dns_rec_id, ".test.") != NULL)
		rec = dns_recs_test;
	else if (strstr (dns_rec_id, ".extra.") != NULL)
		rec = dns_recs_extra;
	else if (strstr (dns_rec_id, ".test2.") != NULL)
		rec = dns_recs2_test;
	else if (strstr (dns_rec_id, ".err1.") != NULL)
		rec = dns_recs_err1;
	else if (strstr (dns_rec_id, ".err2.") != NULL)
		rec = dns_recs_err2;
	else if (strstr (dns_rec_id, ".err3.") != NULL)
		rec = dns_recs_err3;
	else if (strstr (dns_rec_id, ".err4.") != NULL)
		rec = dns_recs_err4;
	else if (strstr (dns_rec_id, ".err5.") != NULL)
		rec = dns_recs_err5;
	else
		return -1;
	strncpy ((char *) nsbuf, rec, bufsize);
	return 0;
}

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

// These mocks assume that the pieces of a dns txt record end with '\n';


int ns_initparse(const u_char *nsbuf, int l, ns_msg *msg_handle)
{
	UNUSED(l);
	char *buf = (char*) nsbuf;
	char *next;
	int i, count = 0;

	msg_handle->_msg_ptr = nsbuf;
	while (true)
	{
		if (buf[0] == 0)
			break;
		count++;
		next = strchr (buf, '\n');
		if (NULL == next)
			break;
		*next = 0;
		buf = ++next;
	}
	for (i=0; i<ns_s_max; i++) {
		if (i == ns_s_an)
			msg_handle->_counts[i] = count;
		else
			msg_handle->_counts[i] = 0;
	}
	return 0;
}

int ns_parserr(ns_msg *msg_handle, ns_sect sect, int rec, ns_rr *rr)
{
	UNUSED(sect);
	int i, l;
	char *ptr = (char *) msg_handle->_msg_ptr;

	if (rec >= msg_handle->_counts[ns_s_an]) {
		errno = EINVAL;
		return -1;
	}

	for (i=0; i < rec; i++) {
		l = strlen (ptr);
		ptr += (l+1);
	}

	l = strlen(ptr);
	if (l == 0) {
		rr->type = ns_t_key;
	} else {
		rr->type = ns_t_txt;
	}
	rr->rdata = (u_char *) ptr;
	rr->rdlength = l; 
	return 0;
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
	UNUSED(statp); UNUSED(class); UNUSED(type_);
	int ret = get_dns_text (txt_record, buf, bufsize);
	if (ret == 0)
		return strlen ( (char*) buf); 
	else
		return -1;
}

void		__res_nclose (res_state statp)
{
	UNUSED (statp);
	function_called ();
}

// Analyzes a jwt structure
void test_analyze_jwt ()
{
    unsigned int port;
    char *server_Address;
	int ret = setup_test_jwts ();
	assert_int_equal (ret, 0);
	ret = analyze_jwt (&jwt1, &server_Address, &port);
	assert_int_equal (ret, 0);
	assert_string_equal (server_Address, "mydns.mycom.net");
	assert_int_equal (port, 8080);
	free (server_Address);
	ret = analyze_jwt (&jwt2, &server_Address, &port);
	assert_int_equal (ret, TOKEN_ERR_JWT_EXPIRED);
	ret = analyze_jwt (&jwt3, &server_Address, &port);
	assert_int_equal (ret, 1);
	assert_string_equal (server_Address, "mydns.mycom.net");
	assert_int_equal (port, 8080);
	free (server_Address);
	ret = analyze_jwt (&jwt4, &server_Address, &port);
	assert_int_equal (ret, TOKEN_ERR_INVALID_JWT_CONTENT);
}

void test_validate_algo ()
{
	bool ret;
	ParodusCfg cfg;
	cfg.jwt_algo = 1025;
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

	expect_function_call (__res_nclose);

	len = nquery (txt_record_id, nsbuf);
	assert_int_equal (len, strlen(dns_recs_test));

	will_return (__res_ninit, -1);
	expect_function_call (__res_ninit);

	len = nquery (txt_record_id, nsbuf);
	assert_int_equal (len, -1);

	will_return (__res_ninit, 0);
	expect_function_call (__res_ninit);

	len = nquery (".nosuch.", nsbuf);
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

	rlen = strlen (s1);
	result = strip_rr_data (s1, &rlen);
	assert_int_equal (rlen, s1len);
	if (rlen == s1len) {
		assert_int_equal (strncmp (result, ss1, rlen), 0);
	}

	rlen = strlen (s2);
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
	assert_int_equal (pos, -2);

	pos = find_seq_num (s3, strlen(s3));
	assert_int_equal (pos, -2);

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

void test_get_rr_seq_table()
{
#define SEQ_TABLE_SIZE (MAX_RR_RECS + 1)
	u_char nsbuf[4096];
	ns_msg msg_handle;
	int i, num_txt_recs, ret;
	rr_rec_t seq_table[SEQ_TABLE_SIZE];

	memset (&msg_handle, 0, sizeof(ns_msg));

	ret = get_dns_text (".test.", nsbuf, 4096);
	assert_int_equal (ret, 0);
	ns_initparse (nsbuf, 4096, &msg_handle);
	assert_int_equal (msg_handle._counts[ns_s_an],  3);
	num_txt_recs = get_rr_seq_table (&msg_handle, 3, seq_table);
	assert_int_equal (num_txt_recs, 3);
	assert_ptr_equal (seq_table[0].rr_ptr, NULL);
	for (i=0; i<3; i++) {
		int len = strlen (rr_recs_test[i]);
		assert_int_equal (len, seq_table[i+1].rr_len);
		ret = strncmp (seq_table[i+1].rr_ptr, rr_recs_test[i], len);
		assert_int_equal (ret, 0);
	}

	ret = get_dns_text (".test2.", nsbuf, 4096);
	assert_int_equal (ret, 0);
	ns_initparse (nsbuf, 4096, &msg_handle);
	assert_int_equal (msg_handle._counts[ns_s_an],  1);
	num_txt_recs = get_rr_seq_table (&msg_handle, 1, seq_table);
	assert_int_equal (num_txt_recs, 1);
	assert_ptr_not_equal (seq_table[0].rr_ptr, NULL);
	if (NULL != seq_table[0].rr_ptr) {
		int len = strlen (rr_recs2_test[0]);
		assert_int_equal (len, seq_table[0].rr_len);
		ret = strncmp (seq_table[0].rr_ptr, rr_recs2_test[0], len);
		assert_int_equal (ret, 0);
	}

	ret = get_dns_text (".extra.", nsbuf, 4096);
	assert_int_equal (ret, 0);
	ns_initparse (nsbuf, 4096, &msg_handle);
	assert_int_equal (msg_handle._counts[ns_s_an],  4);
	num_txt_recs = get_rr_seq_table (&msg_handle, 4, seq_table);
	assert_int_equal (num_txt_recs, 3);
	assert_ptr_equal (seq_table[0].rr_ptr, NULL);
	for (i=0; i<3; i++) {
		int len = strlen (rr_recs_test[i]);
		assert_int_equal (len, seq_table[i+1].rr_len);
		ret = strncmp (seq_table[i+1].rr_ptr, rr_recs_test[i], len);
		assert_int_equal (ret, 0);
	}

	ret = get_dns_text (".err1.", nsbuf, 4096);
	assert_int_equal (ret, 0);
	ns_initparse (nsbuf, 4096, &msg_handle);
	assert_int_equal (msg_handle._counts[ns_s_an],  3);
	num_txt_recs = get_rr_seq_table (&msg_handle, 3, seq_table);
	assert_int_equal (num_txt_recs, -1);

	ret = get_dns_text (".err2.", nsbuf, 4096);
	assert_int_equal (ret, 0);
	ns_initparse (nsbuf, 4096, &msg_handle);
	assert_int_equal (msg_handle._counts[ns_s_an],  3);
	num_txt_recs = get_rr_seq_table (&msg_handle, 3, seq_table);
	assert_int_equal (num_txt_recs, -1);

	ret = get_dns_text (".err3.", nsbuf, 4096);
	assert_int_equal (ret, 0);
	ns_initparse (nsbuf, 4096, &msg_handle);
	assert_int_equal (msg_handle._counts[ns_s_an],  3);
	num_txt_recs = get_rr_seq_table (&msg_handle, 3, seq_table);
	assert_int_equal (num_txt_recs, -1);

	ret = get_dns_text (".err4.", nsbuf, 4096);
	assert_int_equal (ret, 0);
	ns_initparse (nsbuf, 4096, &msg_handle);
	assert_int_equal (msg_handle._counts[ns_s_an],  4);
	num_txt_recs = get_rr_seq_table (&msg_handle, 4, seq_table);
	assert_int_equal (num_txt_recs, -1);

	ret = get_dns_text (".err5.", nsbuf, 4096);
	assert_int_equal (ret, 0);
	ns_initparse (nsbuf, 4096, &msg_handle);
	assert_int_equal (msg_handle._counts[ns_s_an],  3);
	num_txt_recs = get_rr_seq_table (&msg_handle, 3, seq_table);
	assert_int_equal (num_txt_recs, -1);
}

void test_assemble_jwt_from_dns ()
{
	ns_msg msg_handle;
	u_char nsbuf[4096];
	char jwt_token[8192];
	int ret;

	memset (&msg_handle, 0, sizeof(ns_msg));

	ret = get_dns_text (".test.", nsbuf, 4096);
	assert_int_equal (ret, 0);
	ns_initparse (nsbuf, 4096, &msg_handle);
	assert_int_equal (msg_handle._counts[ns_s_an],  3);
	ret = assemble_jwt_from_dns (&msg_handle, 3, jwt_token);
	assert_int_equal (ret, 0);

	ret = strcmp (dns_jwt_test, jwt_token);
	assert_int_equal (ret, 0);

	ret = get_dns_text (".err5.", nsbuf, 4096);
	assert_int_equal (ret, 0);
	ns_initparse (nsbuf, 4096, &msg_handle);
	assert_int_equal (msg_handle._counts[ns_s_an],  3);
	ret = assemble_jwt_from_dns (&msg_handle, 3, jwt_token);
	assert_int_equal (ret, -1);

}

void test_query_dns ()
{
	int ret;
	char jwt_buf[8192];

	will_return (__res_ninit, 0);
	expect_function_call (__res_ninit);
	expect_function_call (__res_nclose);

	ret = query_dns (txt_record_id, jwt_buf);
	assert_int_equal (ret, 0);

	will_return (__res_ninit, 0);
	expect_function_call (__res_ninit);
	//expect_function_call (__res_nclose);

	ret = query_dns (".nosuch.", jwt_buf);
	assert_int_equal (ret, -1);

	will_return (__res_ninit, 0);
	expect_function_call (__res_ninit);
	expect_function_call (__res_nclose);

	ret = query_dns (".err5.", jwt_buf);
	assert_int_equal (ret, -1);
}

void test_allow_insecure_conn ()
{
	int insecure;
	char *server_addr;
	unsigned int port;
	ParodusCfg *cfg = get_parodus_cfg();
#ifdef FEATURE_DNS_QUERY
	cfg->record_jwt_file = NULL;
#endif
	parStrncpy (cfg->hw_mac, "aabbccddeeff", sizeof(cfg->hw_mac));
	parStrncpy (cfg->dns_txt_url, "test.mydns.mycom.net", sizeof(cfg->dns_txt_url));
	cfg->jwt_algo = 1025;

	read_key_from_file ("../../tests/pubkey4.pem", cfg->jwt_key, 4096);

	will_return (__res_ninit, 0);
	expect_function_call (__res_ninit);
	expect_function_call (__res_nclose);

	insecure = allow_insecure_conn (&server_addr, &port);
	free (server_addr);
	assert_int_equal (insecure, 0);


	parStrncpy (cfg->hw_mac, "aabbccddeeff", sizeof(cfg->hw_mac));
	parStrncpy (cfg->dns_txt_url, "err5.mydns.mycom.net", sizeof(cfg->dns_txt_url));

	will_return (__res_ninit, 0);
	expect_function_call (__res_ninit);
	expect_function_call (__res_nclose);

	insecure = allow_insecure_conn (&server_addr, &port);
	assert_int_equal (insecure, TOKEN_ERR_QUERY_DNS_FAIL);

	parStrncpy (cfg->hw_mac, "aabbccddeeff", sizeof(cfg->hw_mac));
	parStrncpy (cfg->dns_txt_url, "test.mydns.mycom.net", sizeof(cfg->dns_txt_url));
	cfg->jwt_algo = 1024;
	parStrncpy (cfg->jwt_key, "xxxxxxxxxx", sizeof(cfg->jwt_key));

	will_return (__res_ninit, 0);
	expect_function_call (__res_ninit);
	expect_function_call (__res_nclose);

	insecure = allow_insecure_conn (&server_addr, &port);
	assert_int_equal (insecure, TOKEN_ERR_JWT_DECODE_FAIL);

	parStrncpy (cfg->hw_mac, "aabbccddeeff", sizeof(cfg->hw_mac));
	parStrncpy (cfg->dns_txt_url, "test.mydns.mycom.net", sizeof(cfg->dns_txt_url));
	cfg->jwt_algo = 4097;
	read_key_from_file ("../../tests/pubkey4.pem", cfg->jwt_key, 4096);

	will_return (__res_ninit, 0);
	expect_function_call (__res_ninit);
	expect_function_call (__res_nclose);

	insecure = allow_insecure_conn (&server_addr, &port);
	assert_int_equal (insecure, TOKEN_ERR_ALGO_NOT_ALLOWED);

}

void test_get_tok()
{
	const char *str0 = "";
	const char *str1 = "none";
	const char *str2 = "none:rs256";
	char result[20];
	const char *next;

	next = get_tok (str0, ':', result, 20);
	assert_ptr_equal (next, NULL);
	assert_int_equal ((int) result[0], 0);

	next = get_tok (str1, ':', result, 20);
	assert_string_equal (result, "none");
	assert_ptr_equal (next, NULL);

	next = get_tok (str2, ':', result, 20);
	assert_string_equal (result, "none");
	next = get_tok (next, ':', result, 20);
	assert_string_equal (result, "rs256");
	assert_ptr_equal (next, NULL);
}

void test_get_algo_mask ()
{
	unsigned mask;

	mask = get_algo_mask ("rs256");
	assert_int_equal ((int) mask, 1024);

	mask = get_algo_mask ("rs512:rs256");
	assert_int_equal ((int) mask, 5120);
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
	cmocka_unit_test(test_get_rr_seq_table),
	cmocka_unit_test(test_assemble_jwt_from_dns),
        cmocka_unit_test(test_query_dns),
        cmocka_unit_test(test_allow_insecure_conn),
        cmocka_unit_test(test_get_tok),
        cmocka_unit_test(test_get_algo_mask),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

