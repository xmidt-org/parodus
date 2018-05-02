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
/**
 * @file parodus_interface.c
 *
 * @description Defines parodus-to-parodus API.
 *
 */

#include "ParodusInternal.h"
#include "parodus_interface.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static char *p2p_url;

/*----------------------------------------------------------------------------*/
/*                             Internal Functions                             */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                             External functions                             */
/*----------------------------------------------------------------------------*/
void set_parodus_to_parodus_listener_url(const char *url)
{
    p2p_url = strdup(url);
}

void cleanup_parodus_to_parodus_listener_url(void)
{
    free(p2p_url);
}

bool spoke_send_msg(const char *url, const char *notification, size_t notification_size)
{
    wrp_msg_t *msg;
    int sock;
    int rv;
    int bytes_sent = 0;
    int payload_size;
    int t = 2000;
    char *payload_bytes;
    
    msg = (wrp_msg_t *) malloc(sizeof(wrp_msg_t));
    memset(msg, 0, sizeof(wrp_msg_t));
    
    msg->msg_type = WRP_MSG_TYPE__EVENT;
    msg->u.event.content_type = "application/msgpack";
    msg->u.event.source  = (char *) p2p_url;
    msg->u.event.dest    = (char *) url;
    msg->u.event.payload = (char *) notification;
    msg->u.event.payload_size = notification_size;

    payload_size = wrp_struct_to( (const wrp_msg_t *) msg, WRP_BYTES, (void **) &payload_bytes);
    free(msg);
    
    sock = nn_socket(AF_SP, NN_PUSH);
    if( sock < 0 ) {
        ParodusError("NN parodus send socket error %d\n", sock);
        goto finished;
    }

    rv = nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
    if( rv < 0 ) {
        ParodusError("NN parodus send timeout setting error %d\n", rv);
        goto finished;
    }

    rv = nn_connect(sock, url);
    if( rv < 0 ) {
        ParodusError("NN parodus send connect error %d\n", rv);
        goto finished;
    }

    bytes_sent = nn_send(sock, payload_bytes, payload_size, 0);
    if( bytes_sent < 0 ) {
        ParodusError("NN send msg - bytes_sent = %d, %d(%s)\n", bytes_sent, errno, strerror(errno));
        goto finished;
    }

    if( bytes_sent > 0 ) {
        ParodusPrint("Sent %d bytes (size of struct %d)\n", bytes_sent, (int) payload_size);
    }

    sleep(2);
finished:
    nn_shutdown(sock, 0);
    free(payload_bytes);
    return (bytes_sent == payload_size);
}

ssize_t hub_check_inbox(char **notification)
{
    char *msg = NULL;
    int sock;
    int rv;
    int msg_sz;
    ssize_t n_sz = -1;

    sock = nn_socket(AF_SP, NN_PULL);
    if( sock < 0 ) {
        ParodusError("NN parodus receive socket error %d\n", sock);
        return -1;
    }

    rv = nn_bind(sock, p2p_url);
    if( rv < 0 ) {
        ParodusError("NN parodus receive bind error %d\n", rv);
        n_sz = -2;
        goto finished;
    }

    do {
        msg_sz = nn_recv(sock, &msg, NN_MSG, 0); //NN_DONTWAIT);
        if( msg_sz < 0 ) {
            ParodusError("NN parodus receive error %d, %d(%s)\n", msg_sz, errno, strerror(errno));
            n_sz = -3;
        } else {
            break;
        }
    } while( EAGAIN != errno );

    if( msg_sz > 0 ) {
        wrp_msg_t *wrp_msg;
        int wrp_msg_sz;

        wrp_msg_sz = wrp_to_struct(msg, msg_sz, WRP_BYTES, &wrp_msg);
        if( 0 >= wrp_msg_sz ) {
            return -4;
        }
    
        if( WRP_MSG_TYPE__EVENT != wrp_msg->msg_type ) {
            wrp_free_struct(wrp_msg);
            return -5;
        }
        
        n_sz = wrp_msg->u.event.payload_size + 1;
        *notification = strdup(wrp_msg->u.event.payload);
        wrp_free_struct(wrp_msg);
    }
    nn_freemsg(msg);

finished:
    nn_shutdown(sock, 0);
    return n_sz;
}

ssize_t spoke_check_inbox(char **notification)
{
    char *msg = NULL;
    int sock;
    int rv;
    int msg_sz;
    ssize_t n_sz = -1;

    sock = nn_socket(AF_SP, NN_SUB);
    if( sock < 0 ) {
        ParodusError("NN parodus receive socket error %d\n", sock);
        return -1;
    }

    /* Subscribe to everything ("" means all topics) */
    rv = nn_setsockopt(sock, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
    if( 0 > rv ) {
        ParodusError("NN parodus receive set socket opt error %d\n", sock);
        return -2;
    }

    rv = nn_connect(sock, p2p_url);
    if( rv < 0 ) {
        ParodusError("NN parodus receive bind error %d\n", rv);
        n_sz = -3;
        goto finished;
    }

    do {
        msg_sz = nn_recv(sock, &msg, NN_MSG, 0); // NN_DONTWAIT);
        if( msg_sz < 0 ) {
            ParodusError("NN parodus receive error %d, %d(%s)\n", msg_sz, errno, strerror(errno));
            n_sz = -4;
        } else {
            break;
        }
    } while( true );

    if( msg_sz > 0 ) {
        wrp_msg_t *wrp_msg;
        int wrp_msg_sz;

        wrp_msg_sz = wrp_to_struct(msg, msg_sz, WRP_BYTES, &wrp_msg);
        if( 0 >= wrp_msg_sz ) {
            return -5;
        }

        if( WRP_MSG_TYPE__EVENT != wrp_msg->msg_type ) {
            wrp_free_struct(wrp_msg);
            return -6;
        }

        n_sz = wrp_msg->u.event.payload_size;
        *notification = strdup(wrp_msg->u.event.payload);
        wrp_free_struct(wrp_msg);
    }
    nn_freemsg(msg);

finished:
    nn_shutdown(sock, 0);
    return n_sz;
}

bool hub_send_msg(const char *url, const char *notification, size_t notification_size)
{
    wrp_msg_t *msg;
    int sock;
    int rv;
    int bytes_sent = 0;
    int payload_size;
    int t = 2000;
    char *payload_bytes;

    msg = (wrp_msg_t *) malloc(sizeof(wrp_msg_t));
    memset(msg, 0, sizeof(wrp_msg_t));

    msg->msg_type = WRP_MSG_TYPE__EVENT;
    msg->u.event.content_type = "application/msgpack";
    msg->u.event.source  = (char *) p2p_url;
    msg->u.event.dest    = (char *) url;
    msg->u.event.payload = (char *) notification;
    msg->u.event.payload_size = notification_size;

    payload_size = wrp_struct_to( (const wrp_msg_t *) msg, WRP_BYTES, (void **) &payload_bytes);
    free(msg);

    sock = nn_socket(AF_SP, NN_PUB);
    if( sock < 0 ) {
        ParodusError("NN parodus send socket error %d\n", sock);
        goto finished;
    }

    rv = nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
    if( rv < 0 ) {
        ParodusError("NN parodus send timeout setting error %d\n", rv);
        goto finished;
    }

    rv = nn_bind(sock, url);
    if( rv < 0 ) {
        ParodusError("NN parodus send connect error %d\n", rv);
        goto finished;
    }

    bytes_sent = nn_send(sock, payload_bytes, payload_size, 0);
    if( bytes_sent < 0 ) {
        ParodusError("NN send msg - bytes_sent = %d, %d(%s)\n", bytes_sent, errno, strerror(errno));
        goto finished;
    }

    if( bytes_sent > 0 ) {
        ParodusPrint("Sent %d bytes (size of struct %d)\n", bytes_sent, (int) payload_size);
    }

    sleep(2);
finished:
    nn_shutdown(sock, 0);
    free(payload_bytes);
    return (bytes_sent == payload_size);
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* None */
