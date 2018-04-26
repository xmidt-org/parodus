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
#include "config.h"
#include "parodus_interface.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* None */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct ll_node {
    char *key;
    char *value;
    struct ll_node *next;
} ll_t;

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static ll_t *head = NULL;
static pthread_mutex_t ll_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_execute = true;

/*----------------------------------------------------------------------------*/
/*                             Internal Functions                             */
/*----------------------------------------------------------------------------*/
static void *listen_to_spokes(void *args);
static void process_spoke_msg(const char *msg, size_t msg_sz);
static void ll_clean(ll_t *list);

/*----------------------------------------------------------------------------*/
/*                             External functions                             */
/*----------------------------------------------------------------------------*/
int start_listening_to_spokes(pthread_t *thread, const char *url)
{
    pthread_t t, *p;
    pthread_mutex_init( &ll_mutex, NULL );
    int rv;

    p = &t;
    if( NULL != thread ) {
        p = thread;
    }

    rv = pthread_create( p, NULL, listen_to_spokes, (void*) url );
    if( 0 != rv ) {
        pthread_mutex_destroy(&ll_mutex);
    }

    return rv;
}

bool send_to_hub(const char *url, const char *notification, size_t notification_size)
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
    msg->u.event.source  = get_parodus_cfg()->local_url;
    msg->u.event.dest    = (char *) url;
    msg->u.event.payload = (char *) notification;
    msg->u.event.payload_size = notification_size;

    payload_size = wrp_struct_to( (const wrp_msg_t *) msg, WRP_BYTES, (void **) &payload_bytes);
    free(msg);
    if( 1 > payload_size ) {
        ParodusError("NN payload size %d\n", payload_size);
        return false;
    }
    
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
    if( bytes_sent > 0 ) {
        ParodusPrint("Sent %d bytes (size of struct %d)\n", bytes_sent, (int) payload_size);
    }

finished:
    nn_shutdown(sock, 0);
    
    free(payload_bytes);

    return (bytes_sent == (int) payload_size);
}

ssize_t receive_from_spoke(const char *url, char **msg)
{
    ll_t *current;
    ll_t *next;

    *msg = NULL;
    pthread_mutex_lock(&ll_mutex);
    current = head;
    while( NULL != current ) {
        if( 0 == strcmp(url, current->key) ) {
            *msg = strdup(current->value);
            free(current->key);
            free(current->value);
            next = current->next;
            free(current);
            current = next;
            pthread_mutex_unlock(&ll_mutex);
            return strlen(*msg) + 1;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&ll_mutex);

    return 0;
}

void stop_listening_to_spokes(void)
{
    g_execute = false;
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
static void *listen_to_spokes(void *args)
{
    const char *url;
    char *msg = NULL;
    int sock;
    int rv;
    int msg_sz;

    url = (const char *) args;

    sock = nn_socket(AF_SP, NN_PULL);
    if( sock < 0 ) {
        ParodusError("NN parodus receive socket error %d\n", sock);
        return NULL;
    }

    rv = nn_bind(sock, url);
    if( rv < 0 ) {
        ParodusError("NN parodus receive bind error %d\n", rv);
        goto finished;
    }

    while( true == g_execute ) {
        msg_sz = nn_recv(sock, &msg, NN_MSG, 0);
        if( msg_sz < 0 ) {
            ParodusError("NN parodus receive error %d\n", msg_sz);
            break;
        } else if( msg_sz > 0 ) {
            process_spoke_msg(msg, msg_sz);
        }
        nn_freemsg(msg);
    }

    sleep(1); // wait for messages to flush before shutting down

finished:
    rv = nn_shutdown(sock, 0);
    pthread_mutex_lock(&ll_mutex);
    ll_clean(head);
    pthread_mutex_unlock(&ll_mutex);
    pthread_mutex_destroy(&ll_mutex);
    return NULL;
}

static void process_spoke_msg(const char *msg, size_t msg_sz)
{
    wrp_msg_t *wrp_msg;
    int wrp_msg_sz;
    ll_t *current;
    ll_t *prev;
    
    wrp_msg_sz = wrp_to_struct(msg, msg_sz, WRP_BYTES, &wrp_msg);
    if( 0 >= wrp_msg_sz ) {
        return;
    }

    if( WRP_MSG_TYPE__EVENT != wrp_msg->msg_type ) {
        wrp_free_struct(wrp_msg);
        return;
    }

    pthread_mutex_lock(&ll_mutex);
    current = head;
    while( NULL != current ) {
        if( 0 == strcmp(wrp_msg->u.event.source, current->key) ) {
            free(current->value);
            current->value = strdup(wrp_msg->u.event.payload);
            pthread_mutex_unlock(&ll_mutex);
            wrp_free_struct(wrp_msg);
            return;
        }
        prev = current;
        current = current->next;
    }

    /* TODO: Validate URL of spoke parodus before adding. */
    current = (ll_t *) malloc(sizeof(ll_t));
    current->key = strdup(wrp_msg->u.event.source);
    current->value = strdup(wrp_msg->u.event.payload);
    current->next = NULL;

    if( NULL == head ) {
        head = current;
    } else {
        prev->next = current;
    }

    pthread_mutex_unlock(&ll_mutex);
    wrp_free_struct(wrp_msg);
}

static void ll_clean(ll_t *list)
{
    ll_t *current = list;
    ll_t *next;

    while( NULL != current ) {
        free(current->key);
        free(current->value);

        next = current->next;
        free(current);
        current = next;
    }
    list = NULL;
}
