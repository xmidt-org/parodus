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
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <CUnit/Basic.h>

#include "../src/ParodusInternal.h"
#include "../src/mutex.h"

struct shared_data {
    noPollPtr mutex;
    int number;
};

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr)
{
    UNUSED(attr); UNUSED(mutex);
    return (int)mock();
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    UNUSED(mutex);
    return (int)mock();
}
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
    will_return(pthread_mutex_init, 0);
    data.mutex = createMutex();

    pthread_create(&thread[0], NULL, a, (void*)(&data));
    pthread_create(&thread[1], NULL, b, (void*)(&data));

    pthread_join(thread[0], NULL);
    pthread_join(thread[1], NULL);

    will_return(pthread_mutex_destroy, 0);
    destroyMutex(data.mutex);
    assert_int_equal(33, data.number);
}

void err_mutex()
{
    static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    noPollPtr mutex = &mtx;

    will_return(pthread_mutex_destroy, -1);
    destroyMutex(mutex);

    will_return(pthread_mutex_init, -1);
    mutex = createMutex();
}

void err_mutexNull()
{
    lockMutex(NULL);
    unlockMutex(NULL);
    destroyMutex(NULL);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( void )
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_Mutex),
        cmocka_unit_test(err_mutex),
        cmocka_unit_test(err_mutexNull),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
