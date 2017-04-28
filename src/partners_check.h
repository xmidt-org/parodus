/**
 * @file partners_check.h
 *
 * @description This describes functions to validate partner_id.
 *
 * Copyright (c) 2015  Comcast
 */
 
#ifndef _PARTNERS_CHECK_H_
#define _PARTNERS_CHECK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "wrp-c.h"

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

