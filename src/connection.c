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
 * @file connection.c
 *
 * @description This decribes functions required to manage WebSocket client connections.
 *
 */
 
#include "connection.h"
#include "time.h"
#include "token.h"
#include "config.h"
#include "nopoll_helpers.h"
#include "mutex.h"
#include "spin_thread.h"
#include "ParodusInternal.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define HTTP_CUSTOM_HEADER_COUNT                    	5
#define INITIAL_CJWT_RETRY                    	-2

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

char deviceMAC[32]={'\0'};
static char *reconnect_reason = "webpa_process_starts";
static noPollConn *g_conn = NULL;
static bool LastReasonStatus = false;
static noPollConnOpts * createConnOpts (char * extra_headers, bool secure);
static char* build_extra_headers( const char *auth, const char *device_id,
                                  const char *user_agent, const char *convey );

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
noPollConn *get_global_conn(void)
{
    return g_conn;
}

void set_global_conn(noPollConn *conn)
{
    g_conn = conn;
}

char *get_global_reconnect_reason()
{
    return reconnect_reason;
}

void set_global_reconnect_reason(char *reason)
{
    reconnect_reason = reason;
}

bool get_global_reconnect_status()
{
    return LastReasonStatus;
}

void set_global_reconnect_status(bool status)
{
    LastReasonStatus = status;
}

//--------------------------------------------------------------------
// createNopollConnection_logic:

// call stack:

//  createNopollConnection
//    find_servers
//    keep_trying_connect     // keep trying till success or need to requery dns
//      connect_and_wait      // tries both ipv6 and ipv4, if necessary
//        nopoll_connect
//        wait_connection_ready


//--------------------------------------------------------------------
/*  Defined in ParodusInternal.h
#define SERVER_ADDR_LEN 256
#define PORT_LEN 8

typedef struct {
  int allow_insecure;
  char server_addr[SERVER_ADDR_LEN];
  char port[PORT_LEN];
} server_t;

typedef struct {
  server_t defaults;	// from command line
  server_t jwt;		// from jwt endpoint claim
  server_t redirect;	// from redirect response to
			//  nopoll_conn_wait_until_connection_ready
} server_list_t;
*/

void set_server_null (server_t *server)
{
  server->server_addr[0] = 0;
}

void set_server_list_null (server_list_t *server_list)
{
  set_server_null (&server_list->defaults);
  set_server_null (&server_list->jwt);
  set_server_null (&server_list->redirect);
}

int server_is_null (server_t *server)
{
  return (0 == server->server_addr[0]);
}

// If there's a redirect server, that's it,
// else if there's a jwt server that's it,
// else it's the default server

server_t *get_current_server (server_list_t *server_list)
{
  if (!server_is_null (&server_list->redirect))
    return &server_list->redirect;
  if (!server_is_null (&server_list->jwt))
    return &server_list->jwt;
  return &server_list->defaults;
}

  
int parse_server_url (const char *full_url, server_t *server)
{
  server->allow_insecure = parse_webpa_url (full_url,
	server->server_addr, SERVER_ADDR_LEN, server->port, PORT_LEN);
  return server->allow_insecure;
}

//--------------------------------------------------------------------
typedef struct {
  int running;
  struct timespec start_time;
  struct timespec end_time;
} expire_timer_t;

static void init_expire_timer (expire_timer_t *timer)
{
  timer->running = false;
}

static int check_timer_expired (expire_timer_t *timer, long timeout)
{
  long time_diff;
  
  if (!timer->running) {
    getCurrentTime(&timer->start_time);
    timer->running = true;
    ParodusInfo("First connect error occurred, initialized the connect error timer\n");
    return false;
  }
  
  getCurrentTime(&timer->end_time);
  time_diff = timeValDiff (&timer->start_time, &timer->end_time);
  ParodusPrint("checking timeout difference:%ld\n", time_diff);
  if(time_diff >= timeout)
    return true;
  return false;
}

//--------------------------------------------------------------------
typedef struct {
  int max_delay;
  int delay;
} backoff_timer_t;

static void init_backoff_timer (backoff_timer_t *timer, int max_delay)
{
  timer->max_delay = max_delay;
  timer->delay = 1;
}

static void backoff_delay (backoff_timer_t *timer)
{
  if (timer->delay < timer->max_delay)
    timer->delay = timer->delay + timer->delay + 1;
    // 3,7,15,31 ..
  if (timer->delay > timer->max_delay)
    timer->delay = timer->max_delay;
  ParodusInfo("Waiting with backoffRetryTime %d seconds\n", timer->delay);
  sleep (timer->delay);
}  

//--------------------------------------------------------------------
typedef struct {
  char *conveyHeader;
  char device_id[32];
  char user_agent[512];
} header_info_t;

static void init_header_info (header_info_t *header_info)
{
  char deviceMAC[32]={'\0'};

  header_info->conveyHeader = NULL;
  memset (header_info->device_id, 0, sizeof(header_info->device_id));
  memset (header_info->user_agent, 0, sizeof(header_info->user_agent));
  
  snprintf(header_info->user_agent, sizeof(header_info->user_agent),"%s (%s; %s/%s;)",
     ((0 != strlen(get_parodus_cfg()->webpa_protocol)) ? get_parodus_cfg()->webpa_protocol : "unknown"),
     ((0 != strlen(get_parodus_cfg()->fw_name)) ? get_parodus_cfg()->fw_name : "unknown"),
     ((0 != strlen(get_parodus_cfg()->hw_model)) ? get_parodus_cfg()->hw_model : "unknown"),
     ((0 != strlen(get_parodus_cfg()->hw_manufacturer)) ? get_parodus_cfg()->hw_manufacturer : "unknown"));

  ParodusInfo("User-Agent: %s\n",header_info->user_agent);
  header_info->conveyHeader = getWebpaConveyHeader();
  parStrncpy(deviceMAC, get_parodus_cfg()->hw_mac,sizeof(deviceMAC));
  snprintf(header_info->device_id, sizeof(header_info->device_id), "mac:%s", deviceMAC);
  ParodusInfo("Device_id %s\n", header_info->device_id);
  
}

static char *build_extra_hdrs (header_info_t *header_info)
// result must be freed
{
  char *auth_token = get_parodus_cfg()->webpa_auth_token;
  return build_extra_headers( (0 < strlen(auth_token) ? auth_token : NULL),
	header_info->device_id, header_info->user_agent, header_info->conveyHeader );
}


//--------------------------------------------------------------------
// connection context which is defined in createNopollConnection
// and passed into functions keep_retrying_connect, connect_and_wait,
// wait_connection_ready, and nopoll_connect 
typedef struct {
  noPollCtx *nopoll_ctx;
  server_list_t server_list;
  server_t *current_server;
  header_info_t header_info;
  char *extra_headers;		// need to be freed ?
  expire_timer_t connect_timer;
} create_connection_ctx_t;

static void set_current_server (create_connection_ctx_t *ctx)
{
  ctx->current_server = get_current_server (&ctx->server_list);
}

static void set_extra_headers (create_connection_ctx_t *ctx, int reauthorize)
{
  if (reauthorize && (strlen(get_parodus_cfg()->token_acquisition_script) >0))
  {
    createNewAuthToken(get_parodus_cfg()->webpa_auth_token,
      sizeof(get_parodus_cfg()->webpa_auth_token));
  }
  
  ctx->extra_headers = build_extra_hdrs (&ctx->header_info);
}

static void free_extra_headers (create_connection_ctx_t *ctx)
{
  if (NULL != ctx->extra_headers) {
    free (ctx->extra_headers);
    ctx->extra_headers = NULL;
  }
}


//--------------------------------------------------------------------
// find_servers:
// get and parse default server
// if necessary, query dns and parse server from jwt
// populate server_list
// return:

#define FIND_SUCCESS 0
#define FIND_INVALID_DEFAULT -2
#define FIND_JWT_FAIL -1

static int find_servers (server_list_t *server_list)
{
  server_t *default_server = &server_list->defaults;
  set_server_list_null (server_list);
  // parse default server URL
  if (parse_server_url (get_parodus_cfg()->webpa_url, default_server) < 0)
     return FIND_INVALID_DEFAULT;	// must have valid default url
  ParodusInfo("default server_Address %s\n", default_server->server_addr);
  ParodusInfo("default port %s\n", default_server->port);
#ifdef FEATURE_DNS_QUERY
  if (get_parodus_cfg()->acquire_jwt) {
    server_t *jwt_server = &server_list->jwt;
    //query dns and validate JWT
    jwt_server->allow_insecure = allow_insecure_conn(
             jwt_server->server_addr, SERVER_ADDR_LEN,
             jwt_server->port, PORT_LEN);
    if (jwt_server->allow_insecure < 0) {
      set_server_null (jwt_server);
      return FIND_JWT_FAIL;
    }
    ParodusInfo("JWT ON: jwt_server_url stored as %s\n", jwt_server->server_addr);
  }
#endif
  return 0;
}


//--------------------------------------------------------------------
// connect to current server
static int nopoll_connect (create_connection_ctx_t *ctx, int is_ipv6)
{
   noPollCtx *nopoll_ctx = ctx->nopoll_ctx;
   server_t *server = ctx->current_server;
   noPollConn *connection;
   noPollConnOpts * opts;
   char *default_url = get_parodus_cfg()->webpa_path_url; 

   if (server->allow_insecure > 0) {
      ParodusPrint("secure false\n");
      opts = createConnOpts(ctx->extra_headers, false); 
      connection = nopoll_conn_new_opts (nopoll_ctx, opts, 
        server->server_addr,server->port,
        NULL, default_url,NULL,NULL);// WEBPA-787
   } else {
      ParodusPrint("secure true\n");
      opts = createConnOpts(ctx->extra_headers, true);
      if (is_ipv6) {
         ParodusInfo("Connecting in Ipv6 mode\n");
         connection = nopoll_conn_tls_new6 (nopoll_ctx, opts, 
           server->server_addr, server->port,
           NULL, default_url,NULL,NULL);
      } else {      
         ParodusInfo("Connecting in Ipv4 mode\n");
         connection = nopoll_conn_tls_new (nopoll_ctx, opts, 
           server->server_addr, server->port,
           NULL, default_url,NULL,NULL);
      }      
   }
   if (NULL == connection) {
     if((checkHostIp(server->server_addr) == -2)) {
       if (check_timer_expired (&ctx->connect_timer, 15*60*1000)) {
  	 ParodusError("WebPA unable to connect due to DNS resolving to 10.0.0.1 for over 15 minutes; crashing service.\n");
	 set_global_reconnect_reason("Dns_Res_webpa_reconnect");
	 set_global_reconnect_status(true);
						
	 kill(getpid(),SIGTERM);						
       }
     }
   }
           
   set_global_conn(connection);
   return (NULL != connection);
}

//--------------------------------------------------------------------
// Return codes for wait_connection_ready
#define WAIT_SUCCESS	0
#define WAIT_ACTION_RETRY	1	// if wait_status is 307, 302, 303 or 403
#define WAIT_FAIL 	2

static int wait_connection_ready (create_connection_ctx_t *ctx)
{
  int wait_status;
  char redirectURL[128]={'\0'};

  if(nopoll_conn_wait_until_connection_ready(get_global_conn(), 10, 
	&wait_status, redirectURL)) 
     return WAIT_SUCCESS;
  if(wait_status == 307 || wait_status == 302 || wait_status == 303)    // only when there is a http redirect
  {
	char *redirect_ptr = redirectURL;
	ParodusError("Received temporary redirection response message %s\n", redirectURL);
	// Extract server Address and port from the redirectURL
	if (strncmp (redirect_ptr, "Redirect:", 9) == 0)
	    redirect_ptr += 9;
	parse_server_url (redirect_ptr, &ctx->server_list.redirect);
	set_current_server (ctx); // set current server to redirect server
	return WAIT_ACTION_RETRY;
  }
  if(wait_status == 403) 
  {
	ParodusError("Received Unauthorized response with status: %d\n", wait_status);
	free_extra_headers (ctx);
	set_extra_headers (ctx, true);
	return WAIT_ACTION_RETRY;
  }
  ParodusError("Client connection timeout\n");	
  ParodusError("RDK-10037 - WebPA Connection Lost\n");
  return WAIT_FAIL;
}

 
//--------------------------------------------------------------------
// Return codes for connect_and_wait
#define CONN_WAIT_SUCCESS	0
#define CONN_WAIT_ACTION_RETRY	1	// if wait_status is 307, 302, 303 or 403
#define CONN_WAIT_RETRY_DNS 	2

static int connect_and_wait (create_connection_ctx_t *ctx)
{
  unsigned int force_flags = get_parodus_cfg()->flags;
  int is_ipv6 = true;
  int nopoll_connected;
  int wait_rtn;
  
  if( FLAGS_IPV4_ONLY == (FLAGS_IPV4_ONLY & force_flags) ) {
    is_ipv6 = false;
  }
  
  // This loop will be executed at most twice:
  // Once for ipv6 and once for ipv4
  while (true) {
    nopoll_connected = nopoll_connect (ctx, is_ipv6);
    wait_rtn = WAIT_FAIL;
    if (nopoll_connected) {
      if(nopoll_conn_is_ok(get_global_conn())) { 
	ParodusPrint("Connected to Server but not yet ready\n");
	wait_rtn = wait_connection_ready (ctx);
        if (wait_rtn == WAIT_SUCCESS)
          return CONN_WAIT_SUCCESS;
      } else { // nopoll_conn not ok
	ParodusError("Error connecting to server\n");
	ParodusError("RDK-10037 - WebPA Connection Lost\n");
      }
    } // nopoll_connected
    
    if (nopoll_connected) {
	close_and_unref_connection(get_global_conn());
	set_global_conn(NULL);
    }

    if (wait_rtn == WAIT_ACTION_RETRY)
      return CONN_WAIT_ACTION_RETRY;

    // try ipv4 if we need to      
    if ((0==force_flags) && (0==ctx->current_server->allow_insecure) && is_ipv6) {
      is_ipv6 = false;
      continue;
    }
    
    return CONN_WAIT_RETRY_DNS;
  }
}

//--------------------------------------------------------------------
// Tries to connect until
// a) success, or
// b) need to requery dns
static int keep_trying_to_connect (create_connection_ctx_t *ctx, 
	int max_retry_sleep,
	int query_dns_status)
{
    backoff_timer_t backoff_timer;
    int rtn;
    
    init_backoff_timer (&backoff_timer, max_retry_sleep);

    while (true)
    {
      rtn = connect_and_wait (ctx);
      if (rtn == CONN_WAIT_SUCCESS)
        return true;
      if (rtn == CONN_WAIT_ACTION_RETRY) // if redirected or build_headers
        continue;
      backoff_delay (&backoff_timer); // 3,7,15,31 ..
      if (rtn == CONN_WAIT_RETRY_DNS)
        if (query_dns_status < 0)
          return false;  //find_server again
      // else retry
    }
}


//--------------------------------------------------------------------

/**
 * @brief createNopollConnection interface to create WebSocket client connections.
 *Loads the WebPA config file and creates the intial connection and manages the connection wait, close mechanisms.
 */
int createNopollConnection(noPollCtx *ctx)
{
  create_connection_ctx_t conn_ctx;
  int max_retry_sleep;
  int query_dns_status;
  
  if(ctx == NULL) {
        return nopoll_false;
  }

	ParodusPrint("BootTime In sec: %d\n", get_parodus_cfg()->boot_time);
	ParodusInfo("Received reboot_reason as:%s\n", get_parodus_cfg()->hw_last_reboot_reason);
	ParodusInfo("Received reconnect_reason as:%s\n", reconnect_reason);
	
	max_retry_sleep = (int) get_parodus_cfg()->webpa_backoff_max;
	ParodusPrint("max_retry_sleep is %d\n", max_retry_sleep );
  
	conn_ctx.nopoll_ctx = ctx;
	init_expire_timer (&conn_ctx.connect_timer);
	init_header_info (&conn_ctx.header_info);
	set_extra_headers (&conn_ctx, false);
  
	while (true)
	{
	  query_dns_status = find_servers (&conn_ctx.server_list);
	  if (query_dns_status == FIND_INVALID_DEFAULT)
		return nopoll_false;
	  set_current_server (&conn_ctx);
	  if (keep_trying_to_connect (&conn_ctx, max_retry_sleep, query_dns_status))
		break;
	  // retry dns query
	}
      
	if(conn_ctx.current_server->allow_insecure <= 0)
	{
		ParodusInfo("Connected to server over SSL\n");
	}
	else 
	{
		ParodusInfo("Connected to server\n");
	}
	
	free_extra_headers (&conn_ctx);
	
	// Reset close_retry flag and heartbeatTimer once the connection retry is successful
	ParodusPrint("createNopollConnection(): close_mut lock\n");
	pthread_mutex_lock (&close_mut);
	close_retry = false;
	pthread_mutex_unlock (&close_mut);
	ParodusPrint("createNopollConnection(): close_mut unlock\n");
	heartBeatTimer = 0;
	set_global_reconnect_reason("webpa_process_starts");
	set_global_reconnect_status(false);
	ParodusPrint("LastReasonStatus reset after successful connection\n");
	setMessageHandlers();

	return nopoll_true;
}          

/* Build the extra headers string with any/all conditional logic in one place. */
static char* build_extra_headers( const char *auth, const char *device_id,
                                  const char *user_agent, const char *convey )
{
    return nopoll_strdup_printf(
            "%s%s"
            "\r\nX-WebPA-Device-Name: %s"
            "\r\nX-WebPA-Device-Protocols: wrp-0.11,getset-0.1"
            "\r\nUser-Agent: %s"
            "%s%s",

            (NULL != auth) ? "\r\nAuthorization: Bearer " : "",
            (NULL != auth) ? auth: "",
            device_id,
            user_agent,
            (NULL != convey) ? "\r\nX-WebPA-Convey: " : "",
            (NULL != convey) ? convey : "" );
}


static noPollConnOpts * createConnOpts (char * extra_headers, bool secure)
{
    noPollConnOpts * opts;
    
    opts = nopoll_conn_opts_new ();
    if(secure) 
	{
	    if(strlen(get_parodus_cfg()->cert_path) > 0)
            {
                nopoll_conn_opts_set_ssl_certs(opts, NULL, NULL, NULL, get_parodus_cfg()->cert_path);
            }
	    nopoll_conn_opts_ssl_peer_verify (opts, nopoll_true);
	    nopoll_conn_opts_set_ssl_protocol (opts, NOPOLL_METHOD_TLSV1_2);
	}
	nopoll_conn_opts_set_interface (opts,get_parodus_cfg()->webpa_interface_used);	
	nopoll_conn_opts_set_extra_headers (opts,extra_headers); 
	return opts;   
}


void close_and_unref_connection(noPollConn *conn)
{
    if (conn) {
        nopoll_conn_close(conn);
        if (0 < nopoll_conn_ref_count (conn)) {
            nopoll_conn_unref(conn);
        }
    }
}

