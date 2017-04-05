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
    strcpy(output, "AWYFUJHUDUDKJDDRDKUIIKORE\nSFJLIRRSHLOUTDESTDJJITTESLOIUHJGDRS\nGIUY&%WSJ");
    function_called();
    return (nopoll_bool)mock();
}

char *get_global_reconnect_reason()
{
    function_called();
    return (char *)mock();
}	
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_getWebpaConveyHeader()
{
    ParodusCfg cfg;

    strcpy(cfg.hw_model, "TG1682");
    strcpy(cfg.hw_serial_number, "Fer23u948590");
    strcpy(cfg.hw_manufacturer , "ARRISGroup,Inc.");
    strcpy(cfg.hw_mac , "123567892366");
    strcpy(cfg.hw_last_reboot_reason , "unknown");
    strcpy(cfg.fw_name , "2.364s2");
    strcpy(cfg.webpa_path_url , "/api/v2/device");
    strcpy(cfg.webpa_url , "localhost");
    strcpy(cfg.webpa_interface_used , "eth0");
    strcpy(cfg.webpa_protocol , "WebPA-1.6");
    strcpy(cfg.webpa_uuid , "1234567-345456546");
    cfg.secureFlag = 1;
    cfg.boot_time = 423457;
    cfg.webpa_ping_timeout = 30;
    cfg.webpa_backoff_max = 255;
    set_parodus_cfg(&cfg);
    
    will_return(get_global_reconnect_reason, "Ping-Miss");
    expect_function_call(get_global_reconnect_reason);
    
    will_return(nopoll_base64_encode, nopoll_true);
    expect_function_call(nopoll_base64_encode);
    getWebpaConveyHeader();
}

void err_getWebpaConveyHeader()
{
    ParodusCfg cfg;
    memset(&cfg, 0, sizeof(ParodusCfg));
    set_parodus_cfg(&cfg);
    
    will_return(get_global_reconnect_reason, NULL);
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
