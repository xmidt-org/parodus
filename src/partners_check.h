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
 * @file partners_check.h
 *
 * @description This describes functions to validate partner_id.
 *
 */
 
#ifndef _PARTNERS_CHECK_H_
#define _PARTNERS_CHECK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "wrp-c.h"

#define PARTNER_ID_TEST_PARTNER		"test-partner"
#define PARTNER_ID_COMCAST		"comcast"


/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

int validate_partner_id(wrp_msg_t *msg, partners_t **partnerIds);

#ifdef __cplusplus
}
#endif


#endif /* _PARTNERS_CHECK_H_ */

