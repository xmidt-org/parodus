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

#include "parodus_log.h"

#define N 4096


 
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define SERVER_URL "14cfe21463f2.fabric.webpa.comcast.net"

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


static int query_token(char *jwt_ans)
{
	u_char nsbuf[N];
    char dispbuf[N];
	char dns_url[64];
    ns_msg msg;
    ns_rr rr;
    int i, l = -1;
    
	if( !jwt_ans )
		ret l;
	strcpy(dns_url,get_parodus_cfg()->hw_mac );
	strcat(dns_url,SERVER_URL);
	
    // HEADER
    ParodusInfo("Domain : %s\n", dns_url);
    // ------

    // TXT RECORD
    ParodusInfo("TXT records : \n");
    l = res_query(dns_url, ns_c_any, ns_t_txt, nsbuf, sizeof(nsbuf));
    if (l < 0)
    {
      perror(SERVER_URL);
    }
    ns_initparse(nsbuf, l, &msg);
    l = ns_msg_count(msg, ns_s_an);
    for (i = 0; i < l; i++)
    {
      ns_parserr(&msg, ns_s_an, 0, &rr);
      ns_sprintrr(&msg, &rr, NULL, NULL, dispbuf, sizeof(dispbuf));
      ParodusInfo("\t%s \n", dispbuf);
	  strcpy(jwt_ans,dispbuf);
    }
	return l;
}

//return true for insecure
//return false for secure
bool allow_insecure_conn()
{	
	bool ret_insecure = false;
	int ret = EINVAL;
	char srv_jwt_token[N];
	ret = query_token(&srv_jwt_token);
	
	cjwt *decoded_jwt = NULL;
	ret = cjwt_decode( srv_jwt_token, 0, &decoded_jwt, ( const uint8_t * )get_parodus_cfg()->jwt_key, strlen(get_parodus_cfg()->jwt_key) );

	if(ret){
		ret_insecure = false;
		goto end;
	}else{
		ParodusInfo("Decoded CJWT\n");
		//validate algo from argument
		//validate endpoint
		//read private claims and get 'endpoint', check https
		//get_parodus_cfg()->jwt_algo, 
	
		//ret_insecure = true; 
	}
	cjwt_destroy(decoded_jwt);
	
end:
	return ret_insecure;
}
