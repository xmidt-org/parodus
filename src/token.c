/**
 * @file token.c
 *
 * @description This file contains operations for using jwt token.
 *
 * Copyright (c) 2015  Comcast
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <resolv.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>

//#include "cjwt.h"
#include "../src/cjwt.h"

#define N 4096
 
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
/* none */

static int is_http(const cjwt_t *jwt)
{
	cJSON *claims = jwt->private_claims;
	cJSON *endpoint = NULL;
	
	//retrieve 'endpoint' claim and check for https/http
	if( claims ){
		endpoint = cJSON_GetObjectItem(claims, ENDPOINT_NAME);
		if(endpoint->valuestring && !strncmp(endpoint->valuestring,"http:",5))
		{
			return 1;
		}
	}
		
	return 0;
}

static int validate_algo(const cjwt_t *jwt)
{
	/*
	alg_none = 0,
	alg_es256,
	alg_es384,
	alg_es512,
	alg_hs256,
	alg_hs384,
	alg_hs512,
	alg_ps256,
	alg_ps384,
	alg_ps512,
	alg_rs256,
	alg_rs384,
	alg_rs512
	*/
	//get_parodus_cfg()->jwt_algo,
	if( jwt->header.alg == alg_none)
		return 0;
	return -1;
}

static int query_dns(const char* dnsserv_url,char *jwt_ans)
{
	u_char nsbuf[N];
    char rrbuf[N];
	ns_msg msg;
    ns_rr rr;
    int l = -1;
	
	const char quote[2] = "\"\0";
	char *txt_tok;
	char *prev_txt_token;
    
	if( !dnsserv_url || !jwt_ans )
		return l;
    ParodusInfo("Domain : %s\n", dnsserv_url);
   
    //TXT record
    l = res_query(dnsserv_url, ns_c_any, ns_t_txt, nsbuf, sizeof(nsbuf));
    
    if (l < 0)
    {
      perror(SERVER_URL);
	  return l;
    }
	
    ns_initparse(nsbuf, l, &msg);
    l = ns_msg_count(msg, ns_s_an);
	if(l!=1)
		return -1;
	
	ns_parserr(&msg, ns_s_an, 0, &rr);
    ns_sprintrr(&msg, &rr, NULL, NULL, rrbuf, sizeof(rrbuf));
   
   
	txt_tok = strtok(rrbuf, quote);
	while( txt_tok != NULL ) 
	{
		prev_txt_token = txt_tok; 
		txt_tok = strtok(NULL, quote);
	}
	strcpy(jwt_ans,prev_txt_token);
    
	return 0;
}

int allow_insecure_conn(const char* serv_url)
{	
	int insecure=0, ret = -1;
	char jwt_token[N];
	
	if( !serv_url ){
		goto end;
	}
	
	//Querying dns for jwt token
	ret = query_dns(serv_url, jwt_token);
	if(ret){
		goto end;
	}
	
	ParodusInfo("query_token : %s\n",jwt_token);
	
	//Decoding the jwt token
	cjwt_t *jwt = NULL;
	ret = cjwt_decode( jwt_token, 0, &jwt, ( const uint8_t * )"",0 );

	if(ret){
		goto end;
	}else{
		ParodusInfo("Decoded CJWT\n");
		
		//validate algo from --jwt_algo
		if( !validate_algo(jwt) ){
			insecure = is_http(jwt);
		}
	}
	cjwt_destroy(&jwt);
	
end:
	return insecure;
}
#if 0
int main()
{
	//char dns_url[64];
	//strcpy(dns_url,get_parodus_cfg()->hw_mac );
	//strcat(dns_url,SERVER_URL);
  printf("Parodus integration\n");
  int insecure = allow_insecure_conn(SERVER_URL);
  printf("allow : %d\n",insecure);

  return(0);
}
#endif
