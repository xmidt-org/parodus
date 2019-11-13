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
#include <stdarg.h>

#include <CUnit/Basic.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <assert.h>
#include <wrp-c.h>
#include <pthread.h>
#include "../src/token.h"


/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/


pthread_mutex_t crud_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t crud_con=PTHREAD_COND_INITIALIZER;

pthread_cond_t *get_global_crud_con(void)
{
    return &crud_con;
}

pthread_mutex_t *get_global_crud_mut(void)
{
    return &crud_mut;
}

void addCRUDmsgToQueue(wrp_msg_t *crudMsg)
{
	(void)crudMsg;
	return;
}

void *CRUDHandlerTask()
{
	return NULL;
}

void test_allow_insecure_conn ()
{
	int insecure;
	char *server_Address = NULL;
	unsigned int port;
	insecure = allow_insecure_conn (&server_Address, &port);
	assert_int_equal (insecure, TOKEN_NO_DNS_QUERY);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_allow_insecure_conn),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

