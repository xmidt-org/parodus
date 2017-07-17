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
#include <malloc.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <CUnit/Basic.h>

#include "../src/config.h"
#include "../src/ParodusInternal.h"
#define K_argc 15

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

void test_setParodusConfig()
{
    ParodusCfg cfg;
    memset(&cfg,0,sizeof(cfg));

    parStrncpy(cfg.hw_model, "TG1682", sizeof(cfg.hw_model));
    parStrncpy(cfg.hw_serial_number, "Fer23u948590", sizeof(cfg.hw_serial_number));
    parStrncpy(cfg.hw_manufacturer , "ARRISGroup,Inc.", sizeof(cfg.hw_manufacturer));
    parStrncpy(cfg.hw_mac , "123567892366", sizeof(cfg.hw_mac));
    parStrncpy(cfg.hw_last_reboot_reason , "unknown", sizeof(cfg.hw_last_reboot_reason));
    parStrncpy(cfg.fw_name , "2.364s2", sizeof(cfg.fw_name));
    parStrncpy(cfg.webpa_path_url , "/v1", sizeof(cfg.webpa_path_url));
    parStrncpy(cfg.webpa_url , "localhost", sizeof(cfg.webpa_url));
    parStrncpy(cfg.webpa_interface_used , "eth0", sizeof(cfg.webpa_interface_used));
    parStrncpy(cfg.webpa_protocol , "WebPA-1.6", sizeof(cfg.webpa_protocol));
    parStrncpy(cfg.webpa_uuid , "1234567-345456546", sizeof(cfg.webpa_uuid));
    parStrncpy(cfg.partner_id , "comcast", sizeof(cfg.partner_id));
    parStrncpy(cfg.seshat_url, "ipc://tmp/seshat_service.url", sizeof(cfg.seshat_url));
    cfg.secureFlag = 1;
    cfg.boot_time = 423457;
    cfg.webpa_ping_timeout = 30;
    cfg.webpa_backoff_max = 255;
    
    set_parodus_cfg(&cfg);

    ParodusCfg *temp = get_parodus_cfg();

    assert_string_equal(cfg.hw_model, temp->hw_model);
    assert_string_equal(cfg.hw_serial_number, temp->hw_serial_number);
    assert_string_equal(cfg.hw_manufacturer, temp->hw_manufacturer);
    assert_string_equal(cfg.hw_mac, temp->hw_mac);
    assert_string_equal(cfg.hw_last_reboot_reason, temp->hw_last_reboot_reason);
    assert_string_equal(cfg.webpa_path_url, temp->webpa_path_url);
    assert_string_equal(cfg.webpa_url, temp->webpa_url);
    assert_string_equal(cfg.webpa_interface_used, temp->webpa_interface_used);
    assert_string_equal(cfg.webpa_protocol, temp->webpa_protocol);
    assert_string_equal(cfg.webpa_uuid, temp->webpa_uuid);
    assert_string_equal(cfg.partner_id, temp->partner_id);
    assert_string_equal(cfg.seshat_url, temp->seshat_url);


    assert_int_equal((int) cfg.secureFlag, (int) temp->secureFlag);
    assert_int_equal((int) cfg.boot_time, (int) temp->boot_time);
    assert_int_equal((int) cfg.webpa_ping_timeout, (int) temp->webpa_ping_timeout);
    assert_int_equal((int) cfg.webpa_backoff_max, (int) temp->webpa_backoff_max);
}

void test_getParodusConfig()
{
    ParodusCfg cfg;
    memset(&cfg,0,sizeof(cfg));

    parStrncpy(cfg.hw_model, "TG1682133",sizeof(cfg.hw_model));
    set_parodus_cfg(&cfg);

    ParodusCfg *temp = get_parodus_cfg();

    assert_string_equal(cfg.hw_model, temp->hw_model);
}

void test_parseCommandLine()
{
    int argc =K_argc;
    char * command[argc+1];
    int i = 0;

    command[i++] = "parodus";
    command[i++] = "--hw-model=TG1682";
    command[i++] = "--hw-serial-number=Fer23u948590";
    command[i++] = "--hw-manufacturer=ARRISGroup,Inc.";
    command[i++] = "--hw-mac=123567892366";
    command[i++] = "--hw-last-reboot-reason=unknown";
    command[i++] = "--fw-name=TG1682_DEV_master_2016000000sdy";
    command[i++] = "--webpa-ping-time=180";
    command[i++] = "--webpa-inteface-used=br0";
    command[i++] = "--webpa-url=localhost";
    command[i++] = "--webpa-backoff-max=0";
    command[i++] = "--boot-time=1234";
    command[i++] = "--parodus-local-url=tcp://127.0.0.1:6666";
    command[i++] = "--partner-id=cox";
    command[i++] = "--seshat-url=ipc://127.0.0.1:7777";
    command[i] = '\0';

    ParodusCfg parodusCfg;
    memset(&parodusCfg,0,sizeof(parodusCfg));

    parseCommandLine(argc,command,&parodusCfg);

    assert_string_equal( parodusCfg.hw_model, "TG1682");
    assert_string_equal( parodusCfg.hw_serial_number, "Fer23u948590");
    assert_string_equal( parodusCfg.hw_manufacturer, "ARRISGroup,Inc.");
    assert_string_equal( parodusCfg.hw_mac, "123567892366");	
    assert_string_equal( parodusCfg.hw_last_reboot_reason, "unknown");	
    assert_string_equal( parodusCfg.fw_name, "TG1682_DEV_master_2016000000sdy");	
    assert_int_equal( (int) parodusCfg.webpa_ping_timeout,180);	
    assert_string_equal( parodusCfg.webpa_interface_used, "br0");	
    assert_string_equal( parodusCfg.webpa_url, "localhost");
    assert_int_equal( (int) parodusCfg.webpa_backoff_max,0);
    assert_int_equal( (int) parodusCfg.boot_time,1234);
    assert_string_equal(  parodusCfg.local_url,"tcp://127.0.0.1:6666");
    assert_string_equal(  parodusCfg.partner_id,"cox");
    assert_string_equal(  parodusCfg.seshat_url, "ipc://127.0.0.1:7777");

}

void test_parseCommandLineNull()
{
    parseCommandLine(0,NULL,NULL);
}

void err_parseCommandLine()
{
    int argc =K_argc;
    char * command[20]={'\0'};

    command[0] = "parodus";
    command[1] = "--hw-model=TG1682";
    command[12] = "webpa";

    ParodusCfg parodusCfg;
    memset(&parodusCfg,0,sizeof(parodusCfg));

    parseCommandLine(argc,command,&parodusCfg);
    assert_string_equal( parodusCfg.hw_model, "");
    assert_string_equal( parodusCfg.hw_serial_number, "");
}

void test_loadParodusCfg()
{
    ParodusCfg  tmpcfg;
    ParodusCfg *Cfg;
    Cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));

    parStrncpy(Cfg->hw_model, "TG1682", sizeof(Cfg->hw_model));
    parStrncpy(Cfg->hw_serial_number, "Fer23u948590", sizeof(Cfg->hw_serial_number));
    parStrncpy(Cfg->hw_manufacturer , "ARRISGroup,Inc.", sizeof(Cfg->hw_manufacturer));
    parStrncpy(Cfg->hw_mac , "123567892366", sizeof(Cfg->hw_mac));
    parStrncpy(Cfg->hw_last_reboot_reason , "unknown", sizeof(Cfg->hw_last_reboot_reason));
    parStrncpy(Cfg->fw_name , "2.364s2", sizeof(Cfg->fw_name));
    parStrncpy(Cfg->webpa_path_url , "/v1", sizeof(Cfg->webpa_path_url));
    parStrncpy(Cfg->webpa_url , "localhost", sizeof(Cfg->webpa_url));
    parStrncpy(Cfg->webpa_interface_used , "eth0", sizeof(Cfg->webpa_interface_used));
    parStrncpy(Cfg->webpa_protocol , "WebPA-1.6", sizeof(Cfg->webpa_protocol));
    parStrncpy(Cfg->local_url , "tcp://10.0.0.1:6000", sizeof(Cfg->local_url));
    parStrncpy(Cfg->partner_id , "shaw", sizeof(Cfg->partner_id));

    memset(&tmpcfg,0,sizeof(ParodusCfg));
    loadParodusCfg(Cfg,&tmpcfg);

    assert_string_equal( tmpcfg.hw_model, "TG1682");
    assert_string_equal( tmpcfg.hw_serial_number, "Fer23u948590");
    assert_string_equal( tmpcfg.hw_manufacturer, "ARRISGroup,Inc.");
    assert_string_equal( tmpcfg.hw_mac, "123567892366");
    assert_string_equal( tmpcfg.local_url, "tcp://10.0.0.1:6000");
    assert_string_equal( tmpcfg.partner_id, "shaw");
    free(Cfg);
}

void test_loadParodusCfgNull()
{
    ParodusCfg *cfg = (ParodusCfg *) malloc(sizeof(ParodusCfg));
    memset(cfg,0,sizeof(ParodusCfg));

    ParodusCfg temp;
    memset(&temp, 0, sizeof(ParodusCfg));

    loadParodusCfg(cfg,&temp);

    assert_string_equal(temp.hw_model, "");
    assert_string_equal(temp.hw_serial_number, "");
    assert_string_equal(temp.hw_manufacturer, "");
    assert_int_equal( (int) temp.secureFlag,1);	
    assert_string_equal( temp.webpa_path_url, WEBPA_PATH_URL);	
    assert_string_equal( temp.webpa_uuid,"1234567-345456546");
    assert_string_equal( temp.local_url, PARODUS_UPSTREAM);

    free(cfg);
}

void err_loadParodusCfg()
{
    ParodusCfg cfg;
    loadParodusCfg(NULL,&cfg);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_getParodusConfig),
        cmocka_unit_test(test_setParodusConfig),
        cmocka_unit_test(test_loadParodusCfg),
        cmocka_unit_test(test_loadParodusCfgNull),
        cmocka_unit_test(err_loadParodusCfg),
        cmocka_unit_test(test_parseCommandLine),
        cmocka_unit_test(test_parseCommandLineNull),
        cmocka_unit_test(err_parseCommandLine),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
