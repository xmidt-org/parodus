/**
 * Copyright 2015 Comcast Cable Communications Management, LLC
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
/**
 * @file config.c
 *
 * @description This file contains configuration details of parodus
 *
 */

#include <stdio.h>
#include <fcntl.h> 
#include "config.h"
#include "ParodusInternal.h"
#include <cjwt/cjwt.h>

#define MAX_BUF_SIZE	128

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

static ParodusCfg parodusCfg;
static unsigned int rsa_algorithms = 
	(1<<alg_rs256) | (1<<alg_rs384) | (1<<alg_rs512);

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

ParodusCfg *get_parodus_cfg(void) 
{
    return &parodusCfg;
}

void set_parodus_cfg(ParodusCfg *cfg) 
{
    memcpy(&parodusCfg, cfg, sizeof(ParodusCfg));
}

void set_cloud_disconnect_reason(ParodusCfg *cfg, char *disconn_reason)
{
    cfg->cloud_disconnect = strdup(disconn_reason);
}

void reset_cloud_disconnect_reason(ParodusCfg *cfg)
{
	cfg->cloud_disconnect = NULL;
}


const char *get_tok (const char *src, int delim, char *result, int resultsize)
{
	int i;
	char c;
	int endx = resultsize-1;

	memset (result, 0, resultsize);
	for (i=0; (c=src[i]) != 0; i++) {
		if (c == delim)
			break;
 		if (i < endx)
			result[i] = c;
	}
	if (c == 0)
		return NULL;
	return src + i + 1;
}

// the algorithm mask indicates which algorithms are allowed
unsigned int get_algo_mask (const char *algo_str)
{
  unsigned int mask = 0;
  unsigned int mask_val;
#define BUFLEN 16
  char tok[BUFLEN];
	int alg_val;

	while(NULL != algo_str)
	{
		algo_str = get_tok (algo_str, ':', tok, BUFLEN);
		alg_val = cjwt_alg_str_to_enum (tok);
		if ((alg_val < 0)  || (alg_val >= num_algorithms)) {
       ParodusError("Invalid jwt algorithm %s\n", tok);
       return (unsigned int) (-1);
		}
		if (alg_val == alg_none) {
       ParodusError("Disallowed jwt algorithm none\n");
       return (unsigned int) (-1);
		}
		mask_val = (1<<alg_val);
#if !ALLOW_NON_RSA_ALG
		if (0 == (mask_val & rsa_algorithms)) {
       ParodusError("Disallowed non-rsa jwt algorithm %s\n", tok);
       return (unsigned int) (-1);
		}
#endif
		mask |= mask_val;
		
	}
	return mask;
#undef BUFLEN
}

static int open_input_file (const char *fname)
{
  int fd = open(fname, O_RDONLY);
  if (fd<0)
  {
    ParodusError ("File %s open error\n", fname);
    abort ();
  }
  return fd;
} 

void read_key_from_file (const char *fname, char *buf, size_t buflen)
{
  ssize_t nbytes;
  int fd = open_input_file(fname);
  nbytes = read(fd, buf, buflen);
  if (nbytes < 0)
  {
    ParodusError ("Read file %s error\n", fname);
    close(fd);
    abort ();
  }
  close(fd);
  ParodusInfo ("%d bytes read\n", nbytes);
}

void execute_token_script(char *token, char *name, size_t len, char *mac, char *serNum)
{
    FILE* out = NULL, *file = NULL;
    char command[MAX_BUF_SIZE] = {'\0'};
    if(strlen(name)>0)
    {
        file = fopen(name, "r");
        if(file)
        {
            snprintf(command,sizeof(command),"%s %s %s",name,serNum,mac);
            out = popen(command, "r");
            if(out)
            {
                fgets(token, len, out);
                pclose(out);
            }
            fclose(file);
        }
        else
        {
            ParodusError ("File %s open error\n", name);
        }
    }
}

// strips ':' characters
// verifies that there exactly 12 characters
int parse_mac_address (char *target, const char *arg)
{
	int count = 0;
	int i;
	char c;

	for (i=0; (c=arg[i]) != 0; i++) {
		if (c !=':')
			count++;
	}
	if (count != 12)
		return -1;			// bad mac address
	for (i=0; (c=arg[i]) != 0; i++) {
		if (c != ':')
			*(target++) = c;
	}
	*target = 0;	// terminating null
	return 0;
}

int server_is_http (const char *full_url,
	const char **server_ptr)
{
	int http_match;
	const char *ptr;
	
	if (strncmp(full_url, "https://", 8) == 0) {
		http_match = 0;
		ptr = full_url + 8;
	} else if (strncmp(full_url, "http://", 7) == 0) {
		http_match = 1;
		ptr = full_url + 7;	
	} else {
		ParodusError ("Invalid url %s\n", full_url);
		return -1;
	}
	if (NULL != server_ptr)
		*server_ptr = ptr;
	return http_match;
}
	
	
int parse_webpa_url__ (const char *full_url, 
	char *server_addr, int server_addr_buflen,
	char *port_buf, int port_buflen)
{
	const char *server_ptr;
	char *port_val;
	char *end_port;
	size_t server_len;
	int http_match;
	char *closeBracket = NULL;
	char *openBracket = NULL;
	char *checkPort = NULL;

	ParodusInfo ("full url: %s\n", full_url);
	http_match = server_is_http (full_url, &server_ptr);
	if (http_match < 0)
		return http_match;

	ParodusInfo ("server address copied from url\n");
	parStrncpy (server_addr, server_ptr, server_addr_buflen);
	server_len = strlen(server_addr);
	// If there's a '/' on end, null it out
	if ((server_len>0) && (server_addr[server_len-1] == '/'))
		server_addr[server_len-1] = '\0';

    openBracket = strchr(server_addr,'[');
    if(openBracket != NULL){
        //Remove [ from server address
        char *remove = server_addr;
        remove++;
        parStrncpy (server_addr, remove, server_addr_buflen);
        closeBracket = strchr(server_addr,']');
        if(closeBracket != NULL){
            //Remove ] by making it as null
            *closeBracket = '\0';
            closeBracket++;
            checkPort = strchr(closeBracket,':');
            if (NULL == checkPort) {
                if (http_match)
                    parStrncpy (port_buf, "80", port_buflen);
                else
                    parStrncpy (port_buf, "443", port_buflen);
            } else {
                checkPort++;
                end_port = strchr (checkPort, '/');
                if (NULL != end_port)
                    *end_port = '\0'; // terminate port with null
                parStrncpy (port_buf, checkPort, port_buflen);
            }

        }else{
            ParodusError("Invalid url %s\n", full_url);
            return -1;
        }
    }else if (strchr(server_addr,']') != NULL ){
		ParodusError("Invalid url %s\n", full_url);
		return -1;
	}else{
	// Look for ':'
	port_val = strchr (server_addr, ':');

	if (NULL == port_val) {
		if (http_match)
			parStrncpy (port_buf, "80", port_buflen);
		else
			parStrncpy (port_buf, "443", port_buflen);
		end_port = strchr (server_addr, '/');
		if (NULL != end_port) {
			*end_port = '\0'; // terminate server address with null
		}

	} else {
		*port_val = '\0'; // terminate server address with null
		port_val++;
		end_port = strchr (port_val, '/');
		if (NULL != end_port)
			*end_port = '\0'; // terminate port with null
		parStrncpy (port_buf, port_val, port_buflen);
	}
    }
	ParodusInfo ("server %s, port %s, http_match %d\n", 
		server_addr, port_buf, http_match);
	return http_match;

}

unsigned int parse_num_arg (const char *arg, const char *arg_name)
{
	unsigned int result = 0;
	int i;
	char c;
	
	if (arg[0] == '\0') {
		ParodusError ("Empty %s argument\n", arg_name);
		return (unsigned int) -1;
	}
	for (i=0; '\0' != (c=arg[i]); i++)
	{
		if ((c<'0') || (c>'9')) {
			ParodusError ("Non-numeric %s argument\n", arg_name);
			return (unsigned int) -1;
		}
		result = (result*10) + c - '0';
	}
	return result;
}

int parse_webpa_url (const char *full_url, 
	char **server_addr, unsigned int *port)
{
  int allow_insecure;
  unsigned int port_val;
  int buflen = strlen (full_url) + 1;
  char *url_buf = NULL;
  char port_buf[8];

#define ERROR__(msg) \
  ParodusError (msg); \
  if (NULL != url_buf) \
    free (url_buf);

  *server_addr = NULL;
     
  url_buf = (char *) malloc (buflen);
  if (NULL == url_buf) {
    ERROR__ ("parse_webpa_url allocatio n failed.\n")
    return -1;
  }
  allow_insecure = parse_webpa_url__ (full_url,
	url_buf, buflen, port_buf, 8);
  if (allow_insecure < 0) {
    ERROR__ ("parse_webpa_url invalid url\n")
    return -1;
  }
  port_val = parse_num_arg (port_buf, "server port");
  if (port_val == (unsigned int) -1) {
    ERROR__ ("Invalid port in server url")
    return -1;
  }
  if ((port_val == 0) || (port_val > 65535)) {
    ERROR__ ("port value out of range in server url")
    return -1;
  }
  *server_addr = url_buf;
  *port = port_val;	
  return allow_insecure;
#undef ERROR__
}


int parseCommandLine(int argc,char **argv,ParodusCfg * cfg)
{
    static const struct option long_options[] = {
        {"hw-model",                required_argument, 0, 'm'},
        {"hw-serial-number",        required_argument, 0, 's'},
        {"hw-manufacturer",         required_argument, 0, 'f'},
        {"hw-mac",                  required_argument, 0, 'd'},
        {"hw-last-reboot-reason",   required_argument, 0, 'r'},
        {"fw-name",                 required_argument, 0, 'n'},
        {"boot-time",               required_argument, 0, 'b'},
        {"webpa-url",               required_argument, 0, 'u'},
        {"webpa-ping-timeout",      required_argument, 0, 't'},
        {"webpa-backoff-max",       required_argument, 0, 'o'},
        {"webpa-interface-used",    required_argument, 0, 'i'},
        {"parodus-local-url",       required_argument, 0, 'l'},
        {"partner-id",              required_argument, 0, 'p'},
#ifdef ENABLE_SESHAT
        {"seshat-url",              required_argument, 0, 'e'},
#endif
        {"dns-txt-url",             required_argument, 0, 'D'},
        {"acquire-jwt",				required_argument, 0, 'j'},
        {"jwt-algo",                required_argument, 0, 'a'},
        {"jwt-public-key-file",     required_argument, 0, 'k'},
        {"ssl-cert-path",           required_argument, 0, 'c'},
        {"force-ipv4",              no_argument,       0, '4'},
        {"force-ipv6",              no_argument,       0, '6'},
        {"token-read-script",       required_argument, 0, 'T'},
        {"boot-time-retry-wait",    required_argument, 0, 'w'},
	{"token-acquisition-script",     required_argument, 0, 'J'},
	{"crud-config-file",        required_argument, 0, 'C'},
#ifdef ENABLE_MUTUAL_AUTH
	{"client-cert-path",		required_argument, 0, 'q'},
	{"client-key-path",		required_argument, 0, 'K'},
#endif
        {0, 0, 0, 0}
    };
    int c;
    ParodusInfo("Parsing parodus command line arguments..\n");

	if (NULL == cfg) {
		ParodusError ("NULL cfg structure\n");
		return -1;
	} 
	cfg->flags = 0;
	parStrncpy (cfg->webpa_url, "", sizeof(cfg->webpa_url));
	cfg->acquire_jwt = 0;
	cfg->jwt_algo = 0;
	parStrncpy (cfg->jwt_key, "", sizeof(cfg->jwt_key));
	cfg->crud_config_file = NULL;
	cfg->cloud_status = NULL;
	cfg->cloud_disconnect = NULL;
	optind = 1;  /* We need this if parseCommandLine is called again */
    while (1)
    {

      /* getopt_long stores the option index here. */
      int option_index = 0;
#ifdef ENABLE_MUTUAL_AUTH
      c = getopt_long (argc, argv, "m:s:f:d:r:n:b:u:t:o:i:l:p:e:D:j:a:k:c:T:w:J:46:Cq:K:",
				long_options, &option_index);
#else
      c = getopt_long (argc, argv, "m:s:f:d:r:n:b:u:t:o:i:l:p:e:D:j:a:k:c:T:w:J:46:C",
				long_options, &option_index);

#endif
      /* Detect the end of the options. */
      if (c == -1)
        break;

      switch (c)
        {
        case 'm':
          parStrncpy(cfg->hw_model, optarg,sizeof(cfg->hw_model));
          ParodusInfo("hw-model is %s\n",cfg->hw_model);
         break;
        
        case 's':
          parStrncpy(cfg->hw_serial_number,optarg,sizeof(cfg->hw_serial_number));
          ParodusInfo("hw_serial_number is %s\n",cfg->hw_serial_number);
          break;

        case 'f':
          parStrncpy(cfg->hw_manufacturer, optarg,sizeof(cfg->hw_manufacturer));
          ParodusInfo("hw_manufacturer is %s\n",cfg->hw_manufacturer);
          break;

        case 'd':
					if (parse_mac_address (cfg->hw_mac, optarg) == 0) {
            ParodusInfo ("hw_mac is %s\n",cfg->hw_mac);
					} else {
						ParodusError ("Bad mac address %s\n", optarg);
						return -1;
					}
          break;
#ifdef ENABLE_SESHAT
         case 'e':
           parStrncpy(cfg->seshat_url, optarg,sizeof(cfg->seshat_url));
           ParodusInfo("seshat_url is %s\n",cfg->seshat_url);
          break;
#endif
        case 'r':
          parStrncpy(cfg->hw_last_reboot_reason, optarg,sizeof(cfg->hw_last_reboot_reason));
          ParodusInfo("hw_last_reboot_reason is %s\n",cfg->hw_last_reboot_reason);
          break;

        case 'n':
          parStrncpy(cfg->fw_name, optarg,sizeof(cfg->fw_name));
          ParodusInfo("fw_name is %s\n",cfg->fw_name);
          break;

        case 'b':
          cfg->boot_time = parse_num_arg (optarg, "boot-time");
          ParodusInfo("boot_time is %d\n",cfg->boot_time);
          break;
       
         case 'u':
			parStrncpy(cfg->webpa_url, optarg,sizeof(cfg->webpa_url));
			if (server_is_http (cfg->webpa_url, NULL) < 0) {
				ParodusError ("Bad webpa url %s\n", optarg);
				return -1;
			}
          ParodusInfo("webpa_url is %s\n",cfg->webpa_url);
          break;
        
        case 't':
          cfg->webpa_ping_timeout = parse_num_arg (optarg, "webpa-ping-timeout");
          if (cfg->webpa_ping_timeout == (unsigned int) -1)
			return -1;
          ParodusInfo("webpa_ping_timeout is %d\n",cfg->webpa_ping_timeout);
          break;

        case 'o':
          cfg->webpa_backoff_max = parse_num_arg (optarg, "webpa-backoff-max");
          if (cfg->webpa_backoff_max == (unsigned int) -1)
			return -1;
          ParodusInfo("webpa_backoff_max is %d\n",cfg->webpa_backoff_max);
          break;

        case 'i':
          parStrncpy(cfg->webpa_interface_used, optarg,sizeof(cfg->webpa_interface_used));
          ParodusInfo("webpa_interface_used is %s\n",cfg->webpa_interface_used);
          break;
          
        case 'l':
          parStrncpy(cfg->local_url, optarg,sizeof(cfg->local_url));
          ParodusInfo("parodus local_url is %s\n",cfg->local_url);
          break;
        case 'D':
          // like 'fabric' or 'test'
          // this parameter is used, along with the hw_mac parameter
          // to create the dns txt record id
          parStrncpy(cfg->dns_txt_url, optarg,sizeof(cfg->dns_txt_url));
          ParodusInfo("parodus dns-txt-url is %s\n",cfg->dns_txt_url);
          break;
		 
        case 'j':
          cfg->acquire_jwt = parse_num_arg (optarg, "acquire-jwt");
          if (cfg->acquire_jwt == (unsigned int) -1)
			return -1;
          ParodusInfo("acquire jwt option is %d\n",cfg->acquire_jwt);
          break;

		case 'a':
			// the command line argument is a list of allowed algoritms,
			// separated by colons, like "RS256:RS512:none"
			cfg->jwt_algo = get_algo_mask (optarg);
			if (cfg->jwt_algo == (unsigned int) -1) {
			  return -1;
			}
          ParodusInfo("jwt_algo is %u\n",cfg->jwt_algo);
          break;
		case 'k':
          read_key_from_file (optarg, cfg->jwt_key, sizeof(cfg->jwt_key));
          ParodusInfo("jwt_key is %s\n",cfg->jwt_key);
          break;
        case 'p':
          parStrncpy(cfg->partner_id, optarg,sizeof(cfg->partner_id));
          ParodusInfo("partner_id is %s\n",cfg->partner_id);
          break;

        case 'c':
          parStrncpy(cfg->cert_path, optarg,sizeof(cfg->cert_path));
          ParodusInfo("cert_path is %s\n",cfg->cert_path);
          break;

        case '4':
          ParodusInfo("Force IPv4\n");
          cfg->flags |= FLAGS_IPV4_ONLY;
          break;

        case '6':
          ParodusInfo("Force IPv6\n");
          cfg->flags |= FLAGS_IPV6_ONLY;
          break;

        case 'J':
          parStrncpy(cfg->token_acquisition_script, optarg,sizeof(cfg->token_acquisition_script));
          break;
        
        case 'T':
          parStrncpy(cfg->token_read_script, optarg,sizeof(cfg->token_read_script));
          break;

        case 'w':
          cfg->boot_retry_wait = parse_num_arg (optarg, "boot-time-retry-wait");
          ParodusInfo("boot_retry_wait is %d\n",cfg->boot_retry_wait);
          break;

		case 'C':
		  cfg->crud_config_file = strdup(optarg);
		  ParodusInfo("crud_config_file is %s\n", cfg->crud_config_file);
		  break;

#ifdef ENABLE_MUTUAL_AUTH
        case 'q':
          parStrncpy(cfg->client_cert_path, optarg,sizeof(cfg->client_cert_path));
          break;

        case 'K':
          parStrncpy(cfg->client_key_path, optarg,sizeof(cfg->client_key_path));
          break;
#endif

        case '?':
          /* getopt_long already printed an error message. */
          break;

        default:
           ParodusError("Enter Valid commands..\n");
		   return -1;
        }
    }

	if (0 == strlen (cfg->webpa_url)) {
		ParodusError ("Missing webpa url argument\n");
		return -1;
	}

    if (cfg->acquire_jwt) {
		if (0 == cfg->jwt_algo) {
			ParodusError ("Missing jwt algorithm argument\n");
			return -1;
		}
		if ((0 != (cfg->jwt_algo & rsa_algorithms)) &&
		    (0 == strlen (cfg->jwt_key)) ) {
			ParodusError ("Missing jwt public key file argument\n");
			return -1;
		}
		    
	}
	
    ParodusPrint("argc is :%d\n", argc);
    ParodusPrint("optind is :%d\n", optind);

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
      ParodusPrint ("non-option ARGV-elements: ");
      while (optind < argc)
        ParodusPrint ("%s ", argv[optind++]);
      putchar ('\n');
    }
    return 0;
}

/*
* call parodus create/acquisition script to create new auth token, if success then calls 
* execute_token_script func with args as parodus read script.
*/

void createNewAuthToken(char *newToken, size_t len)
{
	//Call create script
	char output[12] = {'\0'};
	execute_token_script(output,get_parodus_cfg()->token_acquisition_script,sizeof(output),get_parodus_cfg()->hw_mac,get_parodus_cfg()->hw_serial_number);
  	if (strlen(output)>0  && strcmp(output,"SUCCESS")==0)
	{
		//Call read script 
		execute_token_script(newToken,get_parodus_cfg()->token_read_script,len,get_parodus_cfg()->hw_mac,get_parodus_cfg()->hw_serial_number);
	}	
	else 
	{
		ParodusError("Failed to create new token\n");
	}
}

/*
* Fetches authorization token from the output of read script. If read script returns "ERROR"
* it will call createNewAuthToken to create and read new token 
*/

void getAuthToken(ParodusCfg *cfg)
{
	//local var to update cfg->webpa_auth_token only in success case
	char output[4069] = {'\0'} ;
	
	if( strlen(cfg->token_read_script) !=0 && strlen(cfg->token_acquisition_script) !=0)
    	{
		execute_token_script(output,cfg->token_read_script,sizeof(output),cfg->hw_mac,cfg->hw_serial_number);
		
	    	if ((strlen(output) == 0))
	    	{
			ParodusError("Unable to get auth token\n");
		}
		else if(strcmp(output,"ERROR")==0)
		{
			ParodusInfo("Failed to read token from %s. Proceeding to create new token.\n",cfg->token_read_script);
			//Call create/acquisition script
			createNewAuthToken(cfg->webpa_auth_token, sizeof(cfg->webpa_auth_token));	
		}
		else
		{
			ParodusInfo("update cfg->webpa_auth_token in success case\n");
			parStrncpy(cfg->webpa_auth_token, output, sizeof(cfg->webpa_auth_token));
		}
	}
	else
	{
        	ParodusInfo("Both read and write file are NULL \n");
	}
}

void setDefaultValuesToCfg(ParodusCfg *cfg)
{
    if(cfg == NULL)
    {
        ParodusError("cfg is NULL\n");
        return;
    }
    
    ParodusInfo("Setting default values to parodusCfg\n");
    parStrncpy(cfg->local_url, PARODUS_UPSTREAM, sizeof(cfg->local_url));

	cfg->acquire_jwt = 0;

    parStrncpy(cfg->dns_txt_url, DNS_TXT_URL, sizeof(cfg->dns_txt_url));

    parStrncpy(cfg->jwt_key, "\0", sizeof(cfg->jwt_key));
    
    cfg->jwt_algo = 0;
   
    parStrncpy(cfg->cert_path, "\0", sizeof(cfg->cert_path));

    cfg->flags = 0;
    
    parStrncpy(cfg->webpa_path_url, WEBPA_PATH_URL,sizeof(cfg->webpa_path_url));
    
    snprintf(cfg->webpa_protocol, sizeof(cfg->webpa_protocol), "%s-%s", PROTOCOL_VALUE, GIT_COMMIT_TAG);
    ParodusInfo(" cfg->webpa_protocol is %s\n", cfg->webpa_protocol);
    
    parStrncpy(cfg->webpa_uuid, "1234567-345456546",sizeof(cfg->webpa_uuid));
    ParodusPrint("cfg->webpa_uuid is :%s\n", cfg->webpa_uuid);
    cfg->crud_config_file = strdup("parodus_cfg.json");
	ParodusPrint("Default crud_config_file is %s\n", cfg->crud_config_file);
	cfg->cloud_status = CLOUD_STATUS_OFFLINE;
	ParodusInfo("Default cloud_status is %s\n", cfg->cloud_status);
}

void loadParodusCfg(ParodusCfg * config,ParodusCfg *cfg)
{
    if(config == NULL)
    {
        ParodusError("config is NULL\n");
        return;
    }
    
    if(strlen (config->hw_model) !=0)
    {
          parStrncpy(cfg->hw_model, config->hw_model, sizeof(cfg->hw_model));
    }
    else
    {
        ParodusPrint("hw_model is NULL. read from tmp file\n");
    }
    if( strlen(config->hw_serial_number) !=0)
    {
        parStrncpy(cfg->hw_serial_number, config->hw_serial_number, sizeof(cfg->hw_serial_number));
    }
    else
    {
        ParodusPrint("hw_serial_number is NULL. read from tmp file\n");
    }
    if(strlen(config->hw_manufacturer) !=0)
    {
        parStrncpy(cfg->hw_manufacturer, config->hw_manufacturer,sizeof(cfg->hw_manufacturer));
    }
    else
    {
        ParodusPrint("hw_manufacturer is NULL. read from tmp file\n");
    }
    if(strlen(config->hw_mac) !=0)
    {
       parStrncpy(cfg->hw_mac, config->hw_mac,sizeof(cfg->hw_mac));
    }
    else
    {
        ParodusPrint("hw_mac is NULL. read from tmp file\n");
    }
    if(strlen (config->hw_last_reboot_reason) !=0)
    {
         parStrncpy(cfg->hw_last_reboot_reason, config->hw_last_reboot_reason,sizeof(cfg->hw_last_reboot_reason));
    }
    else
    {
        ParodusPrint("hw_last_reboot_reason is NULL. read from tmp file\n");
    }
    if(strlen(config->fw_name) !=0)
    {   
        parStrncpy(cfg->fw_name, config->fw_name,sizeof(cfg->fw_name));
    }
    else
    {
        ParodusPrint("fw_name is NULL. read from tmp file\n");
    }
    if( strlen(config->webpa_url) !=0)
    {
        parStrncpy(cfg->webpa_url, config->webpa_url,sizeof(cfg->webpa_url));
    }
    else
    {
        ParodusPrint("webpa_url is NULL. read from tmp file\n");
    }
    if(strlen(config->webpa_interface_used )!=0)
    {
        parStrncpy(cfg->webpa_interface_used, config->webpa_interface_used,sizeof(cfg->webpa_interface_used));
    }
    else
    {
        ParodusPrint("webpa_interface_used is NULL. read from tmp file\n");
    }
    if( strlen(config->local_url) !=0)
    {
        parStrncpy(cfg->local_url, config->local_url,sizeof(cfg->local_url));
    }
    else
    {
		ParodusInfo("parodus local_url is NULL. adding default url\n");
		parStrncpy(cfg->local_url, PARODUS_UPSTREAM, sizeof(cfg->local_url));
        
    }

    if( strlen(config->partner_id) !=0)
    {
        parStrncpy(cfg->partner_id, config->partner_id,sizeof(cfg->partner_id));
    }
    else
    {
		ParodusPrint("partner_id is NULL. read from tmp file\n");
    }
#ifdef ENABLE_SESHAT
    if( strlen(config->seshat_url) !=0)
    {
        parStrncpy(cfg->seshat_url, config->seshat_url,sizeof(cfg->seshat_url));
    }
    else
    {
        ParodusInfo("seshat_url is NULL. Read from tmp file\n");
    }
#endif
	cfg->acquire_jwt = config->acquire_jwt;

     if( strlen(config->dns_txt_url) !=0)
    {
        parStrncpy(cfg->dns_txt_url, config->dns_txt_url, sizeof(cfg->dns_txt_url));
    }
    else
    {
	ParodusInfo("parodus dns-txt-url is NULL. adding default\n");
	parStrncpy(cfg->dns_txt_url, DNS_TXT_URL, sizeof(cfg->dns_txt_url));
    }

    if(strlen(config->jwt_key )!=0)
    {
        parStrncpy(cfg->jwt_key, config->jwt_key,sizeof(cfg->jwt_key));
    }
    else
    {
        parStrncpy(cfg->jwt_key, "\0", sizeof(cfg->jwt_key));
        ParodusPrint("jwt_key is NULL. set to empty\n");
    }

	cfg->jwt_algo = config->jwt_algo;        

    if(strlen(config->cert_path )!=0)
    {
        parStrncpy(cfg->cert_path, config->cert_path,sizeof(cfg->cert_path));
    }
    else
    {
        parStrncpy(cfg->cert_path, "\0", sizeof(cfg->cert_path));
        ParodusPrint("cert_path is NULL. set to empty\n");
    }

    if(strlen(config->token_acquisition_script )!=0)
    {
          parStrncpy(cfg->token_acquisition_script, config->token_acquisition_script,sizeof(cfg->token_acquisition_script));
    }
    else
    {
          ParodusPrint("token_acquisition_script is NULL. read from tmp file\n");
    }
        
    if(strlen(config->token_read_script )!=0)
    {
          parStrncpy(cfg->token_read_script, config->token_read_script,sizeof(cfg->token_read_script));
    }
    else
    {
          ParodusPrint("token_read_script is NULL. read from tmp file\n");
    }

    cfg->boot_time = config->boot_time;
    cfg->webpa_ping_timeout = config->webpa_ping_timeout;
    cfg->webpa_backoff_max = config->webpa_backoff_max;
    parStrncpy(cfg->webpa_path_url, WEBPA_PATH_URL,sizeof(cfg->webpa_path_url));
    snprintf(cfg->webpa_protocol, sizeof(cfg->webpa_protocol), "%s-%s", PROTOCOL_VALUE, GIT_COMMIT_TAG);
    ParodusInfo("cfg->webpa_protocol is %s\n", cfg->webpa_protocol);
    parStrncpy(cfg->webpa_uuid, "1234567-345456546",sizeof(cfg->webpa_uuid));
    ParodusPrint("cfg->webpa_uuid is :%s\n", cfg->webpa_uuid);
    
    if(config->crud_config_file != NULL)
    {
        cfg->crud_config_file = strdup(config->crud_config_file);
    }
    else
    {
        ParodusPrint("crud_config_file is NULL. set to empty\n");
    }
}


