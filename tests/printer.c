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
#define debug_error(...)      cimplog_error("printer", __VA_ARGS__)
#define debug_info(...)       cimplog_info("printer", __VA_ARGS__)
#define debug_print(...)      cimplog_debug("printer", __VA_ARGS__)


#define SERVICE_NAME "printer"

static void sig_handler(int sig);
static int main_loop(libpd_cfg_t *cfg);
static void _help(char *, char *);
static int wait_time = 60 * 1000; // One minute

int main( int argc, char **argv)
{
    const char *option_string = "p:c:w:d:f:m:t:h::";
    static const struct option options[] = {
        { "help",         optional_argument, 0, 'h' },
        { "parodus-url",  required_argument, 0, 'p' },
        { "client-url",   required_argument, 0, 'c' },
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



static int main_loop(libpd_cfg_t *cfg)
{
    int rv;
    wrp_msg_t *wrp_msg;
    libpd_instance_t hpd_instance;
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
