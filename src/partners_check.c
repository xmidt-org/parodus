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
 * @file partners_check.c
 *
 * @description This describes functions to validate partner_id.
 *
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
static void parse_partner_id(char *partnerId, char ***partnersList, int *retCount)
{
    char *token;
    int i = 0, j = 0, count = 0;
    ParodusPrint("********* %s ********\n",__FUNCTION__);
    while(partnerId[i] != '\0')
    {
        if(partnerId[i] == ',')
        {
            count++;
        }
        i++;
    }
    ParodusPrint("count = %d\n", count+1);
    *partnersList = (char **)malloc(sizeof(char *) * count+1);
    while ((token = strsep(&partnerId, ",")) != NULL)
    {
        ParodusPrint("token=%s\n", token);
        (*partnersList)[j] = strdup(token);
        j++;
    }
    *retCount = count+1;
}

int validate_partner_id(wrp_msg_t *msg, partners_t **partnerIds)
{
    int matchFlag = 0, i = 0, count = 0, partnerCount = 0, j = 0;
    char **partnersList = NULL;
    char *partnerId = NULL;
    ParodusPrint("********* %s ********\n",__FUNCTION__);
    char *temp = get_parodus_cfg()->partner_id;
    ParodusPrint("temp = %s\n",temp);
    if(strlen(temp) > 0)
    {
        partnerId = strdup(temp);
    }
    ParodusPrint("partnerId = %s\n",partnerId);
    if(partnerId != NULL)
    {
        parse_partner_id(partnerId, &partnersList, &partnerCount);
    }
    ParodusPrint("partnerCount = %d\n", partnerCount);
    if(partnerCount == 0)
    {
        ParodusPrint("partner_id is not available to validate\n");
        if(partnerId != NULL)
        {
            free(partnerId);
        }
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
                for(j = 0; j<partnerCount; j++)
                {
                    ParodusPrint("partnersList[%lu] = %s\n",j, partnersList[j]);
                    if(strcmp(partnersList[j], msg->u.event.partner_ids->partner_ids[i]) == 0)
                    {
                        ParodusInfo("partner_id match found\n");
                        matchFlag = 1;
                        break;
                    }
                }
                if(matchFlag == 1)
                {
                    break;
                }
            }
            
            if(matchFlag != 1)
            {
                (*partnerIds) = (partners_t *) malloc(sizeof(partners_t) + (sizeof(char *) * (count+partnerCount)));
                (*partnerIds)->count = count+partnerCount;
                for(i = 0; i < count; i++)
                {
                    (*partnerIds)->partner_ids[i] = msg->u.event.partner_ids->partner_ids[i];
                    ParodusPrint("(*partnerIds)->partner_ids[%d] : %s\n",i,(*partnerIds)->partner_ids[i]);
                }
                j = 0;
                for(i = count; i<(count+partnerCount); i++)
                {
                    (*partnerIds)->partner_ids[i] = (char *) malloc(sizeof(char) * 64);
                    parStrncpy((*partnerIds)->partner_ids[i], partnersList[j], 64);
                    ParodusPrint("(*partnerIds)->partner_ids[%d] : %s\n",i,(*partnerIds)->partner_ids[i]);
                    j++;
                }
            }
        }
        else
        {
            ParodusPrint("partner_ids list is NULL\n");
            (*partnerIds) = (partners_t *) malloc(sizeof(partners_t) + (sizeof(char *) * partnerCount));
            (*partnerIds)->count = partnerCount;
            j=0;
            for(i = 0; i<partnerCount; i++)
            {
                (*partnerIds)->partner_ids[i] = (char *) malloc(sizeof(char) * 64);
                parStrncpy((*partnerIds)->partner_ids[i], partnersList[j], 64);
                ParodusPrint("(*partnerIds)->partner_ids[%d] : %s\n",i,(*partnerIds)->partner_ids[i]);
                j++;
            }
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
                for(j = 0; j<partnerCount; j++)
                {
                    ParodusPrint("partnersList[%lu] = %s\n",j, partnersList[j]);
                    if(strcmp(partnersList[j], msg->u.req.partner_ids->partner_ids[i]) == 0)
                    {
                        ParodusInfo("partner_id match found\n");
                        matchFlag = 1;
                        break;
                    }
                }
            }

            if(matchFlag != 1)
            {
                ParodusError("Invalid partner_id %s\n",temp);
                if(partnerId != NULL)
                {
                    free(partnerId);
                }
                return -1;
            }
        }
        else
        {
            ParodusPrint("partner_ids list is NULL\n");
        }
    }
    if(partnerId != NULL)
    {
        free(partnerId);
    }
    return 1;
}

