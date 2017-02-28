#include <stdbool.h>
#include <nopoll.h>
#include <nopoll_private.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../src/ParodusInternal.h"
#include "nopoll_mock.h"

const unsigned char *nopoll_msg_get_payload(noPollMsg *msg)
{
    if( NULL != msg ) {
        return (unsigned char *) "Dummy payload";
    }

    return NULL;
}

int nopoll_msg_get_payload_size(noPollMsg *msg)
{
    (void) msg;
    return 1;
}

nopoll_bool nopoll_msg_ref(noPollMsg *msg)
{
    (void) msg;
    return false;
}

void close_and_unref_connection(noPollConn *conn)
{
    (void) conn;
}

int checkHostIp(char * serverIP)
{
   (void) serverIP;
   return 0;
}

noPollConn * nopoll_conn_tls_new (noPollCtx  * ctx, noPollConnOpts * options, const char * host_ip, const char * host_port, const char * host_name, const char * get_url, const char * protocols, const char * origin, const char * outbound_interface, const char * headerNames[], const char * headerValues[], const int headerCount)
{
    UNUSED(options); UNUSED(host_port); UNUSED(host_name); UNUSED(get_url); UNUSED(protocols); 
    UNUSED(origin); UNUSED(outbound_interface); UNUSED(headerNames); UNUSED(headerValues); UNUSED(headerCount);
    //ParodusInfo("function_called : %s\n",__FUNCTION__);
    
    function_called();
    check_expected(ctx);
    check_expected(host_ip);
    return (noPollConn *)mock();
}

noPollConn * nopoll_conn_new (noPollCtx  * ctx, const char * host_ip, const char * host_port, const char * host_name, const char * get_url, const char * protocols, const char * origin, const char * outbound_interface, const char * headerNames[], const char * headerValues[], const int headerCount)
{
    UNUSED(host_port); UNUSED(host_name); UNUSED(get_url); UNUSED(protocols); UNUSED(origin);
    UNUSED(outbound_interface); UNUSED(headerNames); UNUSED(headerValues); UNUSED(headerCount);
    //ParodusInfo("function_called : %s\n",__FUNCTION__);
    function_called();
    check_expected(ctx);
    check_expected(host_ip);
    return (noPollConn *)mock();
}
			     
int  __nopoll_conn_send_common (noPollConn * conn, const char * content, long length, nopoll_bool  has_fin, long       sleep_in_header, noPollOpCode frame_type)
{
    UNUSED(has_fin); UNUSED(sleep_in_header); UNUSED(frame_type); UNUSED(content);
    //ParodusInfo("function_called : %s\n",__FUNCTION__);
    function_called();
    
    check_expected(conn);
    check_expected(length);
    return (int)mock();
}

nopoll_bool nopoll_conn_is_ok (noPollConn * conn)
{
    if(conn)
        return nopoll_true;
    
    return nopoll_false;
}

nopoll_bool nopoll_conn_wait_until_connection_ready (noPollConn * conn, int timeout, char * message)
{
    UNUSED(timeout); UNUSED(message);
    
    //ParodusInfo("function_called : %s\n",__FUNCTION__);
    function_called();
    
    if(conn)
        return nopoll_true;
        
    return nopoll_false;
}

int nopoll_conn_flush_writes(noPollConn * conn, long timeout, int previous_result)
{
    UNUSED(timeout);
    
    //ParodusInfo("function_called : %s\n",__FUNCTION__);
    function_called();
    
    check_expected(conn);
    check_expected(previous_result);
    return (int)mock();
}
