/**
 * Copyright 2018 Comcast Cable Communications Management, LLC
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
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <pthread.h>

#include "../src/close_retry.h"
#include "../src/parodus_log.h"

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_close_retry() {
	/* get close_retry initial value */
	bool close_retry =false;
	close_retry = get_close_retry();
	ParodusInfo("close_retry initial value is: %d\n", close_retry);
	assert_int_equal(close_retry, 0);

	/* set close_retry value and check whether its returning modified value or not*/
	set_close_retry();
	close_retry = get_close_retry();
	ParodusInfo("close_retry modified to: %d\n", close_retry);
	assert_int_equal(close_retry,1);

	/* reset close_retry */
	reset_close_retry();
	close_retry = get_close_retry();
	ParodusInfo("close_retry reset to: %d\n", close_retry);
	assert_int_equal(close_retry,0);
}

void *test_mutex_set_close_retry() {
	set_close_retry();
	return NULL;
}

void *test_mutex_reset_close_retry() {
	reset_close_retry();
	return NULL;
}

void test_mutex_close_retry() {
	bool close_retry;
	pthread_t thread[3];

	pthread_create(&thread[0], NULL, test_mutex_set_close_retry, NULL);
	pthread_create(&thread[1], NULL, test_mutex_set_close_retry, NULL);

	pthread_join(thread[0], NULL);
	pthread_join(thread[1], NULL);

	/* After execution of threads check the value of close_retry */
	close_retry = get_close_retry();
	ParodusInfo("Threads execution is completed, close_retry is: %d\n", close_retry);
	assert_int_equal(close_retry, 1);

	pthread_create(&thread[3], NULL, test_mutex_reset_close_retry, NULL);
	pthread_join(thread[3], NULL);

	close_retry = get_close_retry();
	ParodusInfo("close_retry reset to: %d\n", close_retry);
	assert_int_equal(close_retry, 0);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_close_retry),
        cmocka_unit_test(test_mutex_close_retry)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
