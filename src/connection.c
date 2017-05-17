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
#include "upstream.h"
#include "nopoll_helpers.h"
#include "mutex.h"
#include "spin_thread.h"
#include "nopoll_handlers.h"
#include <libwebsockets.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define MAX_PAYLOAD                                     1024

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

char deviceMAC[32]={'\0'};
static char *reconnect_reason = "webpa_process_starts";
static struct lws_context *g_context;
static struct lws *wsi_dumb;
int connected = 0;
pthread_mutex_t res_mutex ;

bool conn_retry;
char * fragmentMsg = NULL;
int fragmentSize = 0;
int payloadSize = 0;

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

struct lws_context *get_global_context(void)
{
    return g_context;
}

void set_global_context(struct lws_context *contextRef)
{
    g_context = contextRef;
}

char *get_global_reconnect_reason()
{
    return reconnect_reason;
}

void set_global_reconnect_reason(char *reason)
{
    reconnect_reason = reason;
}


void
dump_handshake_info(struct lws *wsi)
{
	int n = 0, len;
	char buf[256];
	const unsigned char *c;

	do {
		c = lws_token_to_string(n);
		if (!c) {
			n++;
			continue;
		}

		len = lws_hdr_total_length(wsi, n);
		if (!len || len > sizeof(buf) - 1) {
			n++;
			continue;
		}

		lws_hdr_copy(wsi, buf, sizeof buf, n);
		buf[sizeof(buf) - 1] = '\0';

		fprintf(stderr, "    %s = %s\n", (char *)c, buf);
		n++;
	} while (c);
}

char * join_fragment_msg (char *firstMsg,int firstSize,char *secondMsg,int secondSize,int * result)
{
    *result = firstSize + secondSize;
    char *tmpMsg = (char *)malloc(sizeof(char)* (*result));
    memcpy(tmpMsg,firstMsg,firstSize);
    memcpy (tmpMsg + (firstSize), secondMsg, secondSize);
    return tmpMsg;
} 

static int
parodus_callback(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
	int n;
	char * payload = NULL;
    char * out = NULL;
	switch (reason) {

	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		ParodusInfo("Connected to server successfully\n");
		connected = 1;
		conn_retry = false;
		break;

	case LWS_CALLBACK_CLOSED:
		ParodusInfo("Callback closed, websocket session ends \n");
		wsi_dumb = NULL;
		conn_retry = true;
		break;
    case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		dump_handshake_info(wsi);
		/* you could return non-zero here and kill the connection */
		break;
	case LWS_CALLBACK_CLIENT_RECEIVE:
	    ((char *)in)[len] = '\0';
	    
		//lwsl_notice("length %d payload : %s\n", (int)len, (char *)in);

		ParodusInfo("**** Recieved %d bytes from server\n",(int)len);
		char * tmpMsg = NULL;
		payload = (char *)malloc(sizeof(char)*len);
		strcpy(payload,(char *)in);
	    if (lws_is_final_fragment(wsi))
	    {
	        
	        if(fragmentMsg == NULL)
	        {
		        listenerOnrequest_queue(payload,len);
		    }
		    else
		    {
		        tmpMsg = join_fragment_msg(fragmentMsg,fragmentSize,payload,len,&payloadSize);  
		        len = payloadSize;
		        listenerOnrequest_queue(tmpMsg,len);
		        fragmentMsg = NULL;
		        len = 0;
		    }   
	    }
	    else
	    {
		    if(fragmentMsg == NULL)
		    {
		        fragmentMsg = payload;
		        fragmentSize = len;
		    }
		    else
		    {
                fragmentMsg = join_fragment_msg(fragmentMsg,fragmentSize,payload,len,&payloadSize);
		    }
	    }
		
		lws_callback_on_writable(wsi);
		break;

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		ParodusError("***** Failed to Connected to server *******\n");
		wsi_dumb = NULL;
		conn_retry = true;
		break;

	case LWS_CALLBACK_CLIENT_WRITEABLE:
		// Call back to send response to server
        if(ResponseMsgQ != NULL)
        {
            //Read response data from queue
            pthread_mutex_lock (&res_mutex);
            while(ResponseMsgQ)
            {
                UpStreamMsg *message = ResponseMsgQ;
                ResponseMsgQ = ResponseMsgQ->next;
                pthread_mutex_unlock (&res_mutex);

                out = (char *)malloc(sizeof(char) * (LWS_PRE + message->len));
                memcpy (LWS_PRE + out, message->msg, message->len);
                char * tmpPtr = LWS_PRE + out;
                n = lws_write(wsi, tmpPtr, message->len, LWS_WRITE_BINARY);
	            if (n < 0)
	            {
                    ParodusError("Failed to send to server\n");
		            free(message);
                    message = NULL;
		            return 1;
		        }
		        if (n < message->len) {
			        ParodusError("Partial write\n");
			        return -1;
		        }    
                ParodusInfo("Sent %d bytes of data to server successfully \n",n);
                free(out);
                free(message);
                message = NULL;
            }
        }
                   
	    lws_callback_on_writable(wsi);
		break;

	case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
	    //Call back to send custom header to server
	    ParodusPrint("Send custom header to server\n");
	    char device_id[32]={'\0'};
	    char user_agent[512]={'\0'};
	    char *conveyHeader;
		unsigned char **p = (unsigned char **)in, *end = (*p) + len;
		strncpy(deviceMAC, get_parodus_cfg()->hw_mac,sizeof(deviceMAC));
	    snprintf(device_id, sizeof(device_id), "mac:%s", deviceMAC);
	    snprintf(user_agent, sizeof(user_agent),"%s (%s; %s/%s;)",
         ((0 != strlen(get_parodus_cfg()->webpa_protocol)) ? get_parodus_cfg()->webpa_protocol : "unknown"),
         ((0 != strlen(get_parodus_cfg()->fw_name)) ? get_parodus_cfg()->fw_name : "unknown"),
         ((0 != strlen(get_parodus_cfg()->hw_model)) ? get_parodus_cfg()->hw_model : "unknown"),
         ((0 != strlen(get_parodus_cfg()->hw_manufacturer)) ? get_parodus_cfg()->hw_manufacturer : "unknown"));
        
        ParodusInfo("User-Agent: %s\n",user_agent);
	    /*TODO Implement base64 encoding, currently we are using nopoll API to encode data*/
	    //conveyHeader = getWebpaConveyHeader();			
		
		if (lws_add_http_header_by_name(wsi,
				(unsigned char *)"X-WebPA-Device-Name:",
				(unsigned char *)device_id,strlen(device_id),p,end))
		    return -1;
		if (lws_add_http_header_by_name(wsi,
				(unsigned char *)"X-WebPA-Device-Protocols:",
				(unsigned char *)"wrp-0.11,getset-0.1",strlen("wrp-0.11,getset-0.1"),p,end))
		    return -1;
		if (lws_add_http_header_by_name(wsi,
				(unsigned char *)"User-Agent:",
				(unsigned char *)user_agent,strlen(user_agent),p,end))
		    return -1;
		    
		/* if(strlen(conveyHeader) > 0)
		{
		    if (lws_add_http_header_by_name(wsi,
				    (unsigned char *)"X-WebPA-Convey:",
				    (unsigned char *)conveyHeader,strlen(conveyHeader),p,end))
		    return -1;
		}*/
		break;
    case LWS_CALLBACK_OPENSSL_PERFORM_SERVER_CERT_VERIFICATION:
            /* Disable self signed verification */
            X509_STORE_CTX_set_error((X509_STORE_CTX*)user, X509_V_OK);
            return 0;
        break;
	default:
		break;
	}

	return 0;
}

static const struct lws_protocols protocols[] = {
	{
		NULL,
		parodus_callback,
		0,
		MAX_PAYLOAD,
	},
	{ NULL, NULL, 0, 0 } /* end */
};

LWS_VISIBLE void lwsl_emit_syslog(int level, const char *line)
{
	ParodusInfo(" %s\n",line);
}

void createLWSconnection()
{
    struct lws_context_creation_info info;
	struct lws_client_connect_info i;
	struct lws_context *context;
	const char *prot, *p;
	int port = 8080,use_ssl =0;
    
	memset(&info, 0, sizeof info);
	memset(&i, 0, sizeof(i));
	 
	use_ssl = LCCSCF_USE_SSL;			  
	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;
	lws_set_log_level(LLL_INFO | LLL_NOTICE | LLL_WARN | LLL_LATENCY | LLL_CLIENT | LLL_COUNT,NULL);

	info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

	context = lws_create_context(&info);
	if (context == NULL) {
		fprintf(stderr, "Creating libwebsocket context failed\n");
		exit(1);
	}
    set_global_context(context);
	i.port = port;
	i.address = get_parodus_cfg()->webpa_url;
	i.context = context;
	i.ssl_connection = use_ssl;
	i.host = i.address;
	i.path = "/api/v2/device";
	while(1)
	{
		if (!wsi_dumb)
		{
			ParodusInfo("Connecting to server..\n");
			i.pwsi = &wsi_dumb;
			lws_client_connect_via_info(&i);
		}
		/*We need lws service here till client connects to server, without lws_service callback will not work*/
		lws_service(context, 500);
		if(connected)
		{
		    /*Connected to server, break the loop and try lws service from main thread*/
		    conn_retry = false;
		    break;
		}    
	}
}

