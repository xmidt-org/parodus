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
#include "auth_token.h"
#include "nopoll_helpers.h"
#include "mutex.h"
#include "spin_thread.h"
#include "ParodusInternal.h"
#include "heartBeat.h"
#include "close_retry.h"
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define HTTP_CUSTOM_HEADER_COUNT                    	5
#define INITIAL_CJWT_RETRY                    	-2

/* Close codes defined in RFC 6455, section 11.7. */
enum {
	CloseNormalClosure           = 1000,
	CloseGoingAway               = 1001,
	CloseProtocolError           = 1002,
	CloseUnsupportedData         = 1003,
	CloseNoStatus		     = 1005,
	CloseAbnormalClosure         = 1006,
	CloseInvalidFramePayloadData = 1007,
	ClosePolicyViolation         = 1008,
	CloseMessageTooBig           = 1009,
	CloseMandatoryExtension      = 1010,
	CloseInternalServerErr       = 1011,
	CloseServiceRestart          = 1012,
	CloseTryAgainLater           = 1013,
	CloseTLSHandshake            = 1015
};

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

static char *shutdown_reason = SHUTDOWN_REASON_PARODUS_STOP;  /* goes in the close message */
static char *reconnect_reason = "webpa_process_starts";
static int cloud_disconnect_max_time = 5;
static noPollConn *g_conn = NULL;
static bool LastReasonStatus = false;
static int init = 1;
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

char *get_global_shutdown_reason()
{
    return shutdown_reason;
}

void set_global_shutdown_reason(char *reason)
{
    shutdown_reason = reason;
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

int get_cloud_disconnect_time()
{
   return cloud_disconnect_max_time;
}

void set_cloud_disconnect_time(int disconnTime)
{
    cloud_disconnect_max_time = disconnTime;
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

#define FREE_PTR_VAR(ptr_var) \
  if (NULL != ptr_var) { \
    free(ptr_var); \
    ptr_var = NULL; \
  }

void set_server_null (server_t *server)
{
  server->server_addr = NULL;
}

void set_server_list_null (server_list_t *server_list)
{
  set_server_null (&server_list->defaults);
  set_server_null (&server_list->jwt);
  set_server_null (&server_list->redirect);
}

int server_is_null (server_t *server)
{
  return (NULL == server->server_addr);
}

void free_server (server_t *server)
{
  FREE_PTR_VAR (server->server_addr)
}

void free_server_list (server_list_t *server_list)
{
  free_server (&server_list->redirect);
  free_server (&server_list->jwt);
  free_server (&server_list->defaults);
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
	&server->server_addr, &server->port);
  return server->allow_insecure;
}

//--------------------------------------------------------------------
void init_expire_timer (expire_timer_t *timer)
{
  timer->running = false;
}

int check_timer_expired (expire_timer_t *timer, long timeout_ms)
{
  long time_diff_ms;
  
  if (!timer->running) {
    getCurrentTime(&timer->start_time);
    timer->running = true;
    ParodusInfo("First connect error occurred, initialized the connect error timer\n");
    return false;
  }
  
  getCurrentTime(&timer->end_time);
  time_diff_ms = timeValDiff (&timer->start_time, &timer->end_time);
  ParodusPrint("checking timeout difference:%ld\n", time_diff_ms);
  if(time_diff_ms >= timeout_ms)
    return true;
  return false;
}

//--------------------------------------------------------------------
void init_backoff_timer (backoff_timer_t *timer, int max_count)
{
  timer->count = 1;
  timer->max_count = max_count;
  timer->delay = 1;
}

int update_backoff_delay (backoff_timer_t *timer)
{
  if (timer->count < timer->max_count) {
    timer->count += 1;
    timer->delay = timer->delay + timer->delay + 1;
    // 3,7,15,31 ..
  }
  return timer->delay;
}  

static void backoff_delay (backoff_timer_t *timer)
{
  update_backoff_delay (timer);

  // Update retry time for conn progress
  if(timer->count == timer->max_count) 
  {
	start_conn_in_progress();
  }

  ParodusInfo("Waiting with backoffRetryTime %d seconds\n", timer->delay);
  sleep (timer->delay);
}  

//--------------------------------------------------------------------
void free_header_info (header_info_t *header_info)
{
  FREE_PTR_VAR (header_info->user_agent)
  FREE_PTR_VAR (header_info->device_id)
  // Don't free header_info->conveyHeader, because it's static
  header_info->conveyHeader = NULL;
}

void set_header_info_null (header_info_t *header_info)
{
  header_info->conveyHeader = NULL;
  header_info->user_agent = NULL;
  header_info->device_id = NULL;
}

int init_header_info (header_info_t *header_info)
{
  ParodusCfg *cfg = get_parodus_cfg();
  size_t device_id_len;
#define CFG_PARAM(param) ((0 != strlen(cfg->param)) ? cfg->param : "unknown")

  const char *user_agent_format = "%s (%s; %s/%s;)";
  char *protocol = CFG_PARAM (webpa_protocol);
  char *fw_name = CFG_PARAM (fw_name);
  char *hw_model = CFG_PARAM (hw_model);
  char *hw_manufacturer = CFG_PARAM (hw_manufacturer);
  
  size_t user_agent_len = strlen(protocol) + strlen(fw_name) + 
    strlen(hw_model) + strlen(hw_manufacturer) + strlen(user_agent_format)
    + 1;
    
  set_header_info_null (header_info);  

  header_info->user_agent = (char *) malloc (user_agent_len);
  if (NULL == header_info->user_agent) {
    ParodusError ("header user agent allocation failed.\n");
    return -1;
  }
  
  snprintf(header_info->user_agent, user_agent_len, user_agent_format,
     protocol, fw_name, hw_model, hw_manufacturer);
  device_id_len = strlen (cfg->hw_mac) + 5;
  header_info->device_id = (char *) malloc (device_id_len);
  if (NULL == header_info->device_id) {
    ParodusError ("header device id allocation failed.\n");
    FREE_PTR_VAR (header_info->user_agent)
    return -1;
  }
  snprintf(header_info->device_id, device_id_len, "mac:%s", cfg->hw_mac);
  
  ParodusInfo("User-Agent: %s\n",header_info->user_agent);
  header_info->conveyHeader = getWebpaConveyHeader();  // ptr to static variable returned
  if (NULL == header_info->conveyHeader) {
    ParodusError ("getWebpaConveyHeader error\n");
    free_header_info (header_info);
    return -1;
  }
  ParodusInfo("Device_id %s\n", header_info->device_id);
  return 0;
#undef CFG_PARAM
} 

char *build_extra_hdrs (header_info_t *header_info)
// result must be freed
{
  char *auth_token = get_parodus_cfg()->webpa_auth_token;
  return build_extra_headers( (0 < strlen(auth_token) ? auth_token : NULL),
	header_info->device_id, header_info->user_agent, header_info->conveyHeader );
}


//--------------------------------------------------------------------
void set_current_server (create_connection_ctx_t *ctx)
{
  ctx->current_server = get_current_server (&ctx->server_list);
}

void free_extra_headers (create_connection_ctx_t *ctx)
{
  FREE_PTR_VAR (ctx->extra_headers)
}

void set_extra_headers (create_connection_ctx_t *ctx)
{
  ParodusCfg * cfg = get_parodus_cfg();

  free_extra_headers (ctx);
  if ((strlen(cfg->webpa_auth_token) == 0) &&
      (cfg->client_cert_path != NULL) && (strlen(cfg->client_cert_path) > 0))
  {
    getAuthToken(cfg);
  }
  
  ctx->extra_headers = build_extra_hdrs (&ctx->header_info);
}

void free_connection_ctx (create_connection_ctx_t *ctx)
{
  free_extra_headers (ctx);
  free_header_info (&ctx->header_info);
  free_server_list (&ctx->server_list);
}


//--------------------------------------------------------------------
// find_servers:
// get and parse default server
// if necessary, query dns and parse server from jwt
// populate server_list
// return values defined in ParodusInternal.h

int find_servers (server_list_t *server_list)
{
  server_t *default_server = &server_list->defaults;

  free_server_list (server_list);
  // parse default server URL
  if (parse_server_url (get_parodus_cfg()->webpa_url, default_server) < 0)
     return FIND_INVALID_DEFAULT;	// must have valid default url
  ParodusInfo("default server_Address %s\n", default_server->server_addr);
  ParodusInfo("default port %u\n", default_server->port);
#ifdef FEATURE_DNS_QUERY
  if (get_parodus_cfg()->acquire_jwt) {
    server_t *jwt_server = &server_list->jwt;
    //query dns and validate JWT
    jwt_server->allow_insecure = allow_insecure_conn(
             &jwt_server->server_addr, &jwt_server->port);
    if (jwt_server->allow_insecure < 0) {
      return FIND_JWT_FAIL;
    }
    ParodusInfo("JWT ON: jwt_server_url stored as %s\n", jwt_server->server_addr);
  }
#endif
  return FIND_SUCCESS;
}


//--------------------------------------------------------------------
// connect to current server
int nopoll_connect (create_connection_ctx_t *ctx, int is_ipv6)
{
   noPollCtx *nopoll_ctx = ctx->nopoll_ctx;
   server_t *server = ctx->current_server;
   noPollConn *connection;
   noPollConnOpts * opts;
   char *default_url = get_parodus_cfg()->webpa_path_url; 
   char port_buf[8];

   sprintf (port_buf, "%u", server->port);
   if (server->allow_insecure > 0) {
      ParodusPrint("secure false\n");
      opts = createConnOpts(ctx->extra_headers, false); 
      connection = nopoll_conn_new_opts (nopoll_ctx, opts, 
        server->server_addr, port_buf,
        NULL, default_url,NULL,NULL);// WEBPA-787
   } else {
      ParodusPrint("secure true\n");
      opts = createConnOpts(ctx->extra_headers, true);
      if (is_ipv6) {
         ParodusInfo("Connecting in Ipv6 mode\n");
         connection = nopoll_conn_tls_new6 (nopoll_ctx, opts, 
           server->server_addr, port_buf,
           NULL, default_url,NULL,NULL);
      } else {      
         ParodusInfo("Connecting in Ipv4 mode\n");
         connection = nopoll_conn_tls_new (nopoll_ctx, opts, 
           server->server_addr, port_buf,
           NULL, default_url,NULL,NULL);
      }      
   }
   if ((NULL == connection) && (!is_ipv6)) {
     if((checkHostIp(server->server_addr) == -2)) {
       if (check_timer_expired (&ctx->connect_timer, 15*60*1000) && !get_interface_down_event()) {
  	 ParodusError("WebPA unable to connect due to DNS resolving to 10.0.0.1 for over 15 minutes; crashing service.\n");
	 OnboardLog("WebPA unable to connect due to DNS resolving to 10.0.0.1 for over 15 minutes; crashing service.\n");
	 OnboardLog("Reconnect detected, setting Dns_Res_webpa_reconnect reason for Reconnect\n");
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
#define WAIT_ACTION_RETRY	1	// if wait_status is 307, 302, 303, or 403
#define WAIT_FAIL 	2

#define FREE_NON_NULL_PTR(ptr) if (NULL != ptr) free(ptr)

int wait_connection_ready (create_connection_ctx_t *ctx)
{
  int wait_status = 0;
  char *redirectURL = NULL;

  if(nopoll_conn_wait_for_status_until_connection_ready(get_global_conn(), 10, 
	&wait_status, &redirectURL)) 
  { 
     FREE_NON_NULL_PTR (redirectURL);
     return WAIT_SUCCESS;
  }
  if(wait_status == 307 || wait_status == 302 || wait_status == 303)    // only when there is a http redirect
  {
	char *redirect_ptr = redirectURL;
	ParodusError("Received temporary redirection response message %s\n", redirectURL);
	// Extract server Address and port from the redirectURL
	if (strncmp (redirect_ptr, "Redirect:", 9) == 0)
	    redirect_ptr += 9;
	free_server (&ctx->server_list.redirect);
	if (parse_server_url (redirect_ptr, &ctx->server_list.redirect) < 0) {
	  ParodusError ("Redirect url error %s\n", redirectURL);
  	  free (redirectURL);
	  return WAIT_FAIL;
	}
	free (redirectURL);
	set_current_server (ctx); // set current server to redirect server
	return WAIT_ACTION_RETRY;
  }
  FREE_NON_NULL_PTR (redirectURL);
  if(wait_status == 403) 
  {
    ParodusCfg *cfg = get_parodus_cfg();
    /* clear auth token in cfg so that we will refetch auth token */
    memset (cfg->webpa_auth_token, 0, sizeof(cfg->webpa_auth_token));
    ParodusError("Received Unauthorized response with status: %d\n", wait_status);
	OnboardLog("Received Unauthorized response with status: %d\n", wait_status);
    return WAIT_ACTION_RETRY;
  }
  ParodusError("Client connection timeout\n");	
  ParodusError("RDK-10037 - WebPA Connection Lost\n");
  return WAIT_FAIL;
}

 
//--------------------------------------------------------------------
// Return codes for connect_and_wait
#define CONN_WAIT_SUCCESS	 0
#define CONN_WAIT_ACTION_RETRY	 1	// if wait_status is 307, 302, 303, or 403
#define CONN_WAIT_RETRY_DNS 	 2

int connect_and_wait (create_connection_ctx_t *ctx)
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
int keep_trying_to_connect (create_connection_ctx_t *ctx, 
	backoff_timer_t *backoff_timer)
{
    int rtn;
    
    while (true)
    {
      set_extra_headers (ctx);

      rtn = connect_and_wait (ctx);
      if (rtn == CONN_WAIT_SUCCESS)
        return true;

      if (rtn == CONN_WAIT_ACTION_RETRY) // if redirected or build_headers
        continue;
      backoff_delay (backoff_timer); // 3,7,15,31 ..
      if (rtn == CONN_WAIT_RETRY_DNS)
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
  int max_retry_count;
  int query_dns_status;
  struct timespec connect_time,*connectTimePtr;
  connectTimePtr = &connect_time;
  backoff_timer_t backoff_timer;
  
  if(ctx == NULL) {
        return nopoll_false;
  }

	ParodusPrint("BootTime In sec: %d\n", get_parodus_cfg()->boot_time);
	ParodusInfo("Received reboot_reason as:%s\n", get_parodus_cfg()->hw_last_reboot_reason);
	ParodusInfo("Received reconnect_reason as:%s\n", reconnect_reason);
	
	max_retry_count = (int) get_parodus_cfg()->webpa_backoff_max;
	ParodusPrint("max_retry_count is %d\n", max_retry_count );

	memset (&conn_ctx, 0, sizeof(create_connection_ctx_t));
	conn_ctx.nopoll_ctx = ctx;
	init_expire_timer (&conn_ctx.connect_timer);
	init_header_info (&conn_ctx.header_info);
        set_server_list_null (&conn_ctx.server_list);
        init_backoff_timer (&backoff_timer, max_retry_count);
  
	while (!g_shutdown)
	{
	  query_dns_status = find_servers (&conn_ctx.server_list);
	  if (query_dns_status == FIND_INVALID_DEFAULT)
		return nopoll_false;
	  set_current_server (&conn_ctx);
	  if (keep_trying_to_connect (&conn_ctx, &backoff_timer))
		break;
	  // retry dns query

	  // If interface down event is set, stop retry
	  // and wait till interface is up again.
	  if(get_interface_down_event()) {
		ParodusError("Interface is down, hence pausing retry and waiting until its up\n");
		pthread_mutex_lock(get_interface_down_mut());
		pthread_cond_wait(get_interface_down_con(), get_interface_down_mut());
		pthread_mutex_unlock (get_interface_down_mut());
		ParodusInfo("Interface is back up, re-initializing the convey header\n");
		// Reset the reconnect reason by initializing the convey header again
		((header_info_t *)(&conn_ctx.header_info))->conveyHeader = getWebpaConveyHeader();
		ParodusInfo("Received reconnect_reason as:%s\n", reconnect_reason);  
	  }
	}
      
	if(conn_ctx.current_server->allow_insecure <= 0)
	{
		ParodusInfo("Connected to server over SSL\n");
		OnboardLog("Connected to server over SSL\n");
	}
	else 
	{
		ParodusInfo("Connected to server\n");
		OnboardLog("Connected to server\n");
	}
	
	get_parodus_cfg()->cloud_status = CLOUD_STATUS_ONLINE;
	ParodusInfo("cloud_status set as %s after successful connection\n", get_parodus_cfg()->cloud_status);

	// Invoke the ping status change event callback as "received" ping
	if(NULL != on_ping_status_change)
	{
		on_ping_status_change("received");
	}

	if((get_parodus_cfg()->boot_time != 0) && init) {
		getCurrentTime(connectTimePtr);
		ParodusInfo("connect_time-diff-boot_time=%d\n", connectTimePtr->tv_sec - get_parodus_cfg()->boot_time);
		init = 0; //set init to 0 so that this is logged only during process start up and not during reconnect
	}

	free_extra_headers (&conn_ctx);
        free_header_info (&conn_ctx.header_info);
        free_server_list (&conn_ctx.server_list);
        
	// Reset close_retry flag and heartbeatTimer once the connection retry is successful
	ParodusPrint("createNopollConnection(): reset_close_retry\n");
	reset_close_retry();
	reset_heartBeatTimer();
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
    char * mtls_client_cert_path = NULL;
    char * mtls_client_key_path = NULL;
    
    opts = nopoll_conn_opts_new ();
    if(secure) 
	{
	    if(strlen(get_parodus_cfg()->cert_path) > 0)
            {
                if( ( get_parodus_cfg()->mtls_client_cert_path !=NULL && strlen(get_parodus_cfg()->mtls_client_cert_path) > 0) && (get_parodus_cfg()->mtls_client_key_path !=NULL && strlen(get_parodus_cfg()->mtls_client_key_path) > 0) )
                {
                       mtls_client_cert_path = get_parodus_cfg()->mtls_client_cert_path;
                       mtls_client_key_path = get_parodus_cfg()->mtls_client_key_path;
                }
                nopoll_conn_opts_set_ssl_certs(opts, mtls_client_cert_path, mtls_client_key_path, NULL, get_parodus_cfg()->cert_path);
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
	  const char *reason = get_global_shutdown_reason();
	  int reason_len = 0;
	  int status = CloseNoStatus;
	  if (NULL != reason) {
		reason_len = (int) strlen (reason);
		status = CloseNormalClosure;
	  }
      nopoll_conn_close_ext(conn, status, reason, reason_len);
    }
}

void write_conn_in_prog_file (const char *msg)
{
  int fd;
  FILE *fp;
  unsigned long timestamp;
  ParodusCfg *cfg = get_parodus_cfg();

  if (NULL == cfg->connection_health_file)
    return;
  fd = open (cfg->connection_health_file, O_CREAT | O_WRONLY | O_SYNC, 0666);
  if (fd < 0) {
    ParodusError ("Error(1) %d opening file %s\n", errno, cfg->connection_health_file);
    return;
  }
  ftruncate (fd, 0);
  fp = fdopen (fd, "w");
  if (fp == NULL) {
    ParodusError ("Error(2) %d opening file %s\n", errno, cfg->connection_health_file);
    return;
  }
  timestamp = (unsigned long) time(NULL);
  fprintf (fp, "{%s=%lu}\n", msg, timestamp);
  fclose (fp);
}

void start_conn_in_progress (void)
{
  write_conn_in_prog_file ("START");
}   

void stop_conn_in_progress (void)
{
  write_conn_in_prog_file ("STOP");
}   


void registerParodusOnPingStatusChangeHandler(parodusOnPingStatusChangeHandler callback_func)
{
	on_ping_status_change = callback_func;
}

