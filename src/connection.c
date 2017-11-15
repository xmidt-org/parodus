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

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define HTTP_CUSTOM_HEADER_COUNT                    	5
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

char deviceMAC[32]={'\0'};
static char *reconnect_reason = "webpa_process_starts";
static noPollConn *g_conn = NULL;
static noPollConnOpts * createConnOpts (char * extra_headers);
static noPollConn * nopoll_tls_common_conn (noPollCtx  * ctx,char * serverAddr,char *serverPort,char * extra_headers);

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

/**
 * @brief createNopollConnection interface to create WebSocket client connections.
 *Loads the WebPA config file and creates the intial connection and manages the connection wait, close mechanisms.
 */
int createNopollConnection(noPollCtx *ctx)
{
	bool initial_retry = false;
	int backoffRetryTime = 0;
    int max_retry_sleep;
    char port[8];
    char server_Address[256];
    char redirectURL[128]={'\0'};
	int allow_insecure;
    char *temp_ptr;
    int connErr=0;
    struct timespec connErr_start,connErr_end,*connErr_startPtr,*connErr_endPtr;
    connErr_startPtr = &connErr_start;
    connErr_endPtr = &connErr_end;
    //Retry Backoff count shall start at c=2 & calculate 2^c - 1.
	int c=2;
    char *conveyHeader = NULL;
    char device_id[32]={'\0'};
    char user_agent[512]={'\0'};
    char * extra_headers = NULL;
    
    if(ctx == NULL) {
        return nopoll_false;
    }

	//query dns and validate JWT
	allow_insecure = allow_insecure_conn();
	ParodusPrint("allow: %d\n", allow_insecure);
	if (allow_insecure < 0) {
		return nopoll_false;
	}

	ParodusPrint("BootTime In sec: %d\n", get_parodus_cfg()->boot_time);
	ParodusInfo("Received reboot_reason as:%s\n", get_parodus_cfg()->hw_last_reboot_reason);
	ParodusInfo("Received reconnect_reason as:%s\n", reconnect_reason);
	snprintf(port,sizeof(port),"%d",get_parodus_cfg()->port);
	parStrncpy(server_Address, get_parodus_cfg()->webpa_url, sizeof(server_Address));
	ParodusInfo("server_Address %s\n",server_Address);
					
	max_retry_sleep = (int) pow(2, get_parodus_cfg()->webpa_backoff_max) -1;
	ParodusPrint("max_retry_sleep is %d\n", max_retry_sleep );
	
    snprintf(user_agent, sizeof(user_agent),"%s (%s; %s/%s;)",
     ((0 != strlen(get_parodus_cfg()->webpa_protocol)) ? get_parodus_cfg()->webpa_protocol : "unknown"),
     ((0 != strlen(get_parodus_cfg()->fw_name)) ? get_parodus_cfg()->fw_name : "unknown"),
     ((0 != strlen(get_parodus_cfg()->hw_model)) ? get_parodus_cfg()->hw_model : "unknown"),
     ((0 != strlen(get_parodus_cfg()->hw_manufacturer)) ? get_parodus_cfg()->hw_manufacturer : "unknown"));

	ParodusInfo("User-Agent: %s\n",user_agent);
	conveyHeader = getWebpaConveyHeader();
	parStrncpy(deviceMAC, get_parodus_cfg()->hw_mac,sizeof(deviceMAC));
	snprintf(device_id, sizeof(device_id), "mac:%s", deviceMAC);
	ParodusInfo("Device_id %s\n",device_id);
	
    extra_headers = nopoll_strdup_printf("\r\nX-WebPA-Device-Name: %s"
		     "\r\nX-WebPA-Device-Protocols: wrp-0.11,getset-0.1"
		     "\r\nX-WebPA-Token: %s"
		     "\r\nUser-Agent: %s" "\r\nX-WebPA-Convey: %s",device_id,((0 != strlen(get_parodus_cfg()->webpa_token)) ? get_parodus_cfg()->webpa_token : ""),user_agent,(strlen(conveyHeader) > 0)? conveyHeader :"");
	
	do
	{
		//calculate backoffRetryTime and to perform exponential increment during retry
		if(backoffRetryTime < max_retry_sleep)
		{
			backoffRetryTime = (int) pow(2, c) -1;
		}
		ParodusPrint("New backoffRetryTime value calculated as %d seconds\n", backoffRetryTime);
        noPollConn *connection;
		if((FLAGS_SECURE == (get_parodus_cfg()->secure_flag & get_parodus_cfg()->flags)) || (!allow_insecure))
		{                    
		    ParodusPrint("secure true\n");
            connection = nopoll_tls_common_conn(ctx,server_Address, port, extra_headers);                
		}
		else 
		{
		    ParodusPrint("secure false\n");
            noPollConnOpts * opts;
            opts = createConnOpts(extra_headers);
            connection = nopoll_conn_new_opts (ctx, opts,server_Address,port,NULL,get_parodus_cfg()->webpa_path_url,NULL,NULL);// WEBPA-787
		}
        set_global_conn(connection);

		if(get_global_conn() != NULL)
		{
			if(!nopoll_conn_is_ok(get_global_conn())) 
			{
				ParodusError("Error connecting to server\n");
				ParodusError("RDK-10037 - WebPA Connection Lost\n");
				// Copy the server address from config to avoid retrying to the same failing talaria redirected node
				parStrncpy(server_Address, get_parodus_cfg()->webpa_url, sizeof(server_Address));
				close_and_unref_connection(get_global_conn());
				set_global_conn(NULL);
				initial_retry = true;
				
				ParodusInfo("Waiting with backoffRetryTime %d seconds\n", backoffRetryTime);
				sleep(backoffRetryTime);
				continue;
			}
			else 
			{
				ParodusPrint("Connected to Server but not yet ready\n");
				initial_retry = false;
				//reset backoffRetryTime back to the starting value, as next reason can be different					
				c = 2;
				backoffRetryTime = (int) pow(2, c) -1;
			}

			if(!nopoll_conn_wait_until_connection_ready(get_global_conn(), 10, redirectURL)) 
			{
				
				if (strncmp(redirectURL, "Redirect:", 9) == 0) // only when there is a http redirect
				{
					ParodusError("Received temporary redirection response message %s\n", redirectURL);
					// Extract server Address and port from the redirectURL
					temp_ptr = strtok(redirectURL , ":"); //skip Redirect 
					temp_ptr = strtok(NULL , ":"); // skip https
					temp_ptr = strtok(NULL , ":");
					parStrncpy(server_Address, temp_ptr+2, sizeof(server_Address));
					parStrncpy(port, strtok(NULL , "/"), sizeof(port));
					ParodusInfo("Trying to Connect to new Redirected server : %s with port : %s\n", server_Address, port);
					//reset c=2 to start backoffRetryTime as retrying using new redirect server
					c = 2;
				}
				else
				{
					ParodusError("Client connection timeout\n");	
					ParodusError("RDK-10037 - WebPA Connection Lost\n");
					// Copy the server address from config to avoid retrying to the same failing talaria redirected node
					parStrncpy(server_Address, get_parodus_cfg()->webpa_url, sizeof(server_Address));
					ParodusInfo("Waiting with backoffRetryTime %d seconds\n", backoffRetryTime);
					sleep(backoffRetryTime);
					c++;
				}
				close_and_unref_connection(get_global_conn());
				set_global_conn(NULL);
				initial_retry = true;
				
			}
			else 
			{
				initial_retry = false;				
				ParodusInfo("Connection is ready\n");
			}
		}
		else
		{
			
			/* If the connect error is due to DNS resolving to 10.0.0.1 then start timer.
			 * Timeout after 15 minutes if the error repeats continuously and kill itself. 
			 */
			if((checkHostIp(server_Address) == -2)) 	
			{
				if(connErr == 0)
				{
					getCurrentTime(connErr_startPtr);
					connErr = 1;
					ParodusInfo("First connect error occurred, initialized the connect error timer\n");
				}
				else
				{
					getCurrentTime(connErr_endPtr);
					ParodusPrint("checking timeout difference:%ld\n", timeValDiff(connErr_startPtr, connErr_endPtr));
					if(timeValDiff(connErr_startPtr, connErr_endPtr) >= (15*60*1000))
					{
						ParodusError("WebPA unable to connect due to DNS resolving to 10.0.0.1 for over 15 minutes; crashing service.\n");
						reconnect_reason = "Dns_Res_webpa_reconnect";
						LastReasonStatus = true;
						
						kill(getpid(),SIGTERM);						
					}
				}			
			}
			initial_retry = true;
			ParodusInfo("Waiting with backoffRetryTime %d seconds\n", backoffRetryTime);
			sleep(backoffRetryTime);
			c++;
			// Copy the server address from config to avoid retrying to the same failing talaria redirected node
			parStrncpy(server_Address, get_parodus_cfg()->webpa_url, sizeof(server_Address));
		}
				
	}while(initial_retry);
	
    if( FLAGS_SECURE == (get_parodus_cfg()->secure_flag & get_parodus_cfg()->flags) )
	{
		ParodusInfo("Connected to server over SSL\n");
	}
	else 
	{
		ParodusInfo("Connected to server\n");
	}
	
	// Reset close_retry flag and heartbeatTimer once the connection retry is successful
	ParodusPrint("createNopollConnection(): close_mut lock\n");
	pthread_mutex_lock (&close_mut);
	close_retry = false;
	pthread_mutex_unlock (&close_mut);
	ParodusPrint("createNopollConnection(): close_mut unlock\n");
	heartBeatTimer = 0;
	// Reset connErr flag on successful connection
	connErr = 0;
	reconnect_reason = "webpa_process_starts";
	LastReasonStatus =false;
	ParodusPrint("LastReasonStatus reset after successful connection\n");
	setMessageHandlers();

	return nopoll_true;
}
static noPollConn * nopoll_tls_common_conn (noPollCtx  * ctx,char * serverAddr,char *serverPort,char * extra_headers)
{
        unsigned int flags = 0;
        noPollConnOpts * opts;
        noPollConn *connection = NULL;
        opts = createConnOpts(extra_headers);

        flags = get_parodus_cfg()->flags;

        if( FLAGS_IPV4_ONLY == (FLAGS_IPV4_ONLY & flags) ) {
            ParodusInfo("Connecting in Ipv4 mode\n");
            connection = nopoll_conn_tls_new (ctx, opts,serverAddr,serverPort,NULL,get_parodus_cfg()->webpa_path_url,NULL,NULL);
        } else if( FLAGS_IPV6_ONLY == (FLAGS_IPV6_ONLY & flags) ) {
            ParodusInfo("Connecting in Ipv6 mode\n");
            connection = nopoll_conn_tls_new6 (ctx, opts,serverAddr,serverPort,NULL,get_parodus_cfg()->webpa_path_url,NULL,NULL);
        } else {
            ParodusInfo("Try connecting with Ipv6 mode\n");
            connection = nopoll_conn_tls_new6 (ctx, opts,serverAddr,serverPort,NULL,get_parodus_cfg()->webpa_path_url,NULL,NULL);
            if(connection == NULL)
            {
                ParodusInfo("Ipv6 connection failed. Try connecting with Ipv4 mode \n");
                opts = createConnOpts(extra_headers);
                connection = nopoll_conn_tls_new (ctx, opts,serverAddr,serverPort,NULL,get_parodus_cfg()->webpa_path_url,NULL,NULL);
            }
        }
        return connection;
}

static noPollConnOpts * createConnOpts (char * extra_headers)
{
    noPollConnOpts * opts;
    
    opts = nopoll_conn_opts_new ();
    if( FLAGS_SECURE == (FLAGS_SECURE & get_parodus_cfg()->flags) )
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

