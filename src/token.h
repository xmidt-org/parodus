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
 * @file token.h
 *
 * @description This file contains apis and error codes for using jwt token.
 *
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
	TOKEN_ERR_NO_EXPIRATION = -105,
	TOKEN_ERR_JWT_EXPIRED = -106,
	TOKEN_ERR_BAD_ENDPOINT = -107,
	TOKEN_NO_DNS_QUERY = -1
} token_error_t;


/**

Connection Logic:

----- Criteria  -----

Feature					FeatureDnsQuery enabled
QueryGood				Dns query succeeds, jwt decodes and is valid and unexpired
Endpt starts		Endpoint specified in the jwt starts with http:// or https://
Config Secflag	secureFlag in config is set.  Currently always set.


----- Actions -----

Default			Securely connect to the default URL, specified
						in the config
Secure			Securely connect to the endpoint given in the jwt
Insecure		Insecurely connect to the endpoint given in the jwt

 
----- Logic Table -----

Feature		Query		Endpt		Config			Action
					Good		Claim		SecFlag

No																		Default
Yes				No													Default
Yes				Yes			https								Secure
Yes				Yes			http		False				Insecure
Yes				Yes			http		True				Default
*/


/**
 * query the dns server, obtain a jwt, determine if insecure
 * connections can be allowed. 
 * 
 * @return 1 if insecure connection is allowed, 0 if not,
*    or one of the error codes given above. 
*/ 
int allow_insecure_conn(char *url_buf, int url_buflen);


#endif
