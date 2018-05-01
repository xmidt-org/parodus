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

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/


#define SERVICE_NAME "producer"

static void sig_handler(int sig);
static int main_loop(libpd_cfg_t *cfg);
static void _help(char *, char *);

static char *mac_address;

int main( int argc, char **argv)
{
    const char *option_string = "p:c:w:d:f:m::h::";
    static const struct option options[] = {
        { "help",         optional_argument, 0, 'h' },
        { "parodus-url",  required_argument, 0, 'p' },
        { "client-url",   required_argument, 0, 'c' },
	{ "mac-address",  optional_argument, 0, 'm'},
        { 0, 0, 0, 0 }
    };

    libpd_cfg_t cfg = { .service_name = SERVICE_NAME,
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

    snprintf(source,127,"mac:/%s/%s", mac_address, SERVICE_NAME);
    snprintf(destination, 127, "node-change:");

    printf("starting the main loop...\n");
    do {
        memset(&wrp_msg, 0, sizeof(wrp_msg_t));
        wrp_msg.msg_type = WRP_MSG_TYPE__EVENT;     

	wrp_msg.u.event.source = strdup(source);
        wrp_msg.u.event.dest   = strdup(destination);
	wrp_msg.u.event.payload = NULL;
        wrp_msg.u.event.payload_size = 0;
        
        
        rv = libparodus_send(hpd, &wrp_msg);
        if (0 == rv) {
            printf("producer sent message with no errors\n");
        } else {
             printf("producer send error %d\n", rv);
        }
        sleep(10); // should be 60
       
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



