/**
 * Copyright 2016 Comcast Cable Communications Management, LLC
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
 
#include <string.h>
#include "stdlib.h"
#include "config.h"
#include "auth_token.h"
#include "connection.h"
#include "conn_interface.h"
#include "parodus_log.h"
#include <curl/curl.h>
#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#endif
#include "signal.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef    void    Sigfunc(int);

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static void sig_handler(int sig);

Sigfunc *
signal (int signo, Sigfunc *func)
{
    struct sigaction act, oact;

    act.sa_handler = func;
    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    if (signo == SIGALRM) {
#ifdef  SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;     /* SunOS 4.x */
#endif
    } else {
#ifdef  SA_RESTART
        act.sa_flags |= SA_RESTART; /* SVR4, 4.4BSD */
#endif
    }
    if (sigaction (signo, &act, &oact) < 0) {
	ParodusError ("Signal Handler for signal %d not installed!\n", signo);
        return (SIG_ERR);
    }
    return (oact.sa_handler);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int main( int argc, char **argv)
{
    set_global_shutdown_reason (SHUTDOWN_REASON_PARODUS_STOP);
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
	signal(SIGUSR1, sig_handler);
	signal(SIGUSR2, sig_handler);
	signal(SIGKILL, sig_handler);
	signal(SIGQUIT, sig_handler);
	signal(SIGHUP, sig_handler);
	signal(SIGALRM, sig_handler);
#ifdef INCLUDE_BREAKPAD
    breakpad_ExceptionHandler();
#else
	signal(SIGSEGV, sig_handler);
	signal(SIGBUS, sig_handler);
	signal(SIGFPE, sig_handler);
	signal(SIGILL, sig_handler);
#endif	
    ParodusCfg *cfg;

    /* TODO not ideal, but it fixes a more major problem for now. */
    cfg = get_parodus_cfg();
    memset(cfg,0,sizeof(ParodusCfg));
    
    ParodusInfo("********** Starting component: Parodus **********\n "); 
    setDefaultValuesToCfg(cfg);
    if (0 != parseCommandLine(argc,argv,cfg)) {
		abort();
	}
    curl_global_init(CURL_GLOBAL_DEFAULT);
     
    createSocketConnection( NULL);
    
    return 0;
}

const char *rdk_logger_module_fetch(void)
{
    return "LOG.RDK.PARODUS";
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void sig_handler(int sig)
{

	if ( sig == SIGINT ) 
	{
		signal(SIGINT, sig_handler); /* reset it to this function */
		ParodusInfo("SIGINT received!\n");
		shutdownSocketConnection(SHUTDOWN_REASON_PARODUS_STOP);
	}
	else if ( sig == SIGUSR1 ) 
	{
		signal(SIGUSR1, sig_handler); /* reset it to this function */
		ParodusInfo("SIGUSR1 received!\n");
		shutdownSocketConnection(SHUTDOWN_REASON_SYSTEM_RESTART);
	}
	else if ( sig == SIGUSR2 ) 
	{
		ParodusInfo("SIGUSR2 received!\n");
	}
	else if ( sig == SIGCHLD ) 
	{
		signal(SIGCHLD, sig_handler); /* reset it to this function */
		ParodusInfo("SIGHLD received!\n");
	}
	else if ( sig == SIGPIPE ) 
	{
		signal(SIGPIPE, sig_handler); /* reset it to this function */
		ParodusInfo("SIGPIPE received!\n");
	}
	else if ( sig == SIGALRM ) 
	{
		signal(SIGALRM, sig_handler); /* reset it to this function */
		ParodusInfo("SIGALRM received!\n");
	}
	else 
	{
		ParodusInfo("Signal %d received!\n", sig);
		shutdownSocketConnection(SHUTDOWN_REASON_PARODUS_STOP);
	}
	
}
