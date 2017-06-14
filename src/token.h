/**
 * @file token.h
 *
 * @description This file contains apis and error codes for using jwt token.
 *
 * Copyright (c) 2015  Comcast
 */

#ifndef _TOKEN_H_
#define _TOKEN_H_

/** 
 * @brief token error rtn codes
 * 
 */
typedef enum {
	TOKEN_ERR_MEMORY_FAIL = -999,
	TOKEN_ERR_QUERY_DNS_FAIL = -101,
	TOKEN_ERR_JWT_DECODE_FAIL = -102,
	TOKEN_ERR_ALGO_NOT_ALLOWED = -103,
	TOKEN_ERR_INVALID_JWT_CONTENT = -104,
	TOKEN_ERR_JWT_EXPIRED = -105

} token_error_t;


/**
 * query the dns server, obtain a jwt, determine if insecure
 * connections can be allowed. 
 * 
 * @return 1 if insecure connection is allowed, 0 if not,
*    or one of the error codes given above. 
*/ 
int allow_insecure_conn(void);


#endif
