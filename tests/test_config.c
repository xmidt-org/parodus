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

    strncpy(cfg.hw_model, "TG1682", strlen("TG1682")+1);
    strncpy(cfg.hw_serial_number, "Fer23u948590", strlen("Fer23u948590")+1);
    strncpy(cfg.hw_manufacturer , "ARRISGroup,Inc.", strlen("ARRISGroup,Inc.")+1);
    strncpy(cfg.hw_mac , "123567892366", strlen("123567892366")+1);
    strncpy(cfg.hw_last_reboot_reason , "unknown", strlen("unknown")+1);
    strncpy(cfg.fw_name , "2.364s2", strlen("2.364s2")+1);
    strncpy(cfg.webpa_path_url , "/v1", strlen("/v1")+1);
    strncpy(cfg.webpa_url , "localhost", strlen("localhost")+1);
    strncpy(cfg.webpa_interface_used , "eth0", strlen("eth0")+1);
    strncpy(cfg.webpa_protocol , "WebPA-1.6", strlen("WebPA-1.6")+1);
    strncpy(cfg.webpa_uuid , "1234567-345456546", strlen("1234567-345456546")+1);
    strncpy(cfg.partner_id , "comcast", strlen("comcast")+1);
    strncpy(cfg.seshat_url, "ipc://tmp/seshat_service.url", strlen("ipc://tmp/seshat_service.url")+1);
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

    strncpy(cfg.hw_model, "TG1682133", strlen("TG1682133")+1);
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
    char protocol[32] = {'\0'};

    strncpy(Cfg->hw_model, "TG1682", strlen("TG1682")+1);
    strncpy(Cfg->hw_serial_number, "Fer23u948590", strlen("Fer23u948590")+1);
    strncpy(Cfg->hw_manufacturer , "ARRISGroup,Inc.", strlen("ARRISGroup,Inc.")+1);
    strncpy(Cfg->hw_mac , "123567892366", strlen("123567892366")+1);
    strncpy(Cfg->hw_last_reboot_reason , "unknown", strlen("unknown")+1);
    strncpy(Cfg->fw_name , "2.364s2", strlen("2.364s2")+1);
    strncpy(Cfg->webpa_path_url , "/v1", strlen( "/v1")+1);
    strncpy(Cfg->webpa_url , "localhost", strlen("localhost")+1);
    strncpy(Cfg->webpa_interface_used , "eth0", strlen("eth0")+1);
    snprintf(protocol, sizeof(protocol), "%s-%s", PROTOCOL_VALUE, GIT_COMMIT_TAG);
    strncpy(Cfg->webpa_protocol , protocol, strlen(protocol)+1);
    strncpy(Cfg->local_url , "tcp://10.0.0.1:6000", strlen("tcp://10.0.0.1:6000")+1);
    strncpy(Cfg->partner_id , "shaw", strlen("shaw")+1);

    memset(&tmpcfg,0,sizeof(ParodusCfg));
    loadParodusCfg(Cfg,&tmpcfg);

    assert_string_equal( tmpcfg.hw_model, "TG1682");
    assert_string_equal( tmpcfg.hw_serial_number, "Fer23u948590");
    assert_string_equal( tmpcfg.hw_manufacturer, "ARRISGroup,Inc.");
    assert_string_equal( tmpcfg.hw_mac, "123567892366");
    assert_string_equal( tmpcfg.local_url, "tcp://10.0.0.1:6000");
    assert_string_equal( tmpcfg.partner_id, "shaw");
    assert_string_equal( tmpcfg.webpa_protocol, protocol);
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

void test_parodusGitVersion()
{
   FILE *fp;
   char version[32] = {'\0'};
   char *command = "git describe --tags --always";
   int n;
   size_t len;
   fp = popen(command,"r"); 
   while(fgets(version, 32, fp) !=NULL)
   {
   	len = strlen(version);
  	if (len > 0 && version[len-1] == '\n') 
  	{
    		version[--len] = '\0';
  	}
   }
   fclose(fp);
   
   n = strcmp( version, GIT_COMMIT_TAG);
   assert_int_equal(n, 0);
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
        cmocka_unit_test(test_parodusGitVersion)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
