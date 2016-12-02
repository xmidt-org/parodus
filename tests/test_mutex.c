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
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>

#include <CUnit/Basic.h>

#include "../src/ParodusInternal.h"

struct shared_data {
    noPollPtr mutex;
    int number;
};

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void* a(void *in)
{
    struct shared_data *data = (struct shared_data*) in;

    lockMutex(data->mutex);
    data->number++;
    sleep(1);
    data->number++;
    sleep(1);
    data->number++;
    unlockMutex(data->mutex);

    return NULL;
}

void* b(void *in)
{
    struct shared_data *data = (struct shared_data*) in;

    lockMutex(data->mutex);
    data->number+=10;
    sleep(1);
    data->number+=10;
    sleep(1);
    data->number+=10;
    unlockMutex(data->mutex);

    return NULL;
}

void test_Mutex()
{
    volatile struct shared_data data;
    pthread_t thread[2];

    data.number = 0;
    data.mutex = createMutex();

    pthread_create(&thread[0], NULL, a, (void*)(&data));
    pthread_create(&thread[1], NULL, b, (void*)(&data));

    pthread_join(thread[0], NULL);
    pthread_join(thread[1], NULL);

    destroyMutex(data.mutex);

    CU_ASSERT(33 == data.number);
}

void add_suites( CU_pSuite *suite )
{
    ParodusInfo("--------Start of Test Cases Execution ---------\n");
    *suite = CU_add_suite( "tests", NULL, NULL );
    CU_add_test( *suite, "Test checkHostIp()", test_Mutex );
}




/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( void )
{
    unsigned rv = 1;
    CU_pSuite suite = NULL;

    if( CUE_SUCCESS == CU_initialize_registry() ) {
        add_suites( &suite );

        if( NULL != suite ) {
            CU_basic_set_mode( CU_BRM_VERBOSE );
            CU_basic_run_tests();
            ParodusPrint( "\n" );
            CU_basic_show_failures( CU_get_failure_list() );
            ParodusPrint( "\n\n" );
            rv = CU_get_number_of_tests_failed();
        }

        CU_cleanup_registry();

    }

    return rv;
}
