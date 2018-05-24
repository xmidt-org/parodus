/**
 *  Copyright 2010-2016 Comcast Cable Communications Management, LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <CUnit/Basic.h>
#include <cmocka.h>
#include <pthread.h>
#include "../src/upstream.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
UpStreamMsg *UpStreamMsgQ = NULL;
pthread_mutex_t nano_mut;
pthread_cond_t nano_con;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

UpStreamMsg * get_global_UpStreamMsgQ(void)
{
    return UpStreamMsgQ;
}

void set_global_UpStreamMsgQ(UpStreamMsg * UpStreamQ)
{
    UpStreamMsgQ = UpStreamQ;
}

pthread_cond_t *get_global_nano_con(void)
{
    return &nano_con;
}

pthread_mutex_t *get_global_nano_mut(void)
{
    return &nano_mut;
}


/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
