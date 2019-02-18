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
 * @file auth_token.h
 *
 * @description This file is to fetch authorization token during parodus cloud connection.
 *
 */
 
#ifndef _AUTH_TOKEN_H_ 
#define _AUTH_TOKEN_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

struct token_data {
    size_t size;
    char* data;
};

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

int requestNewAuthToken(char *newToken, size_t len, int r_count);
void getAuthToken(ParodusCfg *cfg);
size_t write_callback_fn(void *buffer, size_t size, size_t nmemb, struct token_data *data);
char* generate_trans_uuid();

#ifdef __cplusplus
}
#endif

#endif
