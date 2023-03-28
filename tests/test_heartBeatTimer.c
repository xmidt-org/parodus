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

#include "../src/heartBeat.h"
#include "../src/parodus_log.h"

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/
void test_heartBeatTimer() {
	/* get heartBeat timer's initial value */
	unsigned int heartBeatTimer =5;
	heartBeatTimer = get_heartBeatTimer();
	ParodusInfo("heartBeatTimer initial vaule is: %d\n", heartBeatTimer);
	assert_int_equal(heartBeatTimer, 0);

	/* increment heartbeat timer value and check whether its returning modified value or not*/
	unsigned int inc_time_ms =5;
	increment_heartBeatTimer(inc_time_ms);
	heartBeatTimer = get_heartBeatTimer();
	ParodusInfo("heartBeatTimer incremented to: %d\n", heartBeatTimer);
	assert_int_equal(heartBeatTimer,5);

	/* reset heartBeat timer to 0 */
	reset_heartBeatTimer();
	heartBeatTimer = get_heartBeatTimer();
	ParodusInfo("heartBeatTimer reset to: %d\n", heartBeatTimer);
	assert_int_equal(heartBeatTimer,0);
}

void *test_mutexIncrementTimer() {
	unsigned int inc_time_ms =5;
	increment_heartBeatTimer(inc_time_ms);
	return NULL;
}

void *test_mutexResetTimer() {
	reset_heartBeatTimer();
	return NULL;
}
void test_mutexHeartBeatTimer() {
	unsigned int heartBeatTimer;
	pthread_t thread[3];

	pthread_create(&thread[0], NULL, test_mutexIncrementTimer, NULL);
	pthread_create(&thread[1], NULL, test_mutexIncrementTimer, NULL);

	pthread_join(thread[0], NULL);
	pthread_join(thread[1], NULL);

	/* After execution of both the threads check the value of timer */
	heartBeatTimer = get_heartBeatTimer();
	ParodusInfo("Threads execution is completed, heartBeatTimer is: %d\n", heartBeatTimer);
	assert_int_equal(heartBeatTimer, 10);

	pthread_create(&thread[2], NULL, test_mutexResetTimer, NULL);
	pthread_join(thread[2], NULL);

	heartBeatTimer = get_heartBeatTimer();
	ParodusInfo("heartBeatTimer reset to: %d\n", heartBeatTimer);
	assert_int_equal(heartBeatTimer, 0);
}
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_heartBeatTimer),
        cmocka_unit_test(test_mutexHeartBeatTimer),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
