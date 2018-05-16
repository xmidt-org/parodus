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
#include <nopoll.h>

#include "../src/ParodusInternal.h"
#include "../src/config.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
ParodusMsg *ParodusMsgQ;
bool LastReasonStatus;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

nopoll_bool nopoll_base64_encode(const char *content,int length,char *output, int *output_size)
{
    UNUSED(content); UNUSED(length);  UNUSED(output_size);
    parStrncpy(output, "AWYFUJHUDUDKJDDRDKUIIKORE\nSFJLIRRSHLOUTDESTDJJITTESLOIUHJGDRS\nGIUY&%WSJ", (size_t) *output_size);
    function_called();
    return (nopoll_bool)(intptr_t)mock();
}

char *get_global_reconnect_reason()
{
    function_called();
    return (char *)(intptr_t)mock();
}	
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_getWebpaConveyHeader()
{
    ParodusCfg cfg;
    memset(&cfg, 0, sizeof(ParodusCfg));
    cfg.hw_model = strdup ("TG1682");
    cfg.hw_serial_number = strdup ("Fer23u948590");
    cfg.hw_manufacturer = strdup ("ARRISGroup,Inc.");
    cfg.hw_mac = strdup ("123567892366");
    cfg.hw_last_reboot_reason = strdup ("unknown");
    cfg.fw_name = strdup ("2.364s2");
    cfg.webpa_path_url = strdup ("/api/v2/device");
    cfg.webpa_url = strdup ("localhost");
    cfg.webpa_interface_used = strdup ("eth0");
    cfg.webpa_protocol = strdup ("WebPA-1.6");
    cfg.webpa_uuid = strdup ("1234567-345456546");
    cfg.flags = 0;
    cfg.boot_time = 423457;
    cfg.webpa_ping_timeout = 30;
    cfg.webpa_backoff_max = 255;
    set_parodus_cfg(&cfg);
    
    will_return(get_global_reconnect_reason, (intptr_t)"Ping-Miss");
    expect_function_call(get_global_reconnect_reason);
    
    will_return(nopoll_base64_encode, nopoll_true);
    expect_function_call(nopoll_base64_encode);
    getWebpaConveyHeader();
    clean_up_parodus_cfg(&cfg);
}

void err_getWebpaConveyHeader()
{
    ParodusCfg cfg;
    memset(&cfg, 0, sizeof(ParodusCfg));
    set_parodus_cfg(&cfg);
    
    will_return(get_global_reconnect_reason, (intptr_t)NULL);
    expect_function_call(get_global_reconnect_reason);
    will_return(nopoll_base64_encode, nopoll_false);
    expect_function_call(nopoll_base64_encode);
    getWebpaConveyHeader();
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_getWebpaConveyHeader),
        cmocka_unit_test(err_getWebpaConveyHeader),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
