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
    strncpy(output, "AWYFUJHUDUDKJDDRDKUIIKORE\nSFJLIRRSHLOUTDESTDJJITTESLOIUHJGDRS\nGIUY&%WSJ", strlen("AWYFUJHUDUDKJDDRDKUIIKORE\nSFJLIRRSHLOUTDESTDJJITTESLOIUHJGDRS\nGIUY&%WSJ")+1);
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

    strncpy(cfg.hw_model, "TG1682", strlen("TG1682")+1);
    strncpy(cfg.hw_serial_number, "Fer23u948590", strlen("Fer23u948590")+1);
    strncpy(cfg.hw_manufacturer , "ARRISGroup,Inc.", strlen("ARRISGroup,Inc.")+1);
    strncpy(cfg.hw_mac , "123567892366", strlen("123567892366")+1);
    strncpy(cfg.hw_last_reboot_reason , "unknown", strlen("unknown")+1);
    strncpy(cfg.fw_name , "2.364s2", strlen("2.364s2")+1);
    strncpy(cfg.webpa_path_url , "/api/v2/device", strlen("/api/v2/device")+1);
    strncpy(cfg.webpa_url , "localhost", strlen("localhost")+1);
    strncpy(cfg.webpa_interface_used , "eth0", strlen("eth0")+1);
    strncpy(cfg.webpa_protocol , "WebPA-1.6", strlen("WebPA-1.6")+1);
    strncpy(cfg.webpa_uuid , "1234567-345456546", strlen("1234567-345456546")+1);
    cfg.secureFlag = 1;
    cfg.boot_time = 423457;
    cfg.webpa_ping_timeout = 30;
    cfg.webpa_backoff_max = 255;
    set_parodus_cfg(&cfg);
    
    will_return(get_global_reconnect_reason, (intptr_t)"Ping-Miss");
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
