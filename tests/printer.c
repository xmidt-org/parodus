/**
 * Copyright 2018 Comcast Cable Communications Management, LLC
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>

#include <libparodus.h>
#include <msgpack.h>

#include <stdarg.h>
#include <cimplog/cimplog.h>
#include <cJSON.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define debug_error(...)      cimplog_error("printer", __VA_ARGS__)
#define debug_info(...)       cimplog_info("printer", __VA_ARGS__)
#define debug_print(...)      cimplog_debug("printer", __VA_ARGS__)


#define SERVICE_NAME "printer"
#define CONTENT_TYPE_JSON  "application/json"

static void sig_handler(int sig);
static int main_loop(libpd_cfg_t *cfg);
static void _help(char *, char *);
static int wait_time = 60 * 1000; // One minute
void subscribeToEvent(char *regex);
libpd_instance_t hpd_instance;
static char *mac_address;

int main( int argc, char **argv)
{
    const char *option_string = "p:c:w:d:f:m:t:h::";
    static const struct option options[] = {
        { "help",         optional_argument, 0, 'h' },
        { "parodus-url",  required_argument, 0, 'p' },
        { "client-url",   required_argument, 0, 'c' },
	{ "mac-address",  optional_argument, 0, 'm'},
        { "wait-time",    required_argument, 0, 't'},
        { 0, 0, 0, 0 }
    };

    libpd_cfg_t cfg = { .service_name = SERVICE_NAME,
                        .receive = true,
                        .keepalive_timeout_secs = 64,
                        .parodus_url = NULL,
                        .client_url = NULL
                      };

    int item = 0;
    int opt_index = 0;
    int rv = 0;

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGBUS, sig_handler);
    signal(SIGKILL, sig_handler);
    signal(SIGFPE, sig_handler);
    signal(SIGILL, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGHUP, sig_handler);
    signal(SIGALRM, sig_handler);

    mac_address = NULL;
    
    while( -1 != (item = getopt_long(argc, argv, option_string, options, &opt_index)) ) {
        switch( item ) {
            case 'p':
                cfg.parodus_url = strdup(optarg);
                break;
            case 'c':
                cfg.client_url = strdup(optarg);
                break;
            case 'h':
                _help(argv[0], optarg);
                break;
	    case 'm':
                mac_address = strdup(optarg);
            break;
            case 't':
                {
                    int val = atoi(optarg);
                    if (val > 0) {
                      wait_time = val;
                    }
                }
                break;
            case '?':
                if (strchr(option_string, optopt)) {
                    printf("%s Option %c requires an argument!\n", argv[0], optopt);
                    _help(argv[0], NULL);
                    rv = -7;
                    break;
                } else {
                    printf("%s Unrecognized option %c\n", argv[0], optopt);
                  }

                break;

            default:
                break;
        }
    }

    if( (rv == 0) &&
        (NULL != cfg.parodus_url) &&
        (NULL != cfg.client_url))
    { 
	if (NULL == mac_address) {
            mac_address = strdup("14cfe1234567");
	}       
        main_loop(&cfg);
        rv = 0;
    } else {
        if ((NULL == cfg.parodus_url)) {
            debug_error("%s parodus_url not specified!\n", argv[0]);
            rv = -1;
        }
        if ((NULL == cfg.client_url)) {
            debug_error("%s client_url not specified !\n", argv[0]);
            rv = -2;
        }        
     }
    
    if (rv != 0) {
        debug_error("%s  program terminating\n", argv[0]);
    }

    if( NULL != cfg.parodus_url )   free( (char*) cfg.parodus_url );
    if( NULL != cfg.client_url )    free( (char*) cfg.client_url );
    if( NULL != mac_address ) free((char *) mac_address);

    return rv;
}

static void sig_handler(int sig)
{
    if( sig == SIGINT ) {
        debug_info("SIGINT received! Program Terminating!\n");
        exit(0);
    } else if ( sig == SIGTERM ) {
        debug_info("SIGTERM received! Program Terminating!\n");
        exit(0);
    } else if( sig == SIGUSR1 ) {
        signal(SIGUSR1, sig_handler); /* reset it to this function */
        debug_info("SIGUSR1 received!\n");
    } else if( sig == SIGUSR2 ) {
        signal(SIGUSR2, sig_handler);
        debug_info("SIGUSR2 received!\n");
    } else if( sig == SIGCHLD ) {
        signal(SIGCHLD, sig_handler); /* reset it to this function */
        debug_info("SIGHLD received!\n");
    } else if( sig == SIGPIPE ) {
        signal(SIGPIPE, sig_handler); /* reset it to this function */
        debug_info("SIGPIPE received!\n");
    } else if( sig == SIGALRM ) {
        signal(SIGALRM, sig_handler); /* reset it to this function */
        debug_info("SIGALRM received!\n");
    } else {
        debug_info("Signal %d received!\n", sig);
        exit(0);
    }
}

void subscribeToEvent(char *regex){

	wrp_msg_t *res_wrp_msg = NULL;
	cJSON *response = NULL;
	char * str = NULL;
	char *contentType = NULL;
	int sendStatus;
	char source[128];
	char dest[128];

	res_wrp_msg = (wrp_msg_t *) malloc(sizeof(wrp_msg_t));
	if (res_wrp_msg)
	{
		memset(res_wrp_msg, 0, sizeof(wrp_msg_t));
	}
	else
	{
		printf("In subscribeToEvent() - malloc Failed !!!");
	}
	
	snprintf(source,127,"mac:%s/%s", mac_address, SERVICE_NAME);
	snprintf(dest,127,"mac:%s/%s/%s", mac_address, "parodus","subscribe");
	printf("source is %s\n",source);
	printf("dest is %s\n",dest);	
	response = cJSON_CreateObject();
	//cJSON_AddItemToObject(response, SERVICE_NAME, parameters = cJSON_CreateObject());
	cJSON_AddStringToObject(response, SERVICE_NAME, regex);

	str = cJSON_PrintUnformatted(response);
    printf("Payload Response: %s\n", str);

	res_wrp_msg->msg_type = WRP_MSG_TYPE__CREATE;
	res_wrp_msg->u.crud.payload = (void *)str;
	res_wrp_msg->u.crud.payload_size = strlen(str);
	res_wrp_msg->u.crud.source = strdup(source);
	res_wrp_msg->u.crud.dest = strdup(dest);
	res_wrp_msg->u.crud.transaction_uuid = "c07ee5e1-70be-444c-a156-097c767ad8aa";
	res_wrp_msg->u.crud.status = 1;
	res_wrp_msg->u.crud.rdr = 0;
	contentType = (char *) malloc(sizeof(char) * (strlen(CONTENT_TYPE_JSON) + 1));
	if (contentType)
	{
		strncpy(contentType, CONTENT_TYPE_JSON, strlen(CONTENT_TYPE_JSON) + 1);
		res_wrp_msg->u.crud.content_type = contentType;
	}
	else
	{
		free(res_wrp_msg);
		printf("In subscribeToEvent() - malloc Failed !!!");
	}

	sendStatus = libparodus_send(hpd_instance, res_wrp_msg);
	printf("sendStatus is %d\n", sendStatus);
	if (sendStatus == 0)
	{
		printf("Sent message successfully to parodus\n");
	}
	else
	{
		printf("libparodus_send() Failed to send message: '%s'\n", libparodus_strerror(sendStatus));
	}
}

static int main_loop(libpd_cfg_t *cfg)
{
    int rv;
    wrp_msg_t *wrp_msg;
    int backoff_retry_time = 0;
    int max_retry_sleep = (1 << 9) - 1;
    int c = 2;

    while( true ) {
        rv = libparodus_init( &hpd_instance, cfg );
        if( 0 != rv ) {
            debug_info("Init for parodus (url %s): %d '%s'\n", cfg->parodus_url, rv, libparodus_strerror(rv) );
            backoff_retry_time = (1 << c) - 1;
            sleep(backoff_retry_time);
            c++;

            if( backoff_retry_time >= max_retry_sleep ) {
                c = 2;
                backoff_retry_time = 0;
            }
        } else {
            printf("printer service registered with libparodus url %s\n", cfg->parodus_url);
            break;
        }
        libparodus_shutdown(&hpd_instance);
    }

    //Subscribe to event
    subscribeToEvent("node-change");

    debug_print("starting the main loop...\n");
    while( true ) {
        rv = libparodus_receive(hpd_instance, &wrp_msg, wait_time);

        if( 0 == rv ) {
            char *bytes = NULL;
            ssize_t n = wrp_struct_to(wrp_msg, WRP_STRING, (void **) &bytes);
            if (n > 0) {
                printf("\n%s", bytes);
                free(bytes);
            } else {
                printf("Service Printer Memory Error on WRP message conversion\n");
		printf("wrp_msg->src %s\n",(char *)wrp_msg->u.crud.source);
                printf("wrp_msg->dest %s\n",(char *)wrp_msg->u.crud.dest);                
                printf("wrp_msg->u.req.payload %s\n",(char *)wrp_msg->u.crud.payload);
            }
            
        } else if( 1 == rv || 2 == rv ) {
            printf("Timed out or message closed.\n");
            continue;
        } else {
            printf("Libparodus failed to receive message: '%s'\n",libparodus_strerror(rv));
        }

        if( (NULL != wrp_msg) && (2 != rv) ) {
            wrp_free_struct(wrp_msg);
            wrp_msg = NULL;
        }
    }

    (void ) libparodus_shutdown(&hpd_instance);
    debug_print("End of printer\n");
    return 0;
}

void _help(char *a, char *b)
{
    (void ) a; (void ) b;
    
    // stub
}
