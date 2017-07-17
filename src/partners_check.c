/**
 * @file partners_check.c
 *
 * @description This describes functions to validate partner_id.
 *
 * Copyright (c) 2015  Comcast
 */

#include "ParodusInternal.h"
#include "config.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             Internal Functions                             */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             External functions                             */
/*----------------------------------------------------------------------------*/
int validate_partner_id(wrp_msg_t *msg, partners_t **partnerIds)
{
    int matchFlag = 0, i = 0, count = 0;
    ParodusPrint("********* %s ********\n",__FUNCTION__);
    char *partnerId = get_parodus_cfg()->partner_id;
    if(strlen(partnerId) <= 0)
    {
        ParodusPrint("partner_id is not available to validate\n");
        return 0;
    }
    
    if(msg->msg_type == WRP_MSG_TYPE__EVENT)
    {
        if(msg->u.event.partner_ids != NULL)
        {
            count = (int) msg->u.event.partner_ids->count;
            ParodusPrint("partner_ids count is %d\n",count);
            for(i = 0; i < count; i++)
            {
                if(strcmp(partnerId, msg->u.event.partner_ids->partner_ids[i]) == 0)
                {
                    ParodusInfo("partner_id match found\n");
                    matchFlag = 1;
                    break;
                }
            }
            
            if(matchFlag != 1)
            {
                (*partnerIds) = (partners_t *) malloc(sizeof(partners_t));
                (*partnerIds)->count = count+1;
                for(i = 0; i < count; i++)
                {
                    (*partnerIds)->partner_ids[i] = msg->u.event.partner_ids->partner_ids[i];
                    ParodusPrint("(*partnerIds)->partner_ids[%d] : %s\n",i,(*partnerIds)->partner_ids[i]);
                }
                (*partnerIds)->partner_ids[count] = (char *) malloc(sizeof(char) * 64);
                parStrncpy((*partnerIds)->partner_ids[count], partnerId, 64);
                ParodusPrint("(*partnerIds)->partner_ids[%d] : %s\n",count,(*partnerIds)->partner_ids[count]);
            }
        }
        else
        {
            ParodusPrint("partner_ids list is NULL\n");
            (*partnerIds) = (partners_t *) malloc(sizeof(partners_t));
            (*partnerIds)->count = 1;
            (*partnerIds)->partner_ids[0] = (char *) malloc(sizeof(char) * 64);
            parStrncpy((*partnerIds)->partner_ids[0], partnerId, 64);
            ParodusPrint("(*partnerIds)->partner_ids[0] : %s\n",(*partnerIds)->partner_ids[0]);
        }
    }
    else if(msg->msg_type == WRP_MSG_TYPE__REQ)
    {
        if(msg->u.req.partner_ids != NULL)
        {
            count = (int) msg->u.req.partner_ids->count;
            ParodusPrint("partner_ids count is %d\n",count);
            for(i = 0; i < count; i++)
            {
                if(strcmp(partnerId, msg->u.req.partner_ids->partner_ids[i]) == 0)
                {
                    ParodusInfo("partner_id match found\n");
                    matchFlag = 1;
                    break;
                }
            }

            if(matchFlag != 1)
            {
                ParodusError("Invalid partner_id %s\n",partnerId);
                return -1;
            }
        }
        else
        {
            ParodusPrint("partner_ids list is NULL\n");
        }
    }
    
    return 1;
}

