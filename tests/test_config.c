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
#define K_argc 21

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
void create_token_script(char *fname)
{
    char command[128] = {'\0'};
    FILE *fp = fopen(fname, "w");
    assert_non_null(fp);
    fprintf(fp, "%s", "printf secure-token-$1-$2");
    fclose(fp);
    sprintf(command, "chmod +x %s",fname);
    system(command);
}
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
#ifdef ENABLE_SESHAT
    parStrncpy(cfg.seshat_url, "ipc://tmp/seshat_service.url", sizeof(cfg.seshat_url));
#endif
    cfg.flags = FLAGS_SECURE;
    cfg.boot_time = 423457;
    cfg.webpa_ping_timeout = 30;
    cfg.webpa_backoff_max = 255;
#ifdef ENABLE_CJWT
    parStrncpy(cfg.dns_id, "test",sizeof(cfg.dns_id));
    parStrncpy(cfg.jwt_algo, "none", sizeof(cfg.jwt_algo));
    parStrncpy(cfg.jwt_key, "key.txt",sizeof(cfg.jwt_key));
#endif
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
#ifdef ENABLE_SESHAT
    assert_string_equal(cfg.seshat_url, temp->seshat_url);
#endif
    assert_int_equal((int) cfg.flags, (int) temp->flags);
    assert_int_equal((int) cfg.boot_time, (int) temp->boot_time);
    assert_int_equal((int) cfg.webpa_ping_timeout, (int) temp->webpa_ping_timeout);
    assert_int_equal((int) cfg.webpa_backoff_max, (int) temp->webpa_backoff_max);
#ifdef ENABLE_CJWT
    assert_string_equal(cfg.dns_id, "test");
    assert_string_equal(cfg.jwt_algo, "none");
    assert_string_equal(cfg.jwt_key, "key.txt");
#endif
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
//If new arguments are added, update argc accordingly.
    int argc =K_argc+3;
#ifndef ENABLE_SESHAT
    argc = argc - 1;
#endif

#ifdef ENABLE_CJWT
    argc = argc+4;
#endif
    char * command[argc+1];
    int i = 0;
    char expectedToken[128] = {'\0'};

    command[i++] = "parodus";
    command[i++] = "--hw-model=TG1682";
    command[i++] = "--hw-serial-number=Fer23u948590";
    command[i++] = "--hw-manufacturer=ARRISGroup,Inc.";
    command[i++] = "--hw-mac=123567892366";
    command[i++] = "--hw-last-reboot-reason=unknown";
    command[i++] = "--fw-name=TG1682_DEV_master_2016000000sdy";
    command[i++] = "--webpa-ping-time=180";
    command[i++] = "--webpa-interface-used=br0";
    command[i++] = "--webpa-url=localhost";
    command[i++] = "--webpa-backoff-max=0";
    command[i++] = "--boot-time=1234";
    command[i++] = "--parodus-local-url=tcp://127.0.0.1:6666";
    command[i++] = "--partner-id=cox";
#ifdef ENABLE_SESHAT
    command[i++] = "--seshat-url=ipc://127.0.0.1:7777";
#endif
    command[i++] = "--force-ipv4";
    command[i++] = "--force-ipv6";
    command[i++] = "--token-read-script=/tmp/token.sh";
    command[i++] = "--token-acquisition-script=/tmp/token.sh";
    command[i++] = "--ssl-cert-path=/etc/ssl/certs/ca-certificates.crt";
    command[i++] = "--secure-flag=http";
    command[i++] = "--secure-flag=";
    command[i++] = "--secure-flag=https";
    command[i++] = "--port=9000";
#ifdef ENABLE_CJWT
    command[i++] = "--dns-id=fabric";
    command[i++] = "--jwt-key=../../tests/webpa-rs256.pem";
    command[i++] = "--jwt-key=AGdyuwyhwl2ow2ydsoioiygkshwdthuwd";
    command[i++] = "--jwt-algo=none:RS256";
#endif
    command[i] = '\0';

    ParodusCfg parodusCfg;
    memset(&parodusCfg,0,sizeof(parodusCfg));
    create_token_script("/tmp/token.sh");
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
#ifdef ENABLE_SESHAT
    assert_string_equal(  parodusCfg.seshat_url, "ipc://127.0.0.1:7777");
#endif
    assert_int_equal( (int) parodusCfg.flags, FLAGS_IPV6_ONLY|FLAGS_IPV4_ONLY);
    sprintf(expectedToken,"secure-token-%s-%s",parodusCfg.hw_serial_number,parodusCfg.hw_mac);
    getAuthToken(&parodusCfg);
    set_parodus_cfg(&parodusCfg);
    
    assert_string_equal(  get_parodus_cfg()->webpa_auth_token,expectedToken);
    assert_string_equal(  parodusCfg.cert_path,"/etc/ssl/certs/ca-certificates.crt");
    assert_int_equal( (int) parodusCfg.secure_flag,FLAGS_SECURE);
    assert_int_equal( (int) parodusCfg.port,9000);
#ifdef ENABLE_CJWT
    assert_string_equal(parodusCfg.dns_id, "fabric");
    assert_string_equal(parodusCfg.jwt_algo, "none:RS256");
    assert_string_equal(parodusCfg.jwt_key, "AGdyuwyhwl2ow2ydsoioiygkshwdthuwd");
#endif
}

void test_parseCommandLineNull()
{
    parseCommandLine(0,NULL,NULL);
}

void err_parseCommandLine()
{
    int argc =K_argc+3;
#ifndef ENABLE_SESHAT
    argc = argc - 1;
#endif

#ifdef ENABLE_CJWT
    argc = argc+4;
#endif
    char * command[argc+1];

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
    ParodusCfg *Cfg = NULL;
    Cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    char protocol[32] = {'\0'};

    parStrncpy(Cfg->hw_model, "TG1682", sizeof(Cfg->hw_model));
    parStrncpy(Cfg->hw_serial_number, "Fer23u948590", sizeof(Cfg->hw_serial_number));
    parStrncpy(Cfg->hw_manufacturer , "ARRISGroup,Inc.", sizeof(Cfg->hw_manufacturer));
    parStrncpy(Cfg->hw_mac , "123567892366", sizeof(Cfg->hw_mac));
    parStrncpy(Cfg->hw_last_reboot_reason , "unknown", sizeof(Cfg->hw_last_reboot_reason));
    parStrncpy(Cfg->fw_name , "2.364s2", sizeof(Cfg->fw_name));
    parStrncpy(Cfg->webpa_path_url , "/v1", sizeof(Cfg->webpa_path_url));
    parStrncpy(Cfg->webpa_url , "localhost", sizeof(Cfg->webpa_url));
    parStrncpy(Cfg->webpa_interface_used , "eth0", sizeof(Cfg->webpa_interface_used));
    snprintf(protocol, sizeof(protocol), "%s-%s", PROTOCOL_VALUE, GIT_COMMIT_TAG);
    parStrncpy(Cfg->webpa_protocol , protocol, sizeof(Cfg->webpa_protocol));
    parStrncpy(Cfg->local_url , "tcp://10.0.0.1:6000", sizeof(Cfg->local_url));
    parStrncpy(Cfg->partner_id , "shaw", sizeof(Cfg->partner_id));
#ifdef ENABLE_CJWT
    parStrncpy(Cfg->dns_id, "fabric",sizeof(Cfg->dns_id));
    parStrncpy(Cfg->jwt_algo, "none:RS256", sizeof(Cfg->jwt_algo));
    parStrncpy(Cfg->jwt_key, "AGdyuwyhwl2ow2ydsoioiygkshwdthuwd",sizeof(Cfg->jwt_key));
#endif
    parStrncpy(Cfg->token_acquisition_script , "/tmp/token.sh", sizeof(Cfg->token_acquisition_script));
    parStrncpy(Cfg->token_read_script , "/tmp/token.sh", sizeof(Cfg->token_read_script));
    parStrncpy(Cfg->cert_path, "/etc/ssl.crt",sizeof(Cfg->cert_path));
#ifdef ENABLE_SESHAT
    parStrncpy(Cfg->seshat_url, "ipc://tmp/seshat_service.url", sizeof(Cfg->seshat_url));
#endif
    memset(&tmpcfg,0,sizeof(ParodusCfg));
    loadParodusCfg(Cfg,&tmpcfg);

    assert_string_equal( tmpcfg.hw_model, "TG1682");
    assert_string_equal( tmpcfg.hw_serial_number, "Fer23u948590");
    assert_string_equal( tmpcfg.hw_manufacturer, "ARRISGroup,Inc.");
    assert_string_equal( tmpcfg.hw_mac, "123567892366");
    assert_string_equal( tmpcfg.local_url, "tcp://10.0.0.1:6000");
    assert_string_equal( tmpcfg.partner_id, "shaw");
    assert_string_equal( tmpcfg.webpa_protocol, protocol);
#ifdef ENABLE_CJWT
    assert_string_equal(tmpcfg.dns_id, "fabric");
    assert_string_equal(tmpcfg.jwt_algo, "none:RS256");
    assert_string_equal(tmpcfg.jwt_key, "AGdyuwyhwl2ow2ydsoioiygkshwdthuwd");
#endif
    assert_string_equal(  tmpcfg.token_acquisition_script,"/tmp/token.sh");
    assert_string_equal(  tmpcfg.token_read_script,"/tmp/token.sh");
    assert_string_equal(tmpcfg.cert_path, "/etc/ssl.crt");
#ifdef ENABLE_SESHAT
    assert_string_equal(tmpcfg.seshat_url, "ipc://tmp/seshat_service.url");
#endif
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
    assert_int_equal( (int) temp.flags,FLAGS_SECURE);
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
   pclose(fp);
   
   n = strcmp( version, GIT_COMMIT_TAG);
   assert_int_equal(n, 0);
}

void test_setDefaultValuesToCfg()
{
    ParodusCfg *cfg = (ParodusCfg *) malloc(sizeof(ParodusCfg));
    memset(cfg,0,sizeof(ParodusCfg));
    setDefaultValuesToCfg(cfg);
    assert_string_equal( cfg->local_url, PARODUS_UPSTREAM);
#ifdef ENABLE_CJWT
    assert_string_equal(cfg->dns_id, DNS_ID);
    assert_string_equal(cfg->jwt_key, "\0");
    assert_string_equal(cfg->jwt_algo, "\0");
#endif
    assert_string_equal(cfg->cert_path, "\0");
    assert_int_equal((int)cfg->flags, FLAGS_SECURE);
    assert_int_equal((int)cfg->secure_flag, FLAGS_SECURE);
    assert_int_equal((int)cfg->port,8080);
    assert_string_equal(cfg->webpa_path_url, WEBPA_PATH_URL);
    assert_string_equal(cfg->webpa_uuid, "1234567-345456546");
}

void err_setDefaultValuesToCfg()
{
    setDefaultValuesToCfg(NULL);
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
        cmocka_unit_test(test_parodusGitVersion),
        cmocka_unit_test(test_setDefaultValuesToCfg),
        cmocka_unit_test(err_setDefaultValuesToCfg),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
