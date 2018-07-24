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

extern int parse_mac_address (char *target, const char *arg);
extern int server_is_http (const char *full_url,
	const char **server_ptr);
extern int parse_webpa_url(const char *full_url, 
	char *server_addr, int server_addr_buflen,
	char *port_buf, int port_buflen);
extern unsigned int get_algo_mask (const char *algo_str);
extern unsigned int parse_num_arg (const char *arg, const char *arg_name);

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
    parStrncpy(cfg.webpa_url , "http://127.0.0.1", sizeof(cfg.webpa_url));
    parStrncpy(cfg.webpa_interface_used , "eth0", sizeof(cfg.webpa_interface_used));
    parStrncpy(cfg.webpa_protocol , "WebPA-1.6", sizeof(cfg.webpa_protocol));
    parStrncpy(cfg.webpa_uuid , "1234567-345456546", sizeof(cfg.webpa_uuid));
    parStrncpy(cfg.partner_id , "mycom", sizeof(cfg.partner_id));
#ifdef ENABLE_SESHAT
    parStrncpy(cfg.seshat_url, "ipc://tmp/seshat_service.url", sizeof(cfg.seshat_url));
#endif
    cfg.flags = 0;
    cfg.boot_time = 423457;
    cfg.webpa_ping_timeout = 30;
    cfg.webpa_backoff_max = 255;
#ifdef FEATURE_DNS_QUERY
    cfg.acquire_jwt = 1;
    parStrncpy(cfg.dns_txt_url, "test",sizeof(cfg.dns_txt_url));
    cfg.jwt_algo = 1025;
    parStrncpy(cfg.jwt_key, "key.txt",sizeof(cfg.jwt_key));
#endif 
	cfg.crud_config_file = strdup("parodus_cfg.json");
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
#ifdef FEATURE_DNS_QUERY
    assert_int_equal( (int) cfg.acquire_jwt, (int) temp->acquire_jwt);
    assert_string_equal(cfg.dns_txt_url, temp->dns_txt_url);
    assert_int_equal( (int) cfg.jwt_algo, (int) temp->jwt_algo);
    assert_string_equal(cfg.jwt_key, temp->jwt_key);
#endif
	assert_string_equal(cfg.crud_config_file, temp->crud_config_file);
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

static int open_output_file (const char *fname)
{
  int fd = open(fname, O_WRONLY | O_CREAT, 0666);
  if (fd<0)
  {
    ParodusError ("File %s open error\n", fname);
    abort ();
  }
  return fd;
} 

void write_key_to_file (const char *fname, const char *buf)
{
  ssize_t nbytes;
  ssize_t buflen = strlen (buf);
  int fd = open_output_file(fname);
  nbytes = write(fd, buf, buflen);
  if (nbytes < 0)
  {
    ParodusError ("Write file %s error\n", fname);
    close(fd);
    abort ();
  }
  close(fd);
  ParodusInfo ("%d bytes written\n", nbytes);
}

void test_parseCommandLine()
{
    char expectedToken[1280] = {'\0'};
#ifdef FEATURE_DNS_QUERY    
	const char *jwt_key =	"AGdyuwyhwl2ow2ydsoioiygkshwdthuwd";
#endif

    char *command[] = {"parodus",
		"--hw-model=TG1682",
		"--hw-serial-number=Fer23u948590",
		"--hw-manufacturer=ARRISGroup,Inc.",
		"--hw-mac=123567892366",
		"--hw-last-reboot-reason=unknown",
		"--fw-name=TG1682_DEV_master_2016000000sdy",
		"--webpa-ping-timeout=180",
		"--webpa-interface-used=br0",
		"--webpa-url=http://127.0.0.1",
		"--webpa-backoff-max=0",
		"--boot-time=1234",
		"--parodus-local-url=tcp://127.0.0.1:6666",
		"--partner-id=cox",
#ifdef ENABLE_SESHAT
		"--seshat-url=ipc://127.0.0.1:7777",
#endif
		"--force-ipv4",
		"--force-ipv6",
		"--token-read-script=/tmp/token.sh",
		"--token-acquisition-script=/tmp/token.sh",
		"--ssl-cert-path=/etc/ssl/certs/ca-certificates.crt",
#ifdef FEATURE_DNS_QUERY
		"--acquire-jwt=1",
		"--dns-txt-url=mydns.mycom.net",
		"--jwt-public-key-file=../../tests/jwt_key.tst",
		"--jwt-algo=RS256",
#endif
		"--crud-config-file=parodus_cfg.json",
		NULL
	};
	int argc = (sizeof (command) / sizeof (char *)) - 1;

    ParodusCfg parodusCfg;
    memset(&parodusCfg,0,sizeof(parodusCfg));

#ifdef FEATURE_DNS_QUERY
	write_key_to_file ("../../tests/jwt_key.tst", jwt_key);
#endif
    create_token_script("/tmp/token.sh");
    assert_int_equal (parseCommandLine(argc,command,&parodusCfg), 0);

    assert_string_equal( parodusCfg.hw_model, "TG1682");
    assert_string_equal( parodusCfg.hw_serial_number, "Fer23u948590");
    assert_string_equal( parodusCfg.hw_manufacturer, "ARRISGroup,Inc.");
    assert_string_equal( parodusCfg.hw_mac, "123567892366");	
    assert_string_equal( parodusCfg.hw_last_reboot_reason, "unknown");	
    assert_string_equal( parodusCfg.fw_name, "TG1682_DEV_master_2016000000sdy");	
    assert_int_equal( (int) parodusCfg.webpa_ping_timeout,180);	
    assert_string_equal( parodusCfg.webpa_interface_used, "br0");	
    assert_string_equal( parodusCfg.webpa_url, "http://127.0.0.1");
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
#ifdef FEATURE_DNS_QUERY
	assert_int_equal( (int) parodusCfg.acquire_jwt, 1);
    assert_string_equal(parodusCfg.dns_txt_url, "mydns.mycom.net");
    assert_int_equal( (int) parodusCfg.jwt_algo, 1024);
	assert_string_equal ( get_parodus_cfg()->jwt_key, jwt_key);
#endif
	assert_string_equal(parodusCfg.crud_config_file, "parodus_cfg.json");
}

void test_parseCommandLineNull()
{
    assert_int_equal (parseCommandLine(0,NULL,NULL), -1);
}

void err_parseCommandLine()
{
	int argc;
    char *command[] = {"parodus",
		"--hw-model=TG1682",
		"--hw-serial-number=Fer23u948590",
		"-Z",
		"--nosuch",
		"--hw-mac=123567892366",
		"webpa",
		NULL
	};
    ParodusCfg parodusCfg;

    memset(&parodusCfg,0,sizeof(parodusCfg));

	argc = (sizeof (command) / sizeof (char *)) - 1;
	// Missing webpa_url
    assert_int_equal (parseCommandLine(argc,command,&parodusCfg), -1);
	// Bad webpa_url
	command[5] = "--webpa-url=127.0.0.1";
    assert_int_equal (parseCommandLine(argc,command,&parodusCfg), -1);
	// Bad mac address
	command[5] = "--hw-mac=1235678923";
    assert_int_equal (parseCommandLine(argc,command,&parodusCfg), -1);
	command[5] = "--webpa-ping-timeout=123x";
    assert_int_equal (parseCommandLine(argc,command,&parodusCfg), -1);
	command[5] = "--webpa-backoff-max=";
    assert_int_equal (parseCommandLine(argc,command,&parodusCfg), -1);
	command[5] = "--boot-time=12x";
    assert_int_equal (parseCommandLine(argc,command,&parodusCfg), -1);
#ifdef FEATURE_DNS_QUERY
	command[5] = "--webpa-url=https://127.0.0.1";
	command[3] = "--acquire-jwt=1";
	command[4] = "--dns-txt-url=mydns.mycom.net";
	// missing algo
    assert_int_equal (parseCommandLine(argc,command,&parodusCfg), -1);
	command[4] = "--jwt-algo=none:RS256";
	// disallowed alogrithm none
    assert_int_equal (parseCommandLine(argc,command,&parodusCfg), -1);
	command[4] = "--jwt-algo=RS256";
	// missing jwt public key file
    assert_int_equal (parseCommandLine(argc,command,&parodusCfg), -1);
	
#endif    
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
    parStrncpy(Cfg->webpa_url , "http://127.0.0.1", sizeof(Cfg->webpa_url));
    parStrncpy(Cfg->webpa_interface_used , "eth0", sizeof(Cfg->webpa_interface_used));
    snprintf(protocol, sizeof(protocol), "%s-%s", PROTOCOL_VALUE, GIT_COMMIT_TAG);
    parStrncpy(Cfg->webpa_protocol , protocol, sizeof(Cfg->webpa_protocol));
    parStrncpy(Cfg->local_url , "tcp://10.0.0.1:6000", sizeof(Cfg->local_url));
    parStrncpy(Cfg->partner_id , "shaw", sizeof(Cfg->partner_id));
#ifdef FEATURE_DNS_QUERY
	Cfg->acquire_jwt = 1;
    parStrncpy(Cfg->dns_txt_url, "mydns",sizeof(Cfg->dns_txt_url));
    Cfg->jwt_algo = 1025;
    parStrncpy(Cfg->jwt_key, "AGdyuwyhwl2ow2ydsoioiygkshwdthuwd",sizeof(Cfg->jwt_key));
#endif
    parStrncpy(Cfg->token_acquisition_script , "/tmp/token.sh", sizeof(Cfg->token_acquisition_script));
    parStrncpy(Cfg->token_read_script , "/tmp/token.sh", sizeof(Cfg->token_read_script));
    parStrncpy(Cfg->cert_path, "/etc/ssl.crt",sizeof(Cfg->cert_path));
#ifdef ENABLE_SESHAT
    parStrncpy(Cfg->seshat_url, "ipc://tmp/seshat_service.url", sizeof(Cfg->seshat_url));
#endif
	Cfg->crud_config_file = strdup("parodus_cfg.json");
    memset(&tmpcfg,0,sizeof(ParodusCfg));
    loadParodusCfg(Cfg,&tmpcfg);

    assert_string_equal( tmpcfg.hw_model, "TG1682");
    assert_string_equal( tmpcfg.hw_serial_number, "Fer23u948590");
    assert_string_equal( tmpcfg.hw_manufacturer, "ARRISGroup,Inc.");
    assert_string_equal( tmpcfg.hw_mac, "123567892366");
    assert_string_equal( tmpcfg.local_url, "tcp://10.0.0.1:6000");
    assert_string_equal( tmpcfg.partner_id, "shaw");
    assert_string_equal( tmpcfg.webpa_protocol, protocol);
#ifdef FEATURE_DNS_QUERY
	assert_int_equal( (int) tmpcfg.acquire_jwt, 1);
    assert_string_equal(tmpcfg.dns_txt_url, "mydns");
    assert_int_equal( (int) tmpcfg.jwt_algo, 1025);
    assert_string_equal(tmpcfg.jwt_key, "AGdyuwyhwl2ow2ydsoioiygkshwdthuwd");
#endif
    assert_string_equal(  tmpcfg.token_acquisition_script,"/tmp/token.sh");
    assert_string_equal(  tmpcfg.token_read_script,"/tmp/token.sh");
    assert_string_equal(tmpcfg.cert_path, "/etc/ssl.crt");
#ifdef ENABLE_SESHAT
    assert_string_equal(tmpcfg.seshat_url, "ipc://tmp/seshat_service.url");
#endif
	assert_string_equal(tmpcfg.crud_config_file, "parodus_cfg.json");
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
    assert_int_equal( (int) temp.flags,0);
    assert_string_equal( temp.webpa_path_url, WEBPA_PATH_URL);	
    assert_string_equal( temp.webpa_uuid,"1234567-345456546");
    assert_string_equal( temp.local_url, PARODUS_UPSTREAM);
	assert_null(temp.crud_config_file);
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
   
   printf ("version: %s\n", version);
   printf ("GIT_COMMIT_TAG: %s\n", GIT_COMMIT_TAG);
   n = strcmp( version, GIT_COMMIT_TAG);
   assert_int_equal(n, 0);
}

void test_setDefaultValuesToCfg()
{
    ParodusCfg *cfg = (ParodusCfg *) malloc(sizeof(ParodusCfg));
    memset(cfg,0,sizeof(ParodusCfg));
    setDefaultValuesToCfg(cfg);
    assert_string_equal( cfg->local_url, PARODUS_UPSTREAM);
#ifdef FEATURE_DNS_QUERY
	assert_int_equal(cfg->acquire_jwt, 0);
    assert_string_equal(cfg->dns_txt_url, DNS_TXT_URL);
    assert_string_equal(cfg->jwt_key, "\0");
    assert_int_equal( (int)cfg->jwt_algo, 0);
#endif
    assert_string_equal(cfg->cert_path, "\0");
    assert_int_equal((int)cfg->flags, 0);
    assert_string_equal(cfg->webpa_path_url, WEBPA_PATH_URL);
    assert_string_equal(cfg->webpa_uuid, "1234567-345456546");
    assert_string_equal(cfg->cloud_status, "offline");
}

void err_setDefaultValuesToCfg()
{
    setDefaultValuesToCfg(NULL);
}

void test_parse_num_arg ()
{
	assert_int_equal (parse_num_arg ("1234", "1234"), 1234);
	assert_int_equal (parse_num_arg ("1", "1"), 1);
	assert_int_equal (parse_num_arg ("0", "0"), 0);
	assert_true (parse_num_arg ("", "empty arg") == (unsigned int) -1);
	assert_true (parse_num_arg ("0x", "non-num arg") == (unsigned int) -1);
	
}

void test_parse_mac_address ()
{
	char result[14];
	assert_int_equal (parse_mac_address (result, "aabbccddeeff"), 0);
	assert_string_equal (result, "aabbccddeeff");
	assert_int_equal (parse_mac_address (result, "aa:bb:cc:dd:ee:ff"), 0);
	assert_string_equal (result, "aabbccddeeff");
	assert_int_equal (parse_mac_address (result, "aabbccddeeff0"), -1);
	assert_int_equal (parse_mac_address (result, "aa:bb:c:dd:ee:ff:00"), -1);
	assert_int_equal (parse_mac_address (result, ""), -1);
}

void test_server_is_http ()
{
	const char *server_ptr;
	assert_int_equal (server_is_http ("https://127.0.0.1", &server_ptr), 0);
	assert_string_equal (server_ptr, "127.0.0.1");
	assert_int_equal (server_is_http ("http://127.0.0.1", &server_ptr), 1);
	assert_string_equal (server_ptr, "127.0.0.1");
	assert_int_equal (server_is_http ("127.0.0.1", &server_ptr), -1);
	
}

void test_parse_webpa_url ()
{
	char addr_buf[80];
	char port_buf[8];
	assert_int_equal (parse_webpa_url ("mydns.mycom.net:8080",
		addr_buf, 80, port_buf, 8), -1);
	assert_int_equal (parse_webpa_url ("https://mydns.mycom.net:8080",
		addr_buf, 80, port_buf, 8), 0);
	assert_string_equal (addr_buf, "mydns.mycom.net");
	assert_string_equal (port_buf, "8080");
	assert_int_equal (parse_webpa_url ("https://mydns.mycom.net/",
		addr_buf, 80, port_buf, 8), 0);
	assert_string_equal (addr_buf, "mydns.mycom.net");
	assert_string_equal (port_buf, "443");
	assert_int_equal (parse_webpa_url ("https://mydns.mycom.net/api/v2/",
		addr_buf, 80, port_buf, 8), 0);
	assert_string_equal (addr_buf, "mydns.mycom.net");
	assert_string_equal (port_buf, "443");
	assert_int_equal (parse_webpa_url ("http://mydns.mycom.net:8080",
		addr_buf, 80, port_buf, 8), 1);
	assert_string_equal (addr_buf, "mydns.mycom.net");
	assert_string_equal (port_buf, "8080");
	assert_int_equal (parse_webpa_url ("http://mydns.mycom.net",
		addr_buf, 80, port_buf, 8), 1);
	assert_string_equal (addr_buf, "mydns.mycom.net");
	assert_string_equal (port_buf, "80");
    assert_int_equal (parse_webpa_url ("https://[2001:558:fc18:2:f816:3eff:fe7f:6efa]:8080",
		addr_buf, 80, port_buf, 8), 0);
	assert_string_equal (addr_buf, "2001:558:fc18:2:f816:3eff:fe7f:6efa");
	assert_string_equal (port_buf, "8080");
    assert_int_equal (parse_webpa_url ("https://[2001:558:fc18:2:f816:3eff:fe7f:6efa]",
		addr_buf, 80, port_buf, 8), 0);
	assert_string_equal (addr_buf, "2001:558:fc18:2:f816:3eff:fe7f:6efa");
	assert_string_equal (port_buf, "443");
    assert_int_equal (parse_webpa_url ("http://[2001:558:fc18:2:f816:3eff:fe7f:6efa]:8080",
		addr_buf, 80, port_buf, 8), 1);
	assert_string_equal (addr_buf, "2001:558:fc18:2:f816:3eff:fe7f:6efa");
	assert_string_equal (port_buf, "8080");
    assert_int_equal (parse_webpa_url ("http://[2001:558:fc18:2:f816:3eff:fe7f:6efa]",
		addr_buf, 80, port_buf, 8), 1);
	assert_string_equal (addr_buf, "2001:558:fc18:2:f816:3eff:fe7f:6efa");
	assert_string_equal (port_buf, "80");
    assert_int_equal (parse_webpa_url ("http://2001:558:fc18:2:f816:3eff:fe7f:6efa]",
		addr_buf, 80, port_buf, 8), -1);
    assert_int_equal (parse_webpa_url ("http://[2001:558:fc18:2:f816:3eff:fe7f:6efa",
		addr_buf, 80, port_buf, 8), -1);
    assert_int_equal (parse_webpa_url ("[2001:558:fc18:2:f816:3eff:fe7f:6efa",
		addr_buf, 80, port_buf, 8), -1);

		
}

void test_get_algo_mask ()
{
	assert_true (get_algo_mask ("RS256:RS512") == 5120);
	assert_true (get_algo_mask ("none:RS256") == (unsigned int) -1);
	assert_true (get_algo_mask ("nosuch") == (unsigned int) -1);
#if ALLOW_NON_RSA_ALG
	assert_true (get_algo_mask ("ES256:RS256") == 1026);
#else
	assert_true (get_algo_mask ("ES256:RS256") == (unsigned int) -1);
#endif	
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
        cmocka_unit_test(test_parse_num_arg),
        cmocka_unit_test(test_parse_mac_address),
        cmocka_unit_test(test_get_algo_mask),
        cmocka_unit_test(test_server_is_http),
        cmocka_unit_test(test_parse_webpa_url),
        cmocka_unit_test(test_parseCommandLine),
        cmocka_unit_test(test_parseCommandLineNull),
        cmocka_unit_test(err_parseCommandLine),
        cmocka_unit_test(test_parodusGitVersion),
        cmocka_unit_test(test_setDefaultValuesToCfg),
        cmocka_unit_test(err_setDefaultValuesToCfg),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
