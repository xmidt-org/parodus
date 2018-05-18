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

    cfg.hw_model = strdup ("TG1682");
    cfg.hw_serial_number = strdup ("Fer23u948590");
    cfg.hw_manufacturer = strdup ("ARRISGroup,Inc.");
    cfg.hw_mac = strdup ("123567892366");
    cfg.hw_last_reboot_reason = strdup ("unknown");
    cfg.fw_name = strdup ("2.364s2");
    cfg.webpa_path_url = strdup ("/v1");
    cfg.webpa_url = strdup ("http://127.0.0.1");
    cfg.webpa_interface_used = strdup ("eth0");
    cfg.webpa_protocol = strdup ("WebPA-1.6");
    cfg.webpa_uuid = strdup ("1234567-345456546");
    cfg.partner_id = strdup ("mycom");
#ifdef ENABLE_SESHAT
    cfg.seshat_url = strdup ("ipc://tmp/seshat_service.url");
#endif
    cfg.flags = 0;
    cfg.boot_time = 423457;
    cfg.webpa_ping_timeout = 30;
    cfg.webpa_backoff_max = 255;
#ifdef FEATURE_DNS_QUERY
    cfg.acquire_jwt = 1;
    cfg.dns_txt_url = strdup ("test");
    cfg.jwt_algo = 1025;
    cfg.jwt_key = strdup ("key.txt");
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
#ifdef FEATURE_DNS_QUERY
    assert_int_equal( (int) cfg.acquire_jwt, (int) temp->acquire_jwt);
    assert_string_equal(cfg.dns_txt_url, temp->dns_txt_url);
    assert_int_equal( (int) cfg.jwt_algo, (int) temp->jwt_algo);
    assert_string_equal(cfg.jwt_key, temp->jwt_key);
#endif
    
    clean_up_parodus_cfg(&cfg);
}

void test_getParodusConfig()
{
    ParodusCfg cfg;
    memset(&cfg,0,sizeof(cfg));

    cfg.hw_model = strdup ("TG1682133");
    set_parodus_cfg(&cfg);

    ParodusCfg *temp = get_parodus_cfg();

    assert_string_equal(cfg.hw_model, temp->hw_model);
    
    free(cfg.hw_model);
}

#ifdef FEATURE_DNS_QUERY
const char *jwt_key_file_path = JWT_KEY_FILE_PATH;
#else
const char *jwt_key_file_path = "foo";
#endif

static FILE * open_output_file (const char *fname)
{
  FILE *fd;
  char *file_path;
  int error;

  file_path = (char *) malloc(strlen(fname) + strlen(jwt_key_file_path) + 1);
  if (NULL == file_path) {
      ParodusError("open_output_file() malloc failed\n");
  }

  strcpy(file_path, jwt_key_file_path);
  strcat(file_path, fname);

  errno = 0;
  fd = fopen(file_path, "w+");
  error = errno;

  if (NULL == fd)
  {
    ParodusError ("File %s open error (%s)\n", file_path, strerror(error));
    abort ();
  }
  return fd;
} 

void write_key_to_file (const char *fname, const char *buf)
{
  ssize_t nbytes = -1;
  ssize_t buflen = strlen (buf);
  FILE *fd = open_output_file(fname);
  
  if (fd) {
    nbytes = fwrite(buf, sizeof(char), buflen, fd);
  }
  
  if (nbytes < 0)
  {
    ParodusError ("Write file %s error\n", fname);
    fclose(fd);
    abort ();
  }
  fclose(fd);
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
        "--jwt-algo=RS256",
        "--jwt-public-key-file=../../tests/jwt_key.tst", /* POSITION DEPENDENT */
#endif
		NULL
	};
	int argc = (sizeof (command) / sizeof (char *)) - 1;
    ParodusCfg parodusCfg;

#ifdef FEATURE_DNS_QUERY
    char *file_path;
    #define JWT_PUBLIC "--jwt-public-key-file="

    file_path = (char *) malloc(strlen(JWT_PUBLIC) + strlen("/jwt_key.tst") +
                         strlen(jwt_key_file_path) + 1);
    if (NULL == file_path) {
        ParodusError("open_output_file() malloc failed\n");
    }

    strcpy(file_path, JWT_PUBLIC);
    strcat(file_path, jwt_key_file_path);
    strcat(file_path, "/jwt_key.tst");
    command[argc-1] = file_path;
#endif

    memset(&parodusCfg,0,sizeof(parodusCfg));

#ifdef FEATURE_DNS_QUERY
	write_key_to_file ("/jwt_key.tst", jwt_key);
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
    free(file_path);
#endif

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

    Cfg->hw_model = strdup ("TG1682");
    Cfg->hw_serial_number = strdup ("Fer23u948590");
    Cfg->hw_manufacturer = strdup ("ARRISGroup,Inc.");
    Cfg->hw_mac = strdup ("123567892366");
    Cfg->hw_last_reboot_reason = strdup ("unknown");
    Cfg->fw_name = strdup ("2.364s2");
    Cfg->webpa_path_url = strdup ("/v1");
    Cfg->webpa_url = strdup ("http://127.0.0.1");
    Cfg->webpa_interface_used = strdup ("eth0");
    snprintf(protocol, sizeof(protocol), "%s-%s", PROTOCOL_VALUE, GIT_COMMIT_TAG);
    Cfg->webpa_protocol = strdup (protocol);
    Cfg->local_url = strdup ("tcp://10.0.0.1:6000");
    Cfg->partner_id = strdup ("shaw");
#ifdef FEATURE_DNS_QUERY
	Cfg->acquire_jwt = 1;
    Cfg->dns_txt_url = strdup ("mydns");
    Cfg->jwt_algo = 1025;
    Cfg->jwt_key = strdup ("AGdyuwyhwl2ow2ydsoioiygkshwdthuwd");
#endif
    Cfg->token_acquisition_script = strdup ("/tmp/token.sh");
    Cfg->token_read_script = strdup ("/tmp/token.sh");
    Cfg->cert_path = strdup ("/etc/ssl.crt");
#ifdef ENABLE_SESHAT
    Cfg->seshat_url = strdup ("ipc://tmp/seshat_service.url");
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
    clean_up_parodus_cfg(Cfg);    
    free(Cfg);
}

void test_loadParodusCfgNull()
{
    ParodusCfg *cfg = (ParodusCfg *) malloc(sizeof(ParodusCfg));
    memset(cfg,0,sizeof(ParodusCfg));

    ParodusCfg temp;
    memset(&temp, 0, sizeof(ParodusCfg));

    loadParodusCfg(cfg,&temp);

    assert_int_equal( (int) temp.flags,0);
    assert_string_equal( temp.webpa_path_url, WEBPA_PATH_URL);	
    assert_string_equal( temp.webpa_uuid,"1234567-345456546");
    assert_string_equal( temp.local_url, PARODUS_UPSTREAM);

    clean_up_parodus_cfg(cfg);    
    free(cfg);
}

void err_loadParodusCfg()
{
    ParodusCfg cfg;
    loadParodusCfg(NULL,&cfg);
}

/* This test makes no sense ;-) 
void test_parodusGitVersion()
{
   FILE *fp;
   char *version = (char *) malloc(256);
   char *command = "git describe --tags --always";
   int n;
   size_t len;
   fp = popen(command,"r"); 
   memset(version, 0, 2048);
   while(fgets(version, 2048, fp) !=NULL)
   {
   	len = strlen(version);
  	if (len > 0) 
  	{
    		version[--len] = '\0';
  	}
   }
   pclose(fp);
   
   printf ("version: %s\n", version);
   printf ("GIT_COMMIT_TAG: %s\n", GIT_COMMIT_TAG);
   n = strcmp( version, GIT_COMMIT_TAG);
   assert_int_equal(n, 0);
   free(version);
}
*/

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
    assert_true(cfg->cert_path == NULL);
    assert_int_equal((int)cfg->flags, 0);
    assert_string_equal(cfg->webpa_path_url, WEBPA_PATH_URL);
    assert_string_equal(cfg->webpa_uuid, "1234567-345456546");
    
    clean_up_parodus_cfg(cfg);    
    free(cfg);
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
       // cmocka_unit_test(test_parodusGitVersion),
        cmocka_unit_test(test_setDefaultValuesToCfg),
        cmocka_unit_test(err_setDefaultValuesToCfg),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
