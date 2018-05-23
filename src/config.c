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
#define PROTOCOL_STR_LEN 1024

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

static void execute_token_script(char *token, char *name, size_t len, char *mac, char *serNum);

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

static FILE * open_input_file (const char *fname, int *file_size)
{
  FILE * fd = fopen(fname, "r");
  if (NULL == fd)
  {
    ParodusError ("File %s open error\n", fname);
    abort ();
  }
  
  fseek(fd, 0, SEEK_END);
  *file_size = ftell(fd);
  fseek(fd, 0, SEEK_SET);
  
  return fd;
} 

void read_key_from_file (const char *fname, char **data)
{
  ssize_t nbytes;
  int file_size = 0;
  
  FILE * fd = open_input_file(fname, &file_size);
  
  if (NULL == (*data = (char *) malloc(sizeof(char) * file_size))) {
    ParodusError ("Read file Out of memory\n", fname);
    fclose(fd);
    abort ();
  }
  
  nbytes = fread(*data, sizeof(char), file_size, fd);
  if (nbytes <= 0)
  {
    ParodusError ("Read file %s error\n", fname);
    free(*data);
    fclose(fd);
    abort ();
  }
  fclose(fd);
  ParodusInfo ("%d bytes read\n", nbytes);
  
  return;
}

static void execute_token_script(char *token, char *name, size_t len, char *mac, char *serNum)
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
	
	
int parse_webpa_url(const char *full_url, 
	char *server_addr, int server_addr_buflen,
	char *port_buf, int port_buflen)
{
	const char *server_ptr;
	char *port_val;
	char *end_port;
	size_t server_len;
	int http_match;

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
	{"token-acquisition-script",     required_argument, 0, 'J'},
        {0, 0, 0, 0}
    };
    int c;
    ParodusInfo("Parsing parodus command line arguments..\n");

	if (NULL == cfg) {
		ParodusError ("NULL cfg structure\n");
		return -1;
	} 
	cfg->flags = 0;
	cfg->acquire_jwt = 0;
	cfg->jwt_algo = 0;
	optind = 1;  /* We need this if parseCommandLine is called again */
    int option_index = 0;/* getopt_long stores the option index here. */
    while (1)
    {
      c = getopt_long (argc, argv, "m:s:f:d:r:n:b:u:t:o:i:l:p:e:D:j:a:k:c:T:J:46",
				long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
        break;

      switch (c)
        {
        case 'm':
          cfg->hw_model = strdup (optarg);
          ParodusInfo("hw-model is %s\n",cfg->hw_model);
         break;
        
        case 's':
          cfg->hw_serial_number = strdup (optarg);
          ParodusInfo("hw_serial_number is %s\n",cfg->hw_serial_number);
          break;

        case 'f':
          cfg->hw_manufacturer = strdup (optarg);
          ParodusInfo("hw_manufacturer is %s\n",cfg->hw_manufacturer);
          break;

        case 'd':
        {
            char mac[16];
            if (parse_mac_address (&mac[0], optarg) == 0) {
                cfg->hw_mac = strdup(mac);
                ParodusInfo ("hw_mac is %s\n",cfg->hw_mac);
            } else {
                ParodusError ("Bad mac address %s\n", optarg);
                return -1;
            }
        }
          break;
#ifdef ENABLE_SESHAT
         case 'e':
           cfg->seshat_url = strdup (optarg);
           ParodusInfo("seshat_url is %s\n",cfg->seshat_url);
          break;
#endif
        case 'r':
          cfg->hw_last_reboot_reason = strdup (optarg);
          ParodusInfo("hw_last_reboot_reason is %s\n",cfg->hw_last_reboot_reason);
          break;

        case 'n':
          cfg->fw_name = strdup (optarg);
          ParodusInfo("fw_name is %s\n",cfg->fw_name);
          break;

        case 'b':
          cfg->boot_time = parse_num_arg (optarg, "boot-time");
          if (cfg->boot_time == (unsigned int) -1)
			return -1;
          ParodusInfo("boot_time is %d\n",cfg->boot_time);
          break;
       
         case 'u':
			cfg->webpa_url = strdup (optarg);
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
          cfg->webpa_interface_used = strdup (optarg);
          ParodusInfo("webpa_interface_used is %s\n",cfg->webpa_interface_used);
          break;
          
        case 'l':
          if (NULL != cfg->local_url) {// free default url 
              free(cfg->local_url);
          }
          cfg->local_url = strdup (optarg);
          ParodusInfo("parodus local_url is %s\n",cfg->local_url);
          break;
        case 'D':
          // like 'fabric' or 'test'
          // this parameter is used, along with the hw_mac parameter
          // to create the dns txt record id
          if (NULL != cfg->dns_txt_url) {// free default url 
              free(cfg->dns_txt_url);
          }
          cfg->dns_txt_url = strdup (optarg);
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
          if (NULL != cfg->jwt_key) {// free default jwt_key 
              free(cfg->jwt_key);
          }
          read_key_from_file (optarg, &cfg->jwt_key);
          ParodusInfo("jwt_key is %s\n",cfg->jwt_key);
          break;
        case 'p':
          cfg->partner_id = strdup (optarg);
          ParodusInfo("partner_id is %s\n",cfg->partner_id);
          break;

        case 'c':
          if (NULL != cfg->cert_path) {// free default cert_path 
              free(cfg->cert_path);
          }            
          cfg->cert_path = strdup (optarg);
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
          cfg->token_acquisition_script = strdup (optarg);
          break;
        
        case 'T':
          cfg->token_read_script = strdup(optarg);
          break;

        case '?':
            ParodusError("Unrecognized option %s Aborting ...\n", argv[optind]);
            return -1;

        default:
           ParodusError("Enter Valid commands..\n");
		   return -1;
        }
    }

	if (cfg->webpa_url && (0 == strlen (cfg->webpa_url))) {
		ParodusError ("Missing webpa url argument\n");
		return -1;
	}

    if (cfg->acquire_jwt) {
		if (0 == cfg->jwt_algo) {
			ParodusError ("Missing jwt algorithm argument\n");
			return -1;
		}

		if (cfg->jwt_algo & rsa_algorithms) {
             if  (!cfg->jwt_key || (0 == strlen (cfg->jwt_key)) ) {
                ParodusError ("Missing jwt public key file argument\n");
                return -1;
            }
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
	
	if( cfg->token_read_script && strlen(cfg->token_read_script) !=0 && 
        cfg->token_acquisition_script && strlen(cfg->token_acquisition_script) !=0)
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
            if (cfg->webpa_auth_token != NULL) {
                free(cfg->webpa_auth_token);
            }
            cfg->webpa_auth_token = (char *) malloc(sizeof(char) * SIZE_OF_WEBPA_AUTH_TOKEN);
			createNewAuthToken(cfg->webpa_auth_token, SIZE_OF_WEBPA_AUTH_TOKEN);
		}
		else
		{
			ParodusInfo("update cfg->webpa_auth_token in success case\n");
			cfg->webpa_auth_token = strdup(output);
		}
	}
	else
	{
        	ParodusInfo("Both read and write file are NULL \n");
	}
}

void setDefaultValuesToCfg(ParodusCfg *cfg)
{
    char protocol_str[PROTOCOL_STR_LEN];
    if(cfg == NULL)
    {
        ParodusError("cfg is NULL\n");
        return;
    }
    
    ParodusInfo("Setting default values to parodusCfg\n");
    cfg->local_url = strdup (PARODUS_UPSTREAM);

	cfg->acquire_jwt = 0;

    cfg->dns_txt_url = strdup (DNS_TXT_URL);

    cfg->jwt_key = strdup ("\0");
    
    cfg->jwt_algo = 0;
   
    cfg->cert_path = NULL;

    cfg->flags = 0;
    
    cfg->webpa_path_url = strdup (WEBPA_PATH_URL);
    
    snprintf(protocol_str, PROTOCOL_STR_LEN, "%s-%s", PROTOCOL_VALUE, GIT_COMMIT_TAG);
    cfg->webpa_protocol = strdup(protocol_str);
    ParodusInfo(" cfg->webpa_protocol is %s\n", cfg->webpa_protocol);
    
    cfg->webpa_uuid = strdup ("1234567-345456546");
    ParodusPrint("cfg->webpa_uuid is :%s\n", cfg->webpa_uuid);
    
}

void loadParodusCfg(ParodusCfg * config,ParodusCfg *cfg)
{
    char protocol_str[PROTOCOL_STR_LEN];

    if(config == NULL || cfg == NULL)
    {
        ParodusError("config is NULL\n");
        return;
    }
    
    if(config->hw_model && strlen (config->hw_model) !=0)
    {
        if (cfg->hw_model)
        {
            free(cfg->hw_model);
        }
        cfg->hw_model = strdup (config->hw_model);
    }
    else
    {
        ParodusPrint("hw_model is NULL. read from tmp file\n");
    }
    
    if( config->hw_serial_number && strlen(config->hw_serial_number) !=0)
    {
        if (cfg->hw_serial_number)
        {
            free(cfg->hw_serial_number);
        }
        cfg->hw_serial_number = strdup (config->hw_serial_number);
    }
    else
    {
        ParodusPrint("hw_serial_number is NULL. read from tmp file\n");
    }
    
    if(config->hw_manufacturer && strlen(config->hw_manufacturer) !=0)
    {
        if (cfg->hw_manufacturer)
        {
            free(cfg->hw_manufacturer);
        }
        cfg->hw_manufacturer = strdup (config->hw_manufacturer);
    }
    else
    {
        ParodusPrint("hw_manufacturer is NULL. read from tmp file\n");
    }
    
    if(config->hw_mac && strlen(config->hw_mac) !=0)
    {
        if (cfg->hw_mac)
        {
            free(cfg->hw_mac);
        }
        cfg->hw_mac = strdup (config->hw_mac);
    }
    else
    {
        ParodusPrint("hw_mac is NULL. read from tmp file\n");
    }
    
    if (cfg->hw_last_reboot_reason)
    {
        free(cfg->hw_last_reboot_reason);
    }
    
    if(config->hw_last_reboot_reason && strlen (config->hw_last_reboot_reason) !=0)
    {

         cfg->hw_last_reboot_reason = strdup (config->hw_last_reboot_reason);
    }
    else
    {
        ParodusPrint("hw_last_reboot_reason is NULL. read from tmp file\n");
        cfg->hw_last_reboot_reason = strdup("unspecified");
    }
    
    if(config->fw_name && strlen(config->fw_name) !=0)
    {   
        if (cfg->fw_name)
        {
            free(cfg->fw_name);
        }
        cfg->fw_name = strdup (config->fw_name);
    }
    else
    {
        ParodusPrint("fw_name is NULL. read from tmp file\n");
    }
    
    if( config->webpa_url && strlen(config->webpa_url) !=0)
    {
        if (cfg->webpa_url)
        {
            free(cfg->webpa_url);
        }
        cfg->webpa_url = strdup (config->webpa_url);
    }
    else
    {
        ParodusPrint("webpa_url is NULL. read from tmp file\n");
    }
    
    if(config->webpa_interface_used && strlen(config->webpa_interface_used )!=0)
    {
         if (cfg->webpa_interface_used)
        {
            free(cfg->webpa_interface_used);
        }
        cfg->webpa_interface_used = strdup (config->webpa_interface_used);
    }
    else
    {
        ParodusPrint("webpa_interface_used is NULL. read from tmp file\n");
    }

    if (cfg->local_url)
    {
       free(cfg->local_url);
    }    
    
    if( config->local_url && strlen(config->local_url) !=0)
    {
        cfg->local_url = strdup (config->local_url);
    }
    else
    {
		ParodusInfo("parodus local_url is NULL. adding default url\n");
		cfg->local_url = strdup (PARODUS_UPSTREAM);        
    }

    if( config->partner_id && strlen(config->partner_id) !=0)
    {
        if (cfg->partner_id)
        {
            free(cfg->partner_id);
        }
        cfg->partner_id = strdup (config->partner_id);
    }
    else
    {
		ParodusPrint("partner_id is NULL. read from tmp file\n");
    }
#ifdef ENABLE_SESHAT
    if( config->seshat_url && strlen(config->seshat_url) !=0)
    {
        if (cfg->seshat_url)
        {
            free(cfg->seshat_url);
        }
        cfg->seshat_url = strdup (config->seshat_url);
    }
    else
    {
        ParodusInfo("seshat_url is NULL. Read from tmp file\n");
    }
#endif
	cfg->acquire_jwt = config->acquire_jwt;
    
    if (cfg->dns_txt_url)
    {
        free(cfg->dns_txt_url);
    }
    
    if( config->dns_txt_url && strlen(config->dns_txt_url) !=0)
    {

        cfg->dns_txt_url = strdup (config->dns_txt_url);
    }
    else
    {
        ParodusInfo("parodus dns-txt-url is NULL. adding default\n");
        cfg->dns_txt_url = strdup (DNS_TXT_URL);
    }

    if (cfg->jwt_key)
    {
        free(cfg->jwt_key);
    }
    
    if(config->jwt_key && strlen(config->jwt_key )!=0)
    {
        cfg->jwt_key = strdup (config->jwt_key);
    }
    else
    {
        cfg->jwt_key = strdup ("\0");
        ParodusPrint("jwt_key is NULL. set to empty\n");
    }

	cfg->jwt_algo = config->jwt_algo;        

    if (cfg->cert_path)
    {
        free(cfg->cert_path);
    }
    
    if(config->cert_path && strlen(config->cert_path )!=0)
    {
        cfg->cert_path = strdup (config->cert_path);
    }
    else
    {
        cfg->cert_path = strdup ("\0");
        ParodusPrint("cert_path is NULL. set to empty\n");
    }

    if(config->token_acquisition_script && strlen(config->token_acquisition_script )!=0)
    {
        if (cfg->token_acquisition_script)
        {
            free(cfg->token_acquisition_script);
        }
        cfg->token_acquisition_script = strdup (config->token_acquisition_script);
    }
    else
    {
          ParodusPrint("token_acquisition_script is NULL. read from tmp file\n");
    }
        
    if(config->token_read_script && strlen(config->token_read_script )!=0)
    {
        if (cfg->token_read_script)
        {
            free(cfg->token_read_script);
        }
        cfg->token_read_script = strdup (config->token_read_script);
    }
    else
    {
          ParodusPrint("token_read_script is NULL. read from tmp file\n");
    }

    cfg->boot_time = config->boot_time;
    cfg->webpa_ping_timeout = config->webpa_ping_timeout;
    cfg->webpa_backoff_max = config->webpa_backoff_max;
    if (cfg->webpa_path_url)
    {
        free(cfg->webpa_path_url);
    }    
    cfg->webpa_path_url = strdup (WEBPA_PATH_URL);
    snprintf(protocol_str, PROTOCOL_STR_LEN, "%s-%s", PROTOCOL_VALUE, GIT_COMMIT_TAG);
    if (cfg->webpa_protocol)
    {
        free(cfg->webpa_protocol);
    }    
    cfg->webpa_protocol = strdup (protocol_str);
    ParodusInfo("cfg->webpa_protocol is %s\n", cfg->webpa_protocol);
    if (cfg->webpa_uuid)
    {
        free(cfg->webpa_uuid);
    }
    cfg->webpa_uuid = strdup ("1234567-345456546");
    ParodusPrint("cfg->webpa_uuid is :%s\n", cfg->webpa_uuid);
}

void clean_up_parodus_cfg(ParodusCfg *cfg)
{    
    (void ) cfg;
#if 0
/* DISABLED FOR NOW */    
    if (cfg->hw_model != NULL) free(cfg->hw_model);
    if (cfg->hw_serial_number != NULL) free(cfg->hw_serial_number);
    if (cfg->hw_manufacturer != NULL) free(cfg->hw_manufacturer);
    if (cfg->hw_mac != NULL) free(cfg->hw_mac);
    if (cfg->hw_last_reboot_reason != NULL) free(cfg->hw_last_reboot_reason);
    if (cfg->fw_name != NULL) free(cfg->fw_name);
    if (cfg->webpa_url != NULL) free(cfg->webpa_url);
    if (cfg->webpa_path_url != NULL) free(cfg->webpa_path_url);
    if (cfg->webpa_interface_used != NULL) free(cfg->webpa_interface_used);
    if (cfg->webpa_protocol != NULL) free(cfg->webpa_protocol);
    if (cfg->webpa_uuid != NULL) free(cfg->webpa_uuid);
    if (cfg->local_url != NULL) free(cfg->local_url);
    if (cfg->partner_id != NULL) free(cfg->partner_id);
#ifdef ENABLE_SESHAT
    if (cfg->seshat_url != NULL) free(cfg->seshat_url);
#endif
    if (cfg->dns_txt_url != NULL) free(cfg->dns_txt_url);
    if (cfg->jwt_key != NULL) free(cfg->jwt_key);
    if (cfg->cert_path != NULL) free(cfg->cert_path);
    if (cfg->webpa_auth_token != NULL) free(cfg->webpa_auth_token);
    if (cfg->token_acquisition_script != NULL) free(cfg->token_acquisition_script);
    if (cfg->token_read_script != NULL) free(cfg->token_read_script);
#endif
}