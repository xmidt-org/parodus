/**
 * Copyright 2015 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
/**
 * @file token.c
 *
 * @description This file contains operations for using jwt token.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <netinet/in.h>
#ifdef FEATURE_DNS_QUERY
#include <ucresolv.h>
#endif
//#include <res_update.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>

#include <assert.h>
#include <sys/types.h>
#include <sys/param.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#include <cjwt/cjwt.h>
#include "token.h"
#include "config.h"
#include "parodus_log.h"
#include "ParodusInternal.h"

#define JWT_MAXBUF	8192

#ifdef NS_MAXMSG
#if NS_MAXMSG > JWT_MAXBUF
#define NS_MAXBUF JWT_MAXBUF
#else
#define NS_MAXBUF NS_MAXMSG
#endif
#else
#define NS_MAXBUF JWT_MAXBUF
#endif

#define TXT_REC_ID_MAXSIZE	128

#define MAX_RR_RECS 10
#define SEQ_TABLE_SIZE (MAX_RR_RECS + 1)

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define ENDPOINT_NAME "endpoint"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
#ifdef FEATURE_DNS_QUERY

extern int __res_ninit(res_state statp);
extern void __res_nclose(res_state statp);
extern int __res_nquery(res_state statp,
	   const char *name,	/* domain name */
	   int class, int type,	/* class and type of query */
	   u_char *answer,	/* buffer to put answer */
	   int anslen);		/* size of answer buffer */

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/


static void show_times (time_t exp_time, time_t cur_time)
{
	char exp_buf[30];
	char cur_buf[30];
	ctime_r (&exp_time, exp_buf);
	exp_buf[strlen(exp_buf)-1] = 0;
	ctime_r (&cur_time, cur_buf);
	cur_buf[strlen(cur_buf)-1] = 0;
	ParodusInfo ("Exp: %d %s, Current: %d %s\n", 
		(int)exp_time, exp_buf+4, (int)cur_time, cur_buf+4);
}

// returns 1 if insecure, 0 if secure, < 0 if error
int analyze_jwt (const cjwt_t *jwt, char **url_buf, unsigned int *port)
{
	cJSON *claims = jwt->private_claims;
	cJSON *endpoint = NULL;
	time_t exp_time, cur_time;
	int http_match;

	if (!claims) {
		ParodusError ("Private claims not found in jwt\n");
		return TOKEN_ERR_INVALID_JWT_CONTENT;
	}

	endpoint = cJSON_GetObjectItem(claims, ENDPOINT_NAME);
	if (!endpoint) {
		ParodusError ("Endpoint claim not found in jwt\n");
		return TOKEN_ERR_INVALID_JWT_CONTENT;
	}

	ParodusInfo ("JWT endpoint: %s\n", endpoint->valuestring);
	exp_time = jwt->exp.tv_sec;
	if (0 == exp_time) {
		ParodusError ("exp not found in JWT payload\n");
		return TOKEN_ERR_NO_EXPIRATION;
	} else {
		cur_time = time(NULL);
		show_times (exp_time, cur_time);
		if (exp_time < cur_time) {
			ParodusError ("JWT has expired\n");
			OnboardLog ("JWT has expired\n");
			return TOKEN_ERR_JWT_EXPIRED;
		}
	}
	http_match = parse_webpa_url (endpoint->valuestring,
		url_buf, port);
	if (http_match < 0) {
		ParodusError ("Invalid endpoint claim in JWT\n");
		OnboardLog("Invalid endpoint claim in JWT\n");
		return TOKEN_ERR_BAD_ENDPOINT;
	}
	ParodusInfo ("JWT is_http strncmp: %d\n", http_match);

	return http_match;
}

bool validate_algo(const cjwt_t *jwt)
{
	// return true if jwt->header.alg is included in the set
	// of allowed algorithms specified by cfg->jwt_algo
	ParodusCfg *cfg = get_parodus_cfg();
        int alg = jwt->header.alg;
        int alg_mask;

	if ((alg < 0) || (alg >= num_algorithms))
		return false;
	alg_mask = 1<<alg;
	if ((alg_mask & cfg->jwt_algo) == 0) {
		ParodusError ("Algorithm %d not allowed (mask %d)\n", alg, alg_mask); 
		return false;
	}
	return true;
}


int nquery(const char* dns_txt_record_id, u_char *nsbuf)
{
	
    int len;
    struct __res_state statp;

    /* Initialize resolver */
		memset (&statp, 0, sizeof(__res_state));
		if (NULL == nsbuf) {
			ParodusError ("nquery: nsbuf is NULL\n");
			return (-1);
		}
		statp.options = RES_DEBUG;
    if (__res_ninit(&statp) < 0) {
        ParodusError ("res_ninit error: can't initialize statp.\n");
        return (-1);
    }

		ParodusInfo ("nquery: domain : %s\n", dns_txt_record_id);
		memset (nsbuf, 0, NS_MAXBUF);
		len = __res_nquery(&statp, dns_txt_record_id, ns_c_in, ns_t_txt, nsbuf, NS_MAXBUF);
    if (len < 0) {
				if (0 != statp.res_h_errno) {
					const char *msg = hstrerror (statp.res_h_errno);
        	ParodusError ("Error in res_nquery: %s\n", msg);
				}
        return len;
    }
    __res_nclose (&statp);
	ParodusInfo ("nquery: nsbuf (1) 0x%lx\n", (unsigned long) nsbuf);
    if (len >= NS_MAXBUF) {
        ParodusError ("res_nquery error: ns buffer too small.\n");
        return -1;
    }

    return len;

}

bool valid_b64_char (char c)
{
	if ((c>='A') && (c<='Z'))
		return true;
	if ((c>='a') && (c<='z'))
		return true;
	if ((c>='0') && (c<='9'))
		return true;
  if ((c=='/') || (c=='+') || (c=='-') || (c=='_'))
		return true;
	return false;
}

static bool is_digit (char c)
{
	return (bool) ((c>='0') && (c<='9'));
}

// strip quotes and newlines from rr rec
const char *strip_rr_data (const char *rr_ptr, int *rrlen)
{
	int len = *rrlen;
	const char *optr = rr_ptr;
	char c;

	if (len > 0) {
		c = optr[0];
		if (!is_digit(c)) {
			optr++;
			len--;
		}
	}
	if (len > 0) {
		if (!valid_b64_char (optr[len-1]))
			len--;
	}
	if (len > 0) {
		if (!valid_b64_char (optr[len-1]))
			len--;
	}
	*rrlen = len;
	return optr;
}

// return offset to seq number in record
// return -1 if not found, -2 if invalid fmt
int find_seq_num (const char *rr_ptr, int rrlen)
{
	char c;
	int i;
	int digit_ct = 0;

	for (i=0; i<rrlen; i++)
	{
		c = rr_ptr[i];
		if (c == ':') {
			if (digit_ct >= 2)
				return i - 2;
			else
				return -2;
		}
		if (is_digit (c))
			digit_ct++;
		else
			digit_ct = 0;
	}
	return -1;		
}

// get seq num in rr rec
// return -1 if not formatted correctly
int get_rr_seq_num (const char *rr_ptr, int rrlen)
{
	char c;
	int lo, hi;
	if (rrlen < 3)
		return -1;
	if (rr_ptr[2] != ':')
		return -1;
	c = rr_ptr[0];
	if (is_digit (c))
		hi = c - '0';
	else
		return -1;
	c = rr_ptr[1];
	if (is_digit (c))
		lo = c - '0';
	else
		return -1;
	return (10*hi) + lo;
}

// scan rr recs and build seq table using seq numbers in the recs
// return num_txt_recs
int get_rr_seq_table (ns_msg *msg_handle, int num_rr_recs, rr_rec_t *seq_table)
{
	ns_rr rr;
	const char *rr_ptr;
	int seq_pos;
	int rrlen;
	int i, ret, seq_num;
	int num_txt_recs = 0;

	if (num_rr_recs > MAX_RR_RECS) {
		ParodusError ("num rr recs (%d) to big, > %d\n", num_rr_recs, MAX_RR_RECS);
		return -1;
	}
	// clear seq table
	for (i=0; i<SEQ_TABLE_SIZE; i++)
	{
		seq_table[i].rr_ptr = NULL;
		seq_table[i].rr_len = 0;
	}

	// extract and concatenate all the records in rr
	for (i=0; i<num_rr_recs; i++) {
		ret = ns_parserr(msg_handle, ns_s_an, i, &rr);
		if (ret != 0) {
			ParodusError ("query_dns: ns_parserr failed: %s\n", strerror (errno));
			return ret;
		}
		if (ns_rr_type(rr) != ns_t_txt)
    	continue;
		++num_txt_recs;
    rr_ptr = (const char *)ns_rr_rdata(rr);
    rrlen = ns_rr_rdlen(rr);
		ParodusPrint ("Found rr rec type %d: %s\n", ns_t_txt, rr_ptr);
		rr_ptr = strip_rr_data (rr_ptr, &rrlen);		
		seq_pos = find_seq_num (rr_ptr, rrlen);
		if (seq_pos == -2) {
			ParodusError ("Invalid seq number in rr record %d\n", i);
			return -1;
		}
		if (seq_pos < 0) {
			seq_num = 0;
		} else {
			rr_ptr += seq_pos;
			rrlen -= seq_pos;
			seq_num = get_rr_seq_num (rr_ptr, rrlen);
		}
		ParodusPrint ("Found seq num %d in rr rec %d\n", seq_num, i);

		if (seq_num < 0) {
			ParodusError ("Seq number not found in rr record %d\n", i);
			return -1;
		}
		if (seq_num > num_rr_recs) {
			ParodusError ("Invalid seq number (too big) in rr record %d\n", i);
			return -1;
		}
		if (NULL != seq_table[seq_num].rr_ptr) {
			ParodusError ("Duplicate rr record number %d\n", seq_num);
			return -1;
		}
		if (seq_num != 0) {
			rr_ptr += 3; // skip the seq number
			rrlen -= 3;
		}
		seq_table[seq_num].rr_ptr = rr_ptr;
		seq_table[seq_num].rr_len = rrlen;
	}

	if (NULL != seq_table[0].rr_ptr) {
		// sequence-less record should not be used when there
		// are multiple records
		if (num_txt_recs > 1) {
			ParodusError ("Seq number not found in rr record\n");
			return -1;
		}
		// when there is only one record, use the sequence-less record
		seq_table[1].rr_ptr = seq_table[0].rr_ptr;
		seq_table[1].rr_len = seq_table[0].rr_len;
	}

	// check if we got them all
	for (i=1; i<num_txt_recs; i++) {
		if (NULL == seq_table[i].rr_ptr) {
			ParodusError ("Missing rr record number %d\n", i+1);
			return -1;
		}
	}
	return num_txt_recs;
}

int assemble_jwt_from_dns (ns_msg *msg_handle, int num_rr_recs, char *jwt_ans)
{
	// slot 0 in the seq table is for the sequence-less record that
	// you get when there is only one record.
	rr_rec_t seq_table[SEQ_TABLE_SIZE];
	int i;
	int num_txt_recs = get_rr_seq_table (msg_handle, num_rr_recs, seq_table);

	if (num_txt_recs < 0)
		return num_txt_recs;
	ParodusPrint ("Found %d TXT records\n", num_txt_recs);
	jwt_ans[0] = 0;
	for (i=1; i<=num_txt_recs; i++)
		strncat (jwt_ans, seq_table[i].rr_ptr, seq_table[i].rr_len);
	return 0;
}

int query_dns(const char* dns_txt_record_id,char *jwt_ans)
{
	u_char *nsbuf;
	ns_msg msg_handle;
	int ret;
	int l = -1;
	
	if( !dns_txt_record_id || !jwt_ans )
		return l;
   
	nsbuf = (u_char *) malloc (NS_MAXBUF);
	ParodusInfo ("nsbuf (1) 0x%lx\n", (unsigned long) nsbuf);
	if (NULL == nsbuf) {
		ParodusError ("Unable to allocate nsbuf in query_dns\n");
		return TOKEN_ERR_MEMORY_FAIL;
	}
	
	l = nquery(dns_txt_record_id,nsbuf);
	if (l < 0) {
		ParodusError("nquery returns error: l value is %d\n", l);
		free (nsbuf);
		return l;
	}
	ParodusInfo ("nsbuf (2) 0x%lx\n", (unsigned long) nsbuf);

/*--
	memset((void *) &msg_handle, 0x5e, sizeof (ns_msg));
	ParodusInfo ("nsbuf (3) 0x%lx\n", (unsigned long) nsbuf);
	msg_handle._msg = nsbuf;
*/
	ParodusInfo ("ns_initparse, msglen %d, nsbuf 0x%lx\n",
    l, (unsigned long) nsbuf);
	ret = ns_initparse((const u_char *) nsbuf, l, &msg_handle);
	if (ret != 0) {
		ParodusError ("ns_initparse failed\n");
		free (nsbuf);
		return ret;
	}
	
	ParodusInfo ("ns_msg_count\n");
	l = ns_msg_count(msg_handle, ns_s_an);
	ParodusInfo ("query_dns: ns_msg_count : %d\n",l);
  jwt_ans[0] = 0;
	
	ret = assemble_jwt_from_dns (&msg_handle, l, jwt_ans);
	free (nsbuf);
	if (ret == 0)
		ParodusInfo ("query_dns JWT: %s\n", jwt_ans);
	return ret;
}

static void get_dns_txt_record_id (char *buf)
{
	ParodusCfg *cfg = get_parodus_cfg();
	buf[0] = 0;

	sprintf (buf, "%s.%s", cfg->hw_mac, cfg->dns_txt_url);
	ParodusInfo("dns_txt_record_id %s\n", buf);
}
#endif

int allow_insecure_conn(char **server_addr, unsigned int *port)
{
#ifdef FEATURE_DNS_QUERY	
	int insecure=0, ret = -1;
	char *jwt_token, *key;
	cjwt_t *jwt = NULL;
	char dns_txt_record_id[TXT_REC_ID_MAXSIZE];
	
	jwt_token = malloc (NS_MAXBUF);
	if (NULL == jwt_token) {
		ParodusError ("Unable to allocate jwt_token in allow_insecure_conn\n");
		insecure = TOKEN_ERR_MEMORY_FAIL;
		goto end;
	}

	get_dns_txt_record_id (dns_txt_record_id);
	
	ret = query_dns(dns_txt_record_id, jwt_token);
	ParodusPrint("query_dns returns %d\n", ret);
		
	if(ret){
		ParodusError("Failed in DNS query\n");
		if (ret == TOKEN_ERR_MEMORY_FAIL){
			insecure = ret;
		} 
		else{
			insecure = TOKEN_ERR_QUERY_DNS_FAIL;
		}
		goto end;
	}
	//Decoding the jwt token
	key = get_parodus_cfg()->jwt_key;
	ret = cjwt_decode( jwt_token, 0, &jwt, ( const uint8_t * )key,strlen(key) );

	if(ret) {
		if (ret == ENOMEM) {
			ParodusError ("Memory allocation failed in JWT decode\n");
		} else {
			ParodusError ("CJWT decode error\n");
		}
		insecure = TOKEN_ERR_JWT_DECODE_FAIL;
		goto end;
	}

	ParodusPrint("Decoded CJWT successfully\n");

	//validate algo from --jwt_algo
	if( validate_algo(jwt) ) {
		insecure = analyze_jwt (jwt, server_addr, port);
	} else {
		insecure = TOKEN_ERR_ALGO_NOT_ALLOWED;
	}

	if (insecure >= 0) {
		char *claim_str = cJSON_Print (jwt->private_claims);
		ParodusInfo ("JWT claims: %s\n", claim_str);
		free (claim_str);
	}
	cjwt_destroy(&jwt);
	
end:
	if (NULL != jwt_token)
		free (jwt_token);
#else
  (void) server_addr;
  (void) port;
  int insecure = TOKEN_NO_DNS_QUERY;
#endif
	ParodusPrint ("Allow Insecure %d\n", insecure);
	return insecure;
}
