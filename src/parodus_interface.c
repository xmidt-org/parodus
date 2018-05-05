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
static int g_spk_sock;
static int g_hub_sock;

/*----------------------------------------------------------------------------*/
/*                             Internal Functions                             */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                             External functions                             */
/*----------------------------------------------------------------------------*/
bool spoke_setup_listener(const char *url)
{
    int rv;

    g_spk_sock = nn_socket(AF_SP, NN_SUB);
    if( g_spk_sock < 0 ) {
        ParodusError("NN parodus receive socket error %d, %d(%s)\n", g_spk_sock, errno, strerror(errno));
        return false;
    }

    /* Subscribe to everything ("" means all topics) */
    rv = nn_setsockopt(g_spk_sock, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
    if( 0 > rv ) {
        ParodusError("NN parodus receive set socket opt error %d, %d(%s)\n", g_spk_sock, errno, strerror(errno));
        return false;
    }

    rv = nn_connect(g_spk_sock, url);
    if( rv < 0 ) {
        ParodusError("NN parodus receive bind error %d, %d(%s)\n", rv, errno, strerror(errno));
        nn_shutdown(g_spk_sock, 0);
        return false;
    }
    return true;
}

void spoke_cleanup_listener(void)
{
    nn_close(g_spk_sock);
    nn_shutdown(g_spk_sock, 0);
}

bool hub_setup_listener(const char *url)
{
    int rv;

    g_hub_sock = nn_socket(AF_SP, NN_PULL);
    if( g_hub_sock < 0 ) {
        ParodusError("NN parodus receive socket error %d\n", g_hub_sock, errno, strerror(errno));
        return false;
    }

    rv = nn_bind(g_hub_sock, url);
    if( rv < 0 ) {
        ParodusError("NN parodus receive bind error %d\n", rv, errno, strerror(errno));
        return false;
    }

    return true;
}

void hub_cleanup_listener(void)
{
    nn_close(g_hub_sock);
    nn_shutdown(g_hub_sock, 0);
}

bool spoke_send_msg(const char *url, const void *notification, size_t notification_size)
{
    int sock;
    int rv;
    int bytes_sent = 0;
    int t = 2000;
    
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

    do {
        bytes_sent = nn_send(sock, notification, notification_size, NN_DONTWAIT);
    } while( EAGAIN == errno );
    if( bytes_sent < 0 ) {
        ParodusError("Spoke send msg - bytes_sent = %d, %d(%s)\n", bytes_sent, errno, strerror(errno));
        goto finished;
    }

    if( bytes_sent > 0 ) {
        ParodusPrint("Sent %d bytes (size of struct %d)\n", bytes_sent, (int) notification_size);
    }

    sleep(2);
finished:
    nn_close(sock);
    nn_shutdown(sock, 0);
    return (bytes_sent == (int) notification_size);
}

ssize_t hub_check_inbox(void **notification)
{
    char *msg = NULL;
    int msg_sz = 0;

    do {
        msg_sz = nn_recv(g_hub_sock, &msg, NN_MSG, NN_DONTWAIT);
        if( msg_sz < 0 && EAGAIN != errno ) {
            ParodusError("Hub parodus receive error %d, %d(%s)\n", msg_sz, errno, strerror(errno));
            return -1;
        } else {
            break;
        }
    } while( EAGAIN == errno );

    if( msg_sz > 0 ) {
        *notification = malloc(msg_sz);
        memcpy(*notification, msg, msg_sz);
        nn_freemsg(msg);
    }

    return msg_sz;
}

ssize_t spoke_check_inbox(void **notification)
{
    char *msg = NULL;
    int msg_sz = 0;

    msg_sz = nn_recv(g_spk_sock, &msg, NN_MSG, 0); //NN_DONTWAIT);
    if( msg_sz < 0 ) {
        ParodusError("Spoke parodus receive error %d, %d(%s)\n", msg_sz, errno, strerror(errno));
        return msg_sz;
    }

    if( msg_sz > 0 ) {
        *notification = malloc(msg_sz);
        memcpy(*notification, msg, msg_sz);
        nn_freemsg(msg);
    }

    return msg_sz;
}

bool hub_send_msg(const char *url, const void *notification, size_t notification_size)
{
    int sock;
    int rv;
    int bytes_sent = 0;
    int t = 2000;

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

    sleep(2);
    do {
        bytes_sent = nn_send(sock, notification, notification_size, NN_DONTWAIT);
    } while( bytes_sent != (int ) notification_size );// nn_send always returning errno as EAGAIN even in success case
    if( bytes_sent < 0 ) {
        ParodusError("Hub send msg - bytes_sent = %d, %d(%s)\n", bytes_sent, errno, strerror(errno));
        goto finished;
    }

    if( bytes_sent > 0 ) {
        ParodusInfo("Sent %d bytes (size of struct %d)\n", bytes_sent, (int) notification_size);
    }

    sleep(2);
finished:
    nn_close(sock);
    nn_shutdown(sock, rv);
    return (bytes_sent == (int) notification_size);
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* None */
