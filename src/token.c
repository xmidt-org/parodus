/**
 * @file token.c
 *
 * @description This file contains operations for using jwt token.
 *
 * Copyright (c) 2015  Comcast
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <netinet/in.h>
#include <resolv.h>
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


#include <cjwt/cjwt.h>
#include "config.h"
#include "parodus_log.h"

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

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

static int is_http(const cjwt_t *jwt)
{
	cJSON *claims = jwt->private_claims;
	cJSON *endpoint = NULL;
	int ret;
	
	//retrieve 'endpoint' claim and check for https/http
	if( claims ){
		endpoint = cJSON_GetObjectItem(claims, ENDPOINT_NAME);
		ret = strncmp(endpoint->valuestring,"http:",5);
		ParodusInfo ("is_http strncmp: %d\n", ret);
		if(endpoint->valuestring && !ret)
		{
			return 1;
		}
	}
		
	return 0;
}

static bool validate_algo(const cjwt_t *jwt)
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


static int nquery(const char* dns_txt_record_id,u_char *nsbuf)
{
	
    int len;
    struct __res_state statp;

    /* Initialize resolver */
		memset (&statp, 0, sizeof(__res_state));
		statp.options |= RES_DEBUG;
    if (res_ninit(&statp) < 0) {
        ParodusError ("res_ninit error: can't initialize statp.\n");
        return (-1);
    }

		ParodusInfo ("Domain : %s\n", dns_txt_record_id);
		len = res_nquery(&statp, dns_txt_record_id, ns_c_any, ns_t_txt, nsbuf, NS_MAXBUF);
    if (len < 0) {
				const char *msg = hstrerror (statp.res_h_errno);
        ParodusError ("Error in res_nquery: %s\n", msg);
        return len;
    }
    res_nclose (&statp);
    if (len >= NS_MAXBUF) {
        ParodusError ("res_nquery error: ns buffer too small.\n");
        return -1;
    }

    return len;

}

static bool valid_b64_char (char c)
{
	if ((c>='A') && (c<='Z'))
		return true;
	if ((c>='a') && (c<='z'))
		return true;
	if ((c>='0') && (c<='9'))
		return true;
  if ((c=='/') || (c=='+'))
		return true;
	return false;
}

static int query_dns(const char* dns_txt_record_id,char *jwt_ans)
{
	u_char *nsbuf;
	ns_msg msg_handle;
	ns_rr rr;
	const char *rr_ptr;
	int rrlen;
	int i, ret;
	int l = -1;
	
	if( !dns_txt_record_id || !jwt_ans )
		return l;
   
	nsbuf = malloc (NS_MAXBUF);
	if (NULL == nsbuf) {
		ParodusError ("Unable to allocate nsbuf in query_dns\n");
		return -1;
	}
	l = nquery(dns_txt_record_id,nsbuf);
	if (l < 0) {
		free (nsbuf);
		return l;
	}
 	
	ret = ns_initparse(nsbuf, l, &msg_handle);
	if (ret != 0) {
		ParodusError ("ns_initparse failed\n");
		free (nsbuf);
		return ret;
	}
	
	l = ns_msg_count(msg_handle, ns_s_an);
	ParodusPrint ("query_dns: ns_msg_count : %d\n",l);
  jwt_ans[0] = 0;
	
	// extract and concatenate all the records in rr
	for (i=0; i<l; i++) {
		ret = ns_parserr(&msg_handle, ns_s_an, i, &rr);
		if (ret != 0) {
			ParodusError ("query_dns: ns_parserr failed: %s\n", strerror (errno));
			free (nsbuf);
			return ret;
		}
		if (ns_rr_type(rr) == ns_t_txt)
    {
      rr_ptr = (const char *)ns_rr_rdata(rr);
			// strip quote or other non-b64 char at front
			if (!valid_b64_char (rr_ptr[0]))	
				rr_ptr++;
			rrlen = strlen (rr_ptr);
			if (rrlen == 0)
				continue;
			// strip new line or other non-b64 char at end
			if (!valid_b64_char (rr_ptr[rrlen-1])) 
				--rrlen;
			// strip quote or other non-b64 char at end
			if (!valid_b64_char (rr_ptr[rrlen-1])) 
				--rrlen;
			if ((strlen(jwt_ans) + rrlen) > JWT_MAXBUF) {
				ParodusError ("query_dns: rr data too big for ns buffer\n");
				free (nsbuf);
				return -1;
			}
			strncat (jwt_ans, rr_ptr, rrlen); 
		}
	}
	ParodusInfo ("query_dns JWT: %s\n", jwt_ans);
	return 0;
}

static void get_dns_txt_record_id (char *buf)
{
	ParodusCfg *cfg = get_parodus_cfg();
	buf[0] = 0;

	sprintf (buf, "%s.%s.webpa.comcast.net", cfg->hw_mac, cfg->dns_id);
	ParodusInfo("dns_txt_record_id %s\n", buf);
}

int allow_insecure_conn(void)
{	
	int insecure=0, ret = -1;
	char *jwt_token, *key;
	cjwt_t *jwt = NULL;
	char dns_txt_record_id[TXT_REC_ID_MAXSIZE];
	
	jwt_token = malloc (NS_MAXBUF);
	if (NULL == jwt_token) {
		ParodusError ("Unable to allocate jwt_token in allow_insecure_conn\n");
		goto end;
	}

	get_dns_txt_record_id (dns_txt_record_id);
	
	//Querying dns for jwt token
	ret = query_dns(dns_txt_record_id, jwt_token);
	if(ret){
		goto end;
	}
	
	//Decoding the jwt token
	key = get_parodus_cfg()->jwt_key;
	ret = cjwt_decode( jwt_token, 0, &jwt, ( const uint8_t * )key,strlen(key) );

	if(ret) {
		if (ret == ENOMEM)
			ParodusError ("Memory allocation failed in JWT decode\n");
		else
			ParodusError ("CJWT decode error\n");
		goto end;
	}else{
		ParodusPrint("Decoded CJWT successfully\n");

		//validate algo from --jwt_algo
		if( validate_algo(jwt) ){
			insecure = is_http(jwt);
		}
	}
	cjwt_destroy(&jwt);
	
end:
	if (NULL != jwt_token)
		free (jwt_token);
	ParodusPrint ("Allow Insecure %d\n", insecure);
	return insecure;
}
/*
int main()
{
#define SERVER_URL "14cfe21463f2.fabric.webpa.comcast.net"
	//char dns_url[64];
	//strcpy(dns_url,get_parodus_cfg()->hw_mac );
	//strcat(dns_url,SERVER_URL);
  printf("Parodus integration\n");
  int insecure = allow_insecure_conn(SERVER_URL);
  printf("allow : %d\n",insecure);

  return(0);
}
*/
