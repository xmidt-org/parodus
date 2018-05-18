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
#include <time.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/


#define SERVICE_NAME "producer"
#define CONTENT_TYPE_JSON  "application/json"

static void sig_handler(int sig);
static int main_loop(libpd_cfg_t *cfg);
static void _help(char *, char *);

static char *mac_address;

static int send_period = 60; // sleep time between each send.

int main( int argc, char **argv)
{
    const char *option_string = "p:c:w:d:f:m:t:h::";
    static const struct option options[] = {
        { "help",         optional_argument, 0, 'h' },
        { "parodus-url",  required_argument, 0, 'p' },
        { "client-url",   required_argument, 0, 'c' },
        { "mac-address",  optional_argument, 0, 'm'},
        { "send-period",  required_argument, 0, 't'},
        { 0, 0, 0, 0 }
    };

    libpd_cfg_t cfg = { .service_name = SERVICE_NAME,
                       /* .receive = true, causes a loop back from libparodus hacked ;-) */
			.receive = false,
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
            case 't':
                {
                    int val = atoi(optarg);
                    if (val > 0) {
                      send_period = val;
                    }
                }
                break;
            case 'm':
                mac_address = strdup(optarg);
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
            printf("%s parodus_url not specified!\n", argv[0]);
            rv = -1;
        }
        if ((NULL == cfg.client_url)) {
            printf("%s client_url not specified !\n", argv[0]);
            rv = -2;
        }        
     }
    
    if (rv != 0) {
        printf("%s  program terminating\n", argv[0]);
    }

    if( NULL != cfg.parodus_url )   free( (char*) cfg.parodus_url );
    if( NULL != cfg.client_url )    free( (char*) cfg.client_url );
    if( NULL != mac_address ) free((char *) mac_address);
    return rv;
}

static void sig_handler(int sig)
{
    if( sig == SIGINT ) {
        printf("SIGINT received! Program Terminating!\n");
        exit(0);
    } else if ( sig == SIGTERM ) {
        printf("SIGTERM received! Program Terminating!\n");
        exit(0);
    } else if( sig == SIGUSR1 ) {
        signal(SIGUSR1, sig_handler); /* reset it to this function */
        printf("SIGUSR1 received!\n");
    } else if( sig == SIGUSR2 ) {
        signal(SIGUSR2, sig_handler);
        printf("SIGUSR2 received!\n");
    } else if( sig == SIGCHLD ) {
        signal(SIGCHLD, sig_handler); /* reset it to this function */
        printf("SIGHLD received!\n");
    } else if( sig == SIGPIPE ) {
        signal(SIGPIPE, sig_handler); /* reset it to this function */
        printf("SIGPIPE received!\n");
    } else if( sig == SIGALRM ) {
        signal(SIGALRM, sig_handler); /* reset it to this function */
        printf("SIGALRM received!\n");
    } else {
        printf("Signal %d received!\n", sig);
        exit(0);
    }
}

static int main_loop(libpd_cfg_t *cfg)
{
    int rv;
    wrp_msg_t wrp_msg;
    libpd_instance_t hpd;
    int backoff_retry_time = 0;
    int max_retry_sleep = (1 << 9) - 1;
    int c = 2;
    char source[128];
    char destination[128];
    char *contentType;
    cJSON *response;
    cJSON *parameters;
    cJSON *device_id, *time_stamp;

    (void ) device_id; (void ) parameters; (void ) time_stamp;

    while( true ) {
        rv = libparodus_init( &hpd, cfg );
        if( 0 != rv ) {
            printf("Init for parodus (url %s): %d '%s'\n", cfg->parodus_url, rv, libparodus_strerror(rv) );	
            backoff_retry_time = (1 << c) - 1;
            sleep(backoff_retry_time);
            c++;

            if( backoff_retry_time >= max_retry_sleep ) {
                c = 2;
                backoff_retry_time = 0;
            }
          libparodus_shutdown(&hpd);
          continue;
        }  // else
            printf("producer service registered with libparodus url %s\n", cfg->parodus_url);
            break;
    }

    snprintf(source,127,"mac:%s/%s", mac_address, SERVICE_NAME);
    snprintf(destination, 127, "event:node-change/");
    // zztop snprintf(destination, 127, "event:node-change/%s","printer");
    
    contentType = (char *) malloc(sizeof(char) * (strlen(CONTENT_TYPE_JSON) + 1));
	if (contentType) {
        strncpy(contentType, CONTENT_TYPE_JSON, strlen(CONTENT_TYPE_JSON) + 1);
    } else {
        printf("Hell Froze over! Malloc failed!\n");
        exit (-911);
    } 
    
    printf("starting the main loop...\n");
    do {
        struct timespec tm;
        time_t unix_time = 0;
        char *str;
        char time_str[32] = "0";
        
        if( 0 == clock_gettime(CLOCK_REALTIME, &tm) ) {
            unix_time = tm.tv_sec; // ignore tm.tv_nsec
            snprintf(time_str, 32, "%lu", unix_time);
        }    
            
        response = cJSON_CreateObject();
	cJSON_AddStringToObject(response, "device_id", mac_address);
	cJSON_AddStringToObject(response, "timestamp", time_str);
        str = cJSON_PrintUnformatted(response);
        printf("Payload Response: %s\n", str);

        memset(&wrp_msg, 0, sizeof(wrp_msg_t));
        wrp_msg.msg_type = WRP_MSG_TYPE__EVENT;     

	    wrp_msg.u.event.source = strdup(source);
        wrp_msg.u.event.dest   = strdup(destination);
        wrp_msg.u.event.content_type = strdup(contentType);
	    wrp_msg.u.event.payload = str;
        wrp_msg.u.event.payload_size = strlen(str);
        
        
        rv = libparodus_send(hpd, &wrp_msg);
        if (0 == rv) {
            printf("producer sent message with no errors\n");
        } else {
             printf("producer send error %d\n", rv);
        }

        sleep(send_period);

        {
            wrp_msg_t *msg = NULL;
            rv = libparodus_receive(hpd, &msg, 1);
            if (0 == rv && msg) {
                char *bytes = NULL;
                ssize_t n = wrp_struct_to(msg, WRP_STRING, (void **) &bytes);
                if (n > 0) {
                    printf("**Producer Got**: \n%s", bytes);
                    free(bytes);
                } else {
		printf("Service Producer Memory Error on WRP message conversion\n");
                printf("wrp_msg->src %s\n",(char *)msg->u.crud.source);
                printf("wrp_msg->dest %s\n",(char *)msg->u.crud.dest);                
                printf("wrp_msg->u.req.payload %s\n",(char *)msg->u.crud.payload);
                }
            }
        }
       
    } while (true);

    (void ) libparodus_shutdown(&hpd);
    printf("End of producer\n");
    return 0;
}

void _help(char *a, char *b)
{
    (void ) a; (void ) b;
    
    // stub
}



