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
/* None */

/*----------------------------------------------------------------------------*/
/*                             Internal Functions                             */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                             External functions                             */
/*----------------------------------------------------------------------------*/
bool spoke_setup(const char *pipeline_url, const char *pubsub_url, const char **topics, int *pipeline_sock, int *pubsub_sock)
{
    int sock;
    int rv;
    int t = 2000;

    sock = nn_socket(AF_SP, NN_SUB);
    printf("nn_sub sock = %d\n", sock);
    if( sock < 0 ) {
        ParodusError("NN spoke sub socket error %d, %d(%s)\n", sock, errno, strerror(errno));
        return false;
    }

    /* Subscribe to everything ("" means all topics) */
    rv = nn_setsockopt(sock, NN_SUB, NN_SUB_SUBSCRIBE, "", 0); (void) topics; // ignoring topics for now
    if( 0 > rv ) {
        ParodusError("NN spoke sub socket topics setting error %d, %d(%s)\n", sock, errno, strerror(errno));
        nn_close(sock);
        return false;
    }

    rv = nn_connect(sock, pubsub_url);
    if( rv < 0 ) {
        ParodusError("NN spoke sub socket %d bind error %d, %d(%s)\n", sock, rv, errno, strerror(errno));
        goto finished;
    }

    *pubsub_sock = sock;

    sock = nn_socket(AF_SP, NN_PUSH);
    printf("nn_push sock = %d\n", sock);
    if( sock < 0 ) {
        ParodusError("NN spoke push socket error %d, %d(%s)\n", sock, errno, strerror(errno));
        return false;
    }

    rv = nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
    if( rv < 0 ) {
        ParodusError("NN spoke push socket timeout setting error %d, %d(%s)\n", rv, errno, strerror(errno));
        nn_close(sock);
        return false;
    }

    rv = nn_connect(sock, pipeline_url);
    if( rv < 0 ) {
        ParodusError("NN spoke push socket connect error %d, %d(%s)\n", rv, errno, strerror(errno));
        goto finished;
    }

    *pipeline_sock = sock;
    return true;

finished:
    nn_shutdown(sock, rv);
    nn_close(sock);

    return false;
}

bool hub_setup(const char *pipeline_url, const char *pubsub_url, int *pipeline_sock, int *pubsub_sock)
{
    int sock;
    int rv;
    int t = 2000;

    sock = nn_socket(AF_SP, NN_PULL);
    printf("nn_pull sock = %d\n", sock);
    if( sock < 0 ) {
        ParodusError("NN hub pull socket error %d, %d(%s)\n", sock, errno, strerror(errno));
        return false;
    }

    rv = nn_bind(sock, pipeline_url);
    if( rv < 0 ) {
        ParodusError("NN hub pull socket %d bind error %d, %d(%s)\n", sock, rv, errno, strerror(errno));
        nn_close(sock);
        return false;
    }

    *pipeline_sock = sock;

    sock = nn_socket(AF_SP, NN_PUB);
    printf("nn_pub sock = %d\n", sock);
    if( sock < 0 ) {
        ParodusError("NN hub pub socket error %d, %d(%s)\n", sock, errno, strerror(errno));
        return false;
    }

    rv = nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
    if( rv < 0 ) {
        ParodusError("NN hub pub socket timeout setting error %d, %d(%s)\n", rv, errno, strerror(errno));
        nn_close(sock);
    }

    rv = nn_bind(sock, pubsub_url);
    if( rv < 0 ) {
        ParodusError("NN parodus send connect error %d, %d(%s)\n", rv, errno, strerror(errno));
        goto finished;
    }

    *pubsub_sock = sock;
    return true;

finished:
    nn_shutdown(sock, rv);
    nn_close(sock);

    return false;
}

void sock_cleanup(int sock)
{
    nn_shutdown(sock, 0);
    nn_close(sock);
}

bool send_msg(int sock, const void *notification, size_t notification_size)
{
    int bytes_sent = 0;
    
    bytes_sent = nn_send(sock, notification, notification_size, 0);
    sleep(5);
    if( bytes_sent < 0 ) {
        ParodusError("Send msg - bytes_sent = %d, %d(%s)\n", bytes_sent, errno, strerror(errno));
    }

    if( bytes_sent > 0 ) {
        ParodusPrint("Sent %d bytes (size of struct %d)\n", bytes_sent, (int) notification_size);
    }

    return (bytes_sent == (int) notification_size);
}

ssize_t check_inbox(int sock, void **notification)
{
    char *msg = NULL;
    int msg_sz = 0;

    msg_sz = nn_recv(sock, &msg, NN_MSG, NN_DONTWAIT);
    sleep(5);
    if( msg_sz < 0 && errno != EAGAIN ) {
        ParodusError("Receive error %d, %d(%s)\n", msg_sz, errno, strerror(errno));
    } 
    if( msg_sz > 0 ) {
       *notification = msg;
    }

    return msg_sz;
}

void free_msg(void *msg)
{
    nn_freemsg(msg);
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* None */
