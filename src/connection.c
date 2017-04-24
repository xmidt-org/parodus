/**
 * @file connection.c
 *
 * @description This decribes functions required to manage WebSocket client connections.
 *
 * Copyright (c) 2015  Comcast
 */
 
#include "connection.h"
#include "time.h"
#include "config.h"
#include "nopoll_helpers.h"
#include "mutex.h"
#include "spin_thread.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define HTTP_CUSTOM_HEADER_COUNT                    	4

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

char deviceMAC[32]={'\0'};
static char *reconnect_reason = "webpa_process_starts";
static noPollConn *g_conn = NULL;
static noPollConnOpts * createConnOpts ();
static noPollConn * nopoll_tls_common_conn (noPollCtx  * ctx,char * serverAddr,char *serverPort);

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
    char *temp_ptr;
    //Retry Backoff count shall start at c=2 & calculate 2^c - 1.
    int c=2;
    
    if(ctx == NULL) {
        return nopoll_false;
    }

	FILE *fp;
	fp = fopen("/tmp/parodus_ready", "r");

	if (fp!=NULL)
	{
		unlink("/tmp/parodus_ready");
		ParodusPrint("Closing Parodus_Ready FIle \n");
		fclose(fp);
	}

	ParodusPrint("BootTime In sec: %d\n", get_parodus_cfg()->boot_time);
	ParodusInfo("Received reconnect_reason as:%s\n", reconnect_reason);
	
	snprintf(port,sizeof(port),"%d",8080);
	parStrncpy(server_Address, get_parodus_cfg()->webpa_url, sizeof(server_Address));
	ParodusInfo("server_Address %s\n",server_Address);
					
	max_retry_sleep = (int) pow(2, get_parodus_cfg()->webpa_backoff_max) -1;
	ParodusPrint("max_retry_sleep is %d\n", max_retry_sleep );
	
	do
	{
		//calculate backoffRetryTime and to perform exponential increment during retry
		if(backoffRetryTime < max_retry_sleep)
		{
			backoffRetryTime = (int) pow(2, c) -1;
		}
		ParodusPrint("New backoffRetryTime value calculated as %d seconds\n", backoffRetryTime);
								
        noPollConn *connection;
		if(get_parodus_cfg()->secureFlag) 
		{                    
		    ParodusPrint("secure true\n");
            connection = nopoll_tls_common_conn(ctx,server_Address, port);                
		}
		else 
		{
		    ParodusPrint("secure false\n");
            connection = nopoll_conn_new(ctx, server_Address, port, NULL,
                       get_parodus_cfg()->webpa_path_url, NULL, NULL);// WEBPA-787
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
			initial_retry = true;
			ParodusInfo("Waiting with backoffRetryTime %d seconds\n", backoffRetryTime);
			sleep(backoffRetryTime);
			c++;
			// Copy the server address from config to avoid retrying to the same failing talaria redirected node
			parStrncpy(server_Address, get_parodus_cfg()->webpa_url, sizeof(server_Address));
		}
				
	}while(initial_retry);
	
	if(get_parodus_cfg()->secureFlag) 
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
	reconnect_reason = "webpa_process_starts";
	LastReasonStatus =false;
	ParodusPrint("LastReasonStatus reset after successful connection\n");
	setMessageHandlers();

	return nopoll_true;
}
static noPollConn * nopoll_tls_common_conn (noPollCtx  * ctx,char * serverAddr,char *serverPort)
{
        noPollConnOpts * opts;
        noPollConn *connection = NULL;
        opts = createConnOpts();
        ParodusPrint("Trying connection with Ipv6 \n"); 
        connection = nopoll_conn_tls_new6 (ctx, opts,serverAddr,serverPort,NULL,get_parodus_cfg()->webpa_path_url,NULL,NULL);
        if(connection == NULL)
        {
            ParodusPrint("Ipv6 connection failed. Trying connection with Ipv4 \n");
            opts = createConnOpts();
            connection = nopoll_conn_tls_new (ctx, opts,serverAddr,serverPort,NULL,get_parodus_cfg()->webpa_path_url,NULL,NULL);
        }
        return connection;
}

static noPollConnOpts * createConnOpts ()
{
    noPollConnOpts * opts;
    char device_id[32]={'\0'};
    char * extra_headers, *conveyHeader = NULL;
    char user_agent[512]={'\0'};
    
    opts = nopoll_conn_opts_new ();
	nopoll_conn_opts_ssl_peer_verify (opts, nopoll_false);
	nopoll_conn_opts_set_ssl_protocol (opts, NOPOLL_METHOD_TLSV1_2);
	
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
		     "\r\nUser-Agent: %s" "\r\nX-WebPA-Convey: %s",device_id,user_agent,(strlen(conveyHeader) > 0)? conveyHeader :"");

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

