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
#include <stdarg.h>

#include <CUnit/Basic.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <assert.h>
#include <nopoll.h>

#include "../src/ParodusInternal.h"
#include "../src/connection.h"
#include "../src/config.h"
#include "../src/token.h"

#define SECURE_WEBPA_URL	"https://127.0.0.1"
#define UNSECURE_WEBPA_URL	"http://127.0.0.1"
#define HOST_IP				"127.0.0.1"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

bool close_retry;
bool LastReasonStatus;
volatile unsigned int heartBeatTimer;
pthread_mutex_t close_mut;
int g_status;
char *g_redirect_url;
char *g_jwt_server_ip;
int mock_strncmp = true;

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
noPollConn * nopoll_conn_new_opts (noPollCtx  * ctx, noPollConnOpts  * opts, const char  * host_ip, const char  * host_port, const char  * host_name,const char  * get_url,const char  * protocols, const char * origin)
{
    UNUSED(host_port); UNUSED(host_name); UNUSED(get_url); UNUSED(protocols); 
    UNUSED(origin); UNUSED(opts);
    
    function_called();
    check_expected((intptr_t)ctx);
    check_expected((intptr_t)host_ip);
    return (noPollConn *) (intptr_t)mock();
}

noPollConn * nopoll_conn_tls_new (noPollCtx  * ctx, noPollConnOpts * options, const char * host_ip, const char * host_port, const char * host_name, const char * get_url, const char * protocols, const char * origin)
{
    UNUSED(options); UNUSED(host_port); UNUSED(host_name); UNUSED(get_url); UNUSED(protocols); 
    UNUSED(origin);
    
    function_called();
    check_expected((intptr_t)ctx);
    check_expected((intptr_t)host_ip);
    return (noPollConn *) (intptr_t)mock();
}

noPollConn * nopoll_conn_tls_new6 (noPollCtx  * ctx, noPollConnOpts * options, const char * host_ip, const char * host_port, const char * host_name, const char * get_url, const char * protocols, const char * origin)
{
    UNUSED(options); UNUSED(host_port); UNUSED(host_name); UNUSED(get_url); UNUSED(protocols); 
    UNUSED(origin);
    
    function_called();
    check_expected((intptr_t)ctx);
    check_expected((intptr_t)host_ip);
    return (noPollConn *) (intptr_t)mock();
}


nopoll_bool nopoll_conn_is_ok (noPollConn * conn)
{
    UNUSED(conn);
    function_called();
    return (nopoll_bool) mock();
}

int getGlobalHttpStatus()
{
	return g_status;
}

void setGlobalHttpStatus(int status)
{
	g_status=status;
}

void setGlobalRedirectUrl (char *redirect_url)
{
	g_redirect_url = redirect_url;
}

nopoll_bool nopoll_conn_wait_for_status_until_connection_ready (noPollConn * conn, int timeout, int *status, char **message)
{
    UNUSED(timeout); UNUSED(message);
    UNUSED(conn);
    *status = getGlobalHttpStatus();
    *message = (char *)malloc(128);

    if (NULL != g_redirect_url)
		parStrncpy (*message, g_redirect_url, 128);
    function_called();
    return (nopoll_bool) mock();
}

void setGlobalJWTUrl (char *jwt_server_ip)
{
	if (NULL != jwt_server_ip)
		g_jwt_server_ip = strdup(jwt_server_ip);
}

int allow_insecure_conn (char *url_buf, int url_buflen,
	char *port_buf, int port_buflen)
{
	UNUSED(url_buflen); UNUSED(port_buf);
    	UNUSED(port_buflen);
    	
    	if (NULL != g_jwt_server_ip)
		parStrncpy (url_buf, g_jwt_server_ip, 128);
		
	function_called ();
	return (int) mock();
}

char* getWebpaConveyHeader()
{
    function_called();
    return (char*) (intptr_t)mock();
}

int checkHostIp(char * serverIP)
{
    (void) serverIP;
    function_called();
    return (int) mock();
}

void getCurrentTime(struct timespec *timer)
{
    (void) timer;
    function_called();
}

long timeValDiff(struct timespec *starttime, struct timespec *finishtime)
{
    (void) starttime; (void) finishtime;
    function_called();
    return (long) mock();
}

int kill(pid_t pid, int sig)
{
    UNUSED(pid); UNUSED(sig);
    function_called();
    return (int) mock();
}

void nopoll_conn_close(noPollConn *conn)	
{
    UNUSED(conn);
    function_called();
}

int nopoll_conn_ref_count(noPollConn * conn)
{
    UNUSED(conn);
    function_called();
    return (int) mock();
}

void nopoll_conn_unref(	noPollConn * conn)	
{
    UNUSED(conn);
    function_called();
}

int standard_strncmp(const char *s1, const char *s2, size_t n)
{
	size_t i;
	for (i=0; i<n; i++) {
		if (s1[i] != s2[i])
			return s1[i] - s2[i];
		if (0 == s1[i])
			return 0;
	}
	return 0;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    if (!mock_strncmp)
		return standard_strncmp (s1, s2, n);
    function_called();
    return (int) mock();
}

char *strtok(char *str, const char *delim)
{
    UNUSED(str); UNUSED(delim); 
    function_called();
    return (char*) (intptr_t)mock();
}

void setMessageHandlers()
{
    function_called();
}
/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

/* When JWT is enabled , connecting with jwt_server_ip */
void test_createSecureConnection()
{
    noPollConn *gNPConn;
    noPollCtx *ctx = nopoll_ctx_new();
    ParodusCfg *cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    memset(cfg, 0, sizeof(ParodusCfg));
    
    mock_strncmp = false;
    cfg->flags = 0;
#ifdef FEATURE_DNS_QUERY
	cfg->acquire_jwt = 1;
#endif
    parStrncpy(cfg->webpa_url , SECURE_WEBPA_URL, sizeof(cfg->webpa_url));
    set_parodus_cfg(cfg);

    assert_non_null(ctx);

    will_return(getWebpaConveyHeader, (intptr_t)"WebPA-1.6 (TG1682)");
    expect_function_call(getWebpaConveyHeader);

#ifdef FEATURE_DNS_QUERY
	setGlobalJWTUrl ("127.0.0.2");
	will_return (allow_insecure_conn, 0);
	expect_function_call (allow_insecure_conn);
#endif
    expect_value(nopoll_conn_tls_new6, (intptr_t)ctx, (intptr_t)ctx);
    
#ifdef FEATURE_DNS_QUERY
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, g_jwt_server_ip);
#else    
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, HOST_IP);
#endif
    
    will_return(nopoll_conn_tls_new6, NULL);
    expect_function_call(nopoll_conn_tls_new6);
    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);
    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);

	expect_value(nopoll_conn_tls_new, (intptr_t)ctx, (intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY	
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, g_jwt_server_ip);
#else  
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, HOST_IP);
#endif
  
    will_return(nopoll_conn_tls_new, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_tls_new);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);

    will_return(nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
    expect_function_call(nopoll_conn_wait_for_status_until_connection_ready);

    expect_function_call(setMessageHandlers);

    int ret = createNopollConnection(ctx);
    assert_int_equal(ret, nopoll_true);
    free(cfg);
    if (g_jwt_server_ip !=NULL)
    {
    	free(g_jwt_server_ip);
    }
    nopoll_ctx_unref (ctx);
}

/* When JWT is enabled , connecting with jwt_server_ip */
void test_createConnection()
{
    noPollConn *gNPConn;
    noPollCtx *ctx = nopoll_ctx_new();
    ParodusCfg *cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    memset(cfg, 0, sizeof(ParodusCfg));
    assert_non_null(cfg);
    
    mock_strncmp = false;
    cfg->flags = 0;
#ifdef FEATURE_DNS_QUERY
	cfg->acquire_jwt = 1;
#endif
    parStrncpy(cfg->webpa_url , UNSECURE_WEBPA_URL, sizeof(cfg->webpa_url));
    set_parodus_cfg(cfg);
    assert_non_null(ctx);

    will_return(getWebpaConveyHeader, (intptr_t)"WebPA-1.6 (TG1682)");
    expect_function_call(getWebpaConveyHeader);

#ifdef FEATURE_DNS_QUERY
	setGlobalJWTUrl ("127.0.0.2");
	will_return (allow_insecure_conn, 1);
	expect_function_call (allow_insecure_conn);
#endif

    expect_value(nopoll_conn_new_opts, (intptr_t)ctx, (intptr_t)ctx);
    
#ifdef FEATURE_DNS_QUERY
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, g_jwt_server_ip);
#else
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, HOST_IP);
#endif

    will_return(nopoll_conn_new_opts, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_new_opts);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);

    will_return(nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
    expect_function_call(nopoll_conn_wait_for_status_until_connection_ready);

    expect_function_call(setMessageHandlers);

    int ret = createNopollConnection(ctx);
    assert_int_equal(ret, nopoll_true);
    assert_string_equal(get_parodus_cfg()->cloud_status, "online");
    free(cfg);
    if (g_jwt_server_ip !=NULL)
    {
    	free(g_jwt_server_ip);
    }
    nopoll_ctx_unref (ctx);
}

/* When JWT is enabled , connecting with jwt_server_ip */ 
void test_createConnectionConnNull()
{
    noPollConn *gNPConn;
    noPollCtx *ctx = nopoll_ctx_new();
    ParodusCfg *cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    memset(cfg, 0, sizeof(ParodusCfg));
    
    mock_strncmp = false;
    cfg->flags = 0;
    cfg->webpa_backoff_max = 2;
#ifdef FEATURE_DNS_QUERY
	cfg->acquire_jwt = 1;
#endif
    parStrncpy(cfg->webpa_url , SECURE_WEBPA_URL,sizeof(cfg->webpa_url));
    set_parodus_cfg(cfg);
    
    assert_non_null(ctx);

    will_return(getWebpaConveyHeader, (intptr_t)"");
    expect_function_call(getWebpaConveyHeader);

#ifdef FEATURE_DNS_QUERY
        setGlobalJWTUrl ("127.0.0.2");
	will_return (allow_insecure_conn, -2);
	expect_function_call (allow_insecure_conn);
#endif

    expect_value(nopoll_conn_tls_new6, (intptr_t)ctx, (intptr_t)ctx);

#ifdef FEATURE_DNS_QUERY   
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, g_jwt_server_ip);
#else
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, HOST_IP);
#endif

    will_return(nopoll_conn_tls_new6, NULL);
    expect_function_call(nopoll_conn_tls_new6);

    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);
    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);

    expect_value(nopoll_conn_tls_new, (intptr_t)ctx, (intptr_t)ctx);

#ifdef FEATURE_DNS_QUERY   
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, g_jwt_server_ip);
#else
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, HOST_IP);
#endif    

    will_return(nopoll_conn_tls_new, (intptr_t)NULL);
    expect_function_call(nopoll_conn_tls_new);

    will_return(checkHostIp, -2);
    expect_function_call(checkHostIp);

    expect_function_call(getCurrentTime);

#ifdef FEATURE_DNS_QUERY
        setGlobalJWTUrl ("127.0.0.2");
	will_return (allow_insecure_conn, TOKEN_ERR_QUERY_DNS_FAIL);
	expect_function_call (allow_insecure_conn);
#endif

	expect_value(nopoll_conn_tls_new6, (intptr_t)ctx, (intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, g_jwt_server_ip);
#else
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, HOST_IP);
#endif

    will_return(nopoll_conn_tls_new6, NULL);
    expect_function_call(nopoll_conn_tls_new6);

    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);
    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);
    

    expect_value(nopoll_conn_tls_new, (intptr_t)ctx, (intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY    
    expect_string(nopoll_conn_tls_new,(intptr_t)host_ip, g_jwt_server_ip);
#else    
    expect_string(nopoll_conn_tls_new,(intptr_t)host_ip, HOST_IP);
#endif
    
    will_return(nopoll_conn_tls_new, (intptr_t)NULL);
    expect_function_call(nopoll_conn_tls_new);

    will_return(checkHostIp, -2);
    expect_function_call(checkHostIp);

    expect_function_call(getCurrentTime);

    will_return(timeValDiff, 15*60*1000);
    expect_function_call(timeValDiff);

    will_return(timeValDiff, 15*60*1000);
    expect_function_call(timeValDiff);

    will_return(kill, 1);
    expect_function_call(kill);

#ifdef FEATURE_DNS_QUERY
        setGlobalJWTUrl ("127.0.0.2");
	will_return (allow_insecure_conn, 0);
	expect_function_call (allow_insecure_conn);
#endif

    expect_value(nopoll_conn_tls_new6, (intptr_t)ctx, (intptr_t)ctx);
    
#ifdef FEATURE_DNS_QUERY     
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, g_jwt_server_ip);
#else
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, HOST_IP);
#endif
    will_return(nopoll_conn_tls_new6, NULL);
    expect_function_call(nopoll_conn_tls_new6);
    
    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);
    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);

    expect_value(nopoll_conn_tls_new, (intptr_t)ctx, (intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, g_jwt_server_ip);
#else    
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, HOST_IP);
#endif    
    
    will_return(nopoll_conn_tls_new, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_tls_new);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);

    will_return(nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
    expect_function_call(nopoll_conn_wait_for_status_until_connection_ready);

    expect_function_call(setMessageHandlers);

    createNopollConnection(ctx);
    free(cfg);
    if (g_jwt_server_ip !=NULL)
    {
    	free(g_jwt_server_ip);
    }
    nopoll_ctx_unref (ctx);
}

/* When JWT is enabled & unable to get jwt_server_url, connecting with config server Ip */
void test_createConnNull_JWT_NULL()
{
    noPollConn *gNPConn;
    noPollCtx *ctx = nopoll_ctx_new();
    ParodusCfg *cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    memset(cfg, 0, sizeof(ParodusCfg));
    
    mock_strncmp = false;
    cfg->flags = 0;
    cfg->webpa_backoff_max = 2;
#ifdef FEATURE_DNS_QUERY
	cfg->acquire_jwt = 1;
#endif
    parStrncpy(cfg->webpa_url , SECURE_WEBPA_URL,sizeof(cfg->webpa_url));
    set_parodus_cfg(cfg);
    
    assert_non_null(ctx);

    will_return(getWebpaConveyHeader, (intptr_t)"");
    expect_function_call(getWebpaConveyHeader);

#ifdef FEATURE_DNS_QUERY
        setGlobalJWTUrl ("");
	will_return (allow_insecure_conn, TOKEN_ERR_MEMORY_FAIL);
	expect_function_call (allow_insecure_conn);
#endif

    expect_value(nopoll_conn_tls_new6, (intptr_t)ctx, (intptr_t)ctx);

#ifdef FEATURE_DNS_QUERY   
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, "");
#else
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, HOST_IP);
#endif

    will_return(nopoll_conn_tls_new6, NULL);
    expect_function_call(nopoll_conn_tls_new6);
    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);
    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);
    
    expect_value(nopoll_conn_tls_new, (intptr_t)ctx, (intptr_t)ctx);

#ifdef FEATURE_DNS_QUERY   
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, "");
#else
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, HOST_IP);
#endif    
    
    will_return(nopoll_conn_tls_new, (intptr_t)NULL);
    expect_function_call(nopoll_conn_tls_new);

    will_return(checkHostIp, -2);
    expect_function_call(checkHostIp);

    expect_function_call(getCurrentTime);

	expect_value(nopoll_conn_tls_new6, (intptr_t)ctx, (intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, HOST_IP);
#else
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, HOST_IP);
#endif

    will_return(nopoll_conn_tls_new6, NULL);
    expect_function_call(nopoll_conn_tls_new6);
    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);
    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);
    

    expect_value(nopoll_conn_tls_new, (intptr_t)ctx, (intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY    
    expect_string(nopoll_conn_tls_new,(intptr_t)host_ip, HOST_IP);
#else    
    expect_string(nopoll_conn_tls_new,(intptr_t)host_ip, HOST_IP);
#endif
    
    will_return(nopoll_conn_tls_new, (intptr_t)NULL);
    expect_function_call(nopoll_conn_tls_new);

    will_return(checkHostIp, -2);
    expect_function_call(checkHostIp);

    expect_function_call(getCurrentTime);

    will_return(timeValDiff, 15*60*1000);
    expect_function_call(timeValDiff);

    will_return(timeValDiff, 15*60*1000);
    expect_function_call(timeValDiff);

    will_return(kill, 1);
    expect_function_call(kill);

    expect_value(nopoll_conn_tls_new6, (intptr_t)ctx, (intptr_t)ctx);
    
#ifdef FEATURE_DNS_QUERY     
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, HOST_IP);
#else
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, HOST_IP);
#endif
    will_return(nopoll_conn_tls_new6, NULL);
    expect_function_call(nopoll_conn_tls_new6);
    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);
    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);
    
    expect_value(nopoll_conn_tls_new, (intptr_t)ctx, (intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, HOST_IP);
#else    
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, HOST_IP);
#endif    
    
    will_return(nopoll_conn_tls_new, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_tls_new);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);

    will_return(nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
    expect_function_call(nopoll_conn_wait_for_status_until_connection_ready);

    expect_function_call(setMessageHandlers);

    createNopollConnection(ctx);
    free(cfg);
    if (g_jwt_server_ip !=NULL)
    {
    	free(g_jwt_server_ip);
    }
    nopoll_ctx_unref (ctx);
}

/* When JWT is enabled , connecting with jwt_server_ip */ 
void test_createConnectionConnNotOk()
{
    noPollConn *gNPConn;
    noPollCtx *ctx = nopoll_ctx_new();
    ParodusCfg *cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    memset(cfg, 0, sizeof(ParodusCfg));
    assert_non_null(cfg);
    
    mock_strncmp = false;
    cfg->flags = 0;
#ifdef FEATURE_DNS_QUERY
	cfg->acquire_jwt = 1;
#endif
    parStrncpy(cfg->webpa_url , UNSECURE_WEBPA_URL, sizeof(cfg->webpa_url));
    set_parodus_cfg(cfg);
    assert_non_null(ctx);

    will_return(getWebpaConveyHeader, (intptr_t)"WebPA-1.6 (TG1682)");
    expect_function_call(getWebpaConveyHeader);

#ifdef FEATURE_DNS_QUERY
	setGlobalJWTUrl ("127.0.0.2");
	will_return (allow_insecure_conn, 1);
	expect_function_call (allow_insecure_conn);
#endif

    expect_value(nopoll_conn_new_opts, (intptr_t)ctx, (intptr_t)ctx);

#ifdef FEATURE_DNS_QUERY    
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, g_jwt_server_ip);
#else    
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, HOST_IP);
#endif
    will_return(nopoll_conn_new_opts, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_new_opts);

    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);

    expect_function_call(nopoll_conn_close);

    will_return(nopoll_conn_ref_count, 1);
    expect_function_call(nopoll_conn_ref_count);

    expect_function_call(nopoll_conn_unref);

    expect_value(nopoll_conn_new_opts, (intptr_t)ctx, (intptr_t)ctx);

#ifdef FEATURE_DNS_QUERY    
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, g_jwt_server_ip);
#else    
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, HOST_IP);
#endif
    will_return(nopoll_conn_new_opts, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_new_opts);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);
	setGlobalHttpStatus(0);

    will_return(nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
    expect_function_call(nopoll_conn_wait_for_status_until_connection_ready);

    expect_function_call(nopoll_conn_close);

    will_return(nopoll_conn_ref_count, 0);
    expect_function_call(nopoll_conn_ref_count);

    expect_value(nopoll_conn_new_opts, (intptr_t)ctx, (intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY    
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, g_jwt_server_ip);
#else    
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, HOST_IP);
#endif   
    
    will_return(nopoll_conn_new_opts, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_new_opts);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);
	
    will_return(nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
    expect_function_call(nopoll_conn_wait_for_status_until_connection_ready);

    expect_function_call(setMessageHandlers);

    int ret = createNopollConnection(ctx);
    assert_int_equal(ret, nopoll_true);
    free(cfg);
    if (g_jwt_server_ip !=NULL)
    {
    	free(g_jwt_server_ip);
    }
    nopoll_ctx_unref (ctx);
}

/* When JWT is enabled & unable to get jwt_server_url, connecting with config server Ip */
void test_createConnNotOk_JWT_NULL()
{
    noPollConn *gNPConn;
    noPollCtx *ctx = nopoll_ctx_new();
    ParodusCfg *cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    memset(cfg, 0, sizeof(ParodusCfg));
    assert_non_null(cfg);
    
    mock_strncmp = false;
    cfg->flags = 0;
#ifdef FEATURE_DNS_QUERY
	cfg->acquire_jwt = 1;
#endif
    parStrncpy(cfg->webpa_url , UNSECURE_WEBPA_URL, sizeof(cfg->webpa_url));
    set_parodus_cfg(cfg);
    assert_non_null(ctx);

    will_return(getWebpaConveyHeader, (intptr_t)"WebPA-1.6 (TG1682)");
    expect_function_call(getWebpaConveyHeader);

#ifdef FEATURE_DNS_QUERY
	setGlobalJWTUrl ("");
	will_return (allow_insecure_conn, 1);
	expect_function_call (allow_insecure_conn);
#endif

    expect_value(nopoll_conn_new_opts, (intptr_t)ctx, (intptr_t)ctx);

#ifdef FEATURE_DNS_QUERY    
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, "");
#else    
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, HOST_IP);
#endif
    will_return(nopoll_conn_new_opts, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_new_opts);

    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);

    expect_function_call(nopoll_conn_close);

    will_return(nopoll_conn_ref_count, 1);
    expect_function_call(nopoll_conn_ref_count);

    expect_function_call(nopoll_conn_unref);

    expect_value(nopoll_conn_new_opts, (intptr_t)ctx, (intptr_t)ctx);

#ifdef FEATURE_DNS_QUERY    
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, HOST_IP);
#else    
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, HOST_IP);
#endif
    will_return(nopoll_conn_new_opts, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_new_opts);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);
	setGlobalHttpStatus(0);

    will_return(nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
    expect_function_call(nopoll_conn_wait_for_status_until_connection_ready);

    expect_function_call(nopoll_conn_close);

    will_return(nopoll_conn_ref_count, 0);
    expect_function_call(nopoll_conn_ref_count);

    expect_value(nopoll_conn_new_opts, (intptr_t)ctx, (intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY    
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, HOST_IP);
#else    
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, HOST_IP);
#endif   
    
    will_return(nopoll_conn_new_opts, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_new_opts);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);
	
    will_return(nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
    expect_function_call(nopoll_conn_wait_for_status_until_connection_ready);

    expect_function_call(setMessageHandlers);

    int ret = createNopollConnection(ctx);
    assert_int_equal(ret, nopoll_true);
    free(cfg);
    if (g_jwt_server_ip !=NULL)
    {
    	free(g_jwt_server_ip);
    }
    nopoll_ctx_unref (ctx);
}

/* When JWT is enabled , connecting with jwt_server_ip */ 
void test_createConnectionConnRedirect()
{
    noPollConn *gNPConn;
    noPollCtx *ctx = nopoll_ctx_new();
    ParodusCfg *cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    memset(cfg, 0, sizeof(ParodusCfg));
    assert_non_null(cfg);
    
    mock_strncmp = false;
    cfg->flags = 0;
#ifdef FEATURE_DNS_QUERY
	cfg->acquire_jwt = 1;
#endif
    parStrncpy(cfg->webpa_url , UNSECURE_WEBPA_URL, sizeof(cfg->webpa_url));
    set_parodus_cfg(cfg);
    assert_non_null(ctx);

    will_return(getWebpaConveyHeader, (intptr_t)"WebPA-1.6 (TG1682)");
    expect_function_call(getWebpaConveyHeader);

#ifdef FEATURE_DNS_QUERY
        setGlobalJWTUrl ("127.0.0.2");
	will_return (allow_insecure_conn, 1);
	expect_function_call (allow_insecure_conn);
#endif

    expect_value(nopoll_conn_new_opts, (intptr_t)ctx, (intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, g_jwt_server_ip);
#else    
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, HOST_IP);
#endif    
    
    will_return(nopoll_conn_new_opts, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_new_opts);

    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);

    expect_function_call(nopoll_conn_close);

    will_return(nopoll_conn_ref_count, 1);
    expect_function_call(nopoll_conn_ref_count);

    expect_function_call(nopoll_conn_unref);
	
    expect_value(nopoll_conn_new_opts, (intptr_t)ctx, (intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, g_jwt_server_ip);
#else   
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, HOST_IP);
#endif   
    
    will_return(nopoll_conn_new_opts, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_new_opts);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);
	setGlobalHttpStatus(307);
    setGlobalRedirectUrl ("Redirect:http://10.0.0.12");
	
    will_return(nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
    expect_function_call(nopoll_conn_wait_for_status_until_connection_ready);

    expect_function_call(nopoll_conn_close);

    will_return(nopoll_conn_ref_count, 1);
    expect_function_call(nopoll_conn_ref_count);

    expect_function_call(nopoll_conn_unref);
    
    expect_value(nopoll_conn_new_opts, (intptr_t)ctx, (intptr_t)ctx);
    expect_string(nopoll_conn_new_opts, (intptr_t)host_ip, "10.0.0.12");
    will_return(nopoll_conn_new_opts, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_new_opts);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);

    will_return(nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
    expect_function_call(nopoll_conn_wait_for_status_until_connection_ready);

    expect_function_call(setMessageHandlers);

    int ret = createNopollConnection(ctx);
    assert_int_equal(ret, nopoll_true);
    free(cfg);
	if (g_jwt_server_ip !=NULL)
	{
		free(g_jwt_server_ip);
	}
    nopoll_ctx_unref (ctx);
}

void test_createIPv4Connection()
{
    noPollConn *gNPConn;
    noPollCtx *ctx = nopoll_ctx_new();
    ParodusCfg *cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    memset(cfg, 0, sizeof(ParodusCfg));

    mock_strncmp = false;
    cfg->flags = 2;
#ifdef FEATURE_DNS_QUERY
	cfg->acquire_jwt = 1;
#endif
    parStrncpy(cfg->webpa_url , SECURE_WEBPA_URL, sizeof(cfg->webpa_url));
    set_parodus_cfg(cfg);

    assert_non_null(ctx);

    will_return(getWebpaConveyHeader, (intptr_t)"WebPA-1.6 (TG1682)");
    expect_function_call(getWebpaConveyHeader);

#ifdef FEATURE_DNS_QUERY
	setGlobalJWTUrl ("127.0.0.2");
	will_return (allow_insecure_conn, 0);
	expect_function_call (allow_insecure_conn);
#endif

	expect_value(nopoll_conn_tls_new, (intptr_t)ctx, (intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, g_jwt_server_ip);
#else
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, HOST_IP);
#endif
    will_return(nopoll_conn_tls_new, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_tls_new);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);

    will_return(nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
    expect_function_call(nopoll_conn_wait_for_status_until_connection_ready);

    expect_function_call(setMessageHandlers);

    int ret = createNopollConnection(ctx);
    assert_int_equal(ret, nopoll_true);
    free(cfg);
    nopoll_ctx_unref (ctx);
}

void test_createIPv6Connection()
{
    noPollConn *gNPConn;
    noPollCtx *ctx = nopoll_ctx_new();
    ParodusCfg *cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    memset(cfg, 0, sizeof(ParodusCfg));

    mock_strncmp = false;
    cfg->flags = 1;
#ifdef FEATURE_DNS_QUERY
	cfg->acquire_jwt = 1;
#endif
    parStrncpy(cfg->webpa_url , SECURE_WEBPA_URL, sizeof(cfg->webpa_url));
    set_parodus_cfg(cfg);

    assert_non_null(ctx);

    will_return(getWebpaConveyHeader, (intptr_t)"WebPA-1.6 (TG1682)");
    expect_function_call(getWebpaConveyHeader);

#ifdef FEATURE_DNS_QUERY
	setGlobalJWTUrl ("127.0.0.2");
	will_return (allow_insecure_conn, 0);
	expect_function_call (allow_insecure_conn);
#endif

	expect_value(nopoll_conn_tls_new6, (intptr_t)ctx, (intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, g_jwt_server_ip);
#else
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, HOST_IP);
#endif

    will_return(nopoll_conn_tls_new6, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_tls_new6);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);

    will_return(nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
    expect_function_call(nopoll_conn_wait_for_status_until_connection_ready);

    expect_function_call(setMessageHandlers);

    int ret = createNopollConnection(ctx);
    assert_int_equal(ret, nopoll_true);
    free(cfg);
    nopoll_ctx_unref (ctx);
}


void test_createIPv6toIPv4Connection()
{
    noPollConn *gNPConn;
    noPollCtx *ctx = nopoll_ctx_new();
    ParodusCfg *cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    memset(cfg, 0, sizeof(ParodusCfg));

    mock_strncmp = false;
	cfg->flags = 0;
#ifdef FEATURE_DNS_QUERY
	cfg->acquire_jwt = 1;
#endif
    parStrncpy(cfg->webpa_url , SECURE_WEBPA_URL, sizeof(cfg->webpa_url));
    set_parodus_cfg(cfg);

    assert_non_null(ctx);

    will_return(getWebpaConveyHeader, (intptr_t)"WebPA-1.6 (TG1682)");
    expect_function_call(getWebpaConveyHeader);

#ifdef FEATURE_DNS_QUERY
	setGlobalJWTUrl ("127.0.0.2");
	will_return (allow_insecure_conn, 0);
	expect_function_call (allow_insecure_conn);
#endif

	expect_value(nopoll_conn_tls_new6, (intptr_t)ctx, (intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, g_jwt_server_ip);
#else
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, HOST_IP);
#endif

    will_return(nopoll_conn_tls_new6, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_tls_new6);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);

    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);

    expect_function_call(nopoll_conn_close);

    will_return(nopoll_conn_ref_count, 1);
    expect_function_call(nopoll_conn_ref_count);

    expect_function_call(nopoll_conn_unref);

    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);

    expect_value(nopoll_conn_tls_new,(intptr_t)ctx,(intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, g_jwt_server_ip);
#else
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, HOST_IP);
#endif

    will_return(nopoll_conn_tls_new, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_tls_new);

    will_return(nopoll_conn_is_ok,nopoll_true);
    expect_function_call(nopoll_conn_is_ok);

    will_return(nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
    expect_function_call(nopoll_conn_wait_for_status_until_connection_ready);

    expect_function_call(setMessageHandlers);

    int ret = createNopollConnection(ctx);
    assert_int_equal(ret, nopoll_true);
    free(cfg);
    nopoll_ctx_unref (ctx);
}

void test_createFallbackRedirectionConn()
{
    noPollConn *gNPConn;
    noPollCtx *ctx = nopoll_ctx_new();
    ParodusCfg *cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    memset(cfg, 0, sizeof(ParodusCfg));

    mock_strncmp = false;
	cfg->flags = 0;
#ifdef FEATURE_DNS_QUERY
	cfg->acquire_jwt = 1;
#endif
    parStrncpy(cfg->webpa_url , SECURE_WEBPA_URL, sizeof(cfg->webpa_url));
    set_parodus_cfg(cfg);

    assert_non_null(ctx);

    will_return(getWebpaConveyHeader, (intptr_t)"WebPA-1.6 (TG1682)");
    expect_function_call(getWebpaConveyHeader);

#ifdef FEATURE_DNS_QUERY
	setGlobalJWTUrl ("127.0.0.2");
	will_return (allow_insecure_conn, 0);
	expect_function_call (allow_insecure_conn);
#endif

	expect_value(nopoll_conn_tls_new6, (intptr_t)ctx, (intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, g_jwt_server_ip);
#else
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, HOST_IP);
#endif

    will_return(nopoll_conn_tls_new6, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_tls_new6);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);

    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);

    expect_function_call(nopoll_conn_close);

    will_return(nopoll_conn_ref_count, 1);
    expect_function_call(nopoll_conn_ref_count);

    expect_function_call(nopoll_conn_unref);

    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);

    expect_value(nopoll_conn_tls_new,(intptr_t)ctx,(intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, g_jwt_server_ip);
#else
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, HOST_IP);
#endif

    will_return(nopoll_conn_tls_new, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_tls_new);

    will_return(nopoll_conn_is_ok,nopoll_true);
    expect_function_call(nopoll_conn_is_ok);

    setGlobalHttpStatus(307);
    setGlobalRedirectUrl ("Redirect:https://10.0.0.25");

    will_return(nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
    expect_function_call(nopoll_conn_wait_for_status_until_connection_ready);

    expect_function_call(nopoll_conn_close);

    will_return(nopoll_conn_ref_count, 1);
    expect_function_call(nopoll_conn_ref_count);

    expect_function_call(nopoll_conn_unref);

    will_return(nopoll_conn_is_ok, nopoll_false);
    expect_function_call(nopoll_conn_is_ok);

    expect_value(nopoll_conn_tls_new, (intptr_t)ctx, (intptr_t)ctx);
    expect_string(nopoll_conn_tls_new, (intptr_t)host_ip, "10.0.0.25");

    will_return(nopoll_conn_tls_new, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_tls_new);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);

    will_return(nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
    expect_function_call(nopoll_conn_wait_for_status_until_connection_ready);

    expect_function_call(setMessageHandlers);

    int ret = createNopollConnection(ctx);
    assert_int_equal(ret, nopoll_true);
    free(cfg);
    nopoll_ctx_unref (ctx);
}

void test_createIPv6FallbackRedirectConn()
{
    noPollConn *gNPConn;
    noPollCtx *ctx = nopoll_ctx_new();
    ParodusCfg *cfg = (ParodusCfg*)malloc(sizeof(ParodusCfg));
    memset(cfg, 0, sizeof(ParodusCfg));

    mock_strncmp = false;
	cfg->flags = 0;
#ifdef FEATURE_DNS_QUERY
	cfg->acquire_jwt = 1;
#endif
    parStrncpy(cfg->webpa_url , SECURE_WEBPA_URL, sizeof(cfg->webpa_url));
    set_parodus_cfg(cfg);

    assert_non_null(ctx);

    will_return(getWebpaConveyHeader, (intptr_t)"WebPA-1.6 (TG1682)");
    expect_function_call(getWebpaConveyHeader);

#ifdef FEATURE_DNS_QUERY
	setGlobalJWTUrl ("127.0.0.2");
	will_return (allow_insecure_conn, 0);
	expect_function_call (allow_insecure_conn);
#endif

	expect_value(nopoll_conn_tls_new6, (intptr_t)ctx, (intptr_t)ctx);
#ifdef FEATURE_DNS_QUERY
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, g_jwt_server_ip);
#else
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, HOST_IP);
#endif

    will_return(nopoll_conn_tls_new6, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_tls_new6);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);

    setGlobalHttpStatus(307);
    setGlobalRedirectUrl ("Redirect:https://10.0.0.50");

	will_return(nopoll_conn_wait_for_status_until_connection_ready, nopoll_false);
    expect_function_call(nopoll_conn_wait_for_status_until_connection_ready);

    expect_function_call(nopoll_conn_close);

    will_return(nopoll_conn_ref_count, 1);
    expect_function_call(nopoll_conn_ref_count);

    expect_function_call(nopoll_conn_unref);

    expect_value(nopoll_conn_tls_new6, (intptr_t)ctx, (intptr_t)ctx);
    expect_string(nopoll_conn_tls_new6, (intptr_t)host_ip, "10.0.0.50");

    will_return(nopoll_conn_tls_new6, (intptr_t)&gNPConn);
    expect_function_call(nopoll_conn_tls_new6);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);

    will_return(nopoll_conn_is_ok, nopoll_true);
    expect_function_call(nopoll_conn_is_ok);

    will_return(nopoll_conn_wait_for_status_until_connection_ready, nopoll_true);
    expect_function_call(nopoll_conn_wait_for_status_until_connection_ready);

    expect_function_call(setMessageHandlers);

    int ret = createNopollConnection(ctx);
    assert_int_equal(ret, nopoll_true);
    free(cfg);
    nopoll_ctx_unref (ctx);
}

void err_createConnectionCtxNull()
{
    noPollCtx *ctx = NULL;
    assert_null(ctx);
    int ret = createNopollConnection(ctx);
    assert_int_equal(ret, nopoll_false);
}

void test_standard_strncmp ()
{
	assert_int_equal (standard_strncmp ("abcde", "abcde", 100), 0);
	assert_true (standard_strncmp ("abcde", "abcdf", 100) < 0);
	assert_true (standard_strncmp ("abcd", "abcdf", 100) < 0);
	assert_true (standard_strncmp ("abcdf", "abcde", 100) > 0);
	assert_true (standard_strncmp ("abcde", "abcd", 100) > 0);
	assert_int_equal (standard_strncmp ("abcde", "abcff", 3), 0);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_standard_strncmp),
        cmocka_unit_test(test_createSecureConnection),
        cmocka_unit_test(test_createConnection),
        cmocka_unit_test(test_createConnectionConnNull),
        cmocka_unit_test(test_createConnectionConnNotOk),
        cmocka_unit_test(test_createConnectionConnRedirect),
        cmocka_unit_test(err_createConnectionCtxNull),
        cmocka_unit_test(test_createConnNotOk_JWT_NULL),
	cmocka_unit_test(test_createConnNull_JWT_NULL),
        cmocka_unit_test(test_createIPv4Connection),
        cmocka_unit_test(test_createIPv6Connection),
        cmocka_unit_test(test_createIPv6toIPv4Connection),
        cmocka_unit_test(test_createFallbackRedirectionConn),
        cmocka_unit_test(test_createIPv6FallbackRedirectConn),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
