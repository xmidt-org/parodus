#include <stdbool.h>
#include <nopoll.h>
#include <nopoll_private.h>

#include "../src/ParodusInternal.h"
#include "mock_connection.h"

void parStrncpy(char *destStr, const char *srcStr, size_t destSize)
{
    (void) destStr; (void) srcStr; (void) destSize;
}

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

ParodusCfg *get_parodus_cfg(void)
{
    return &gPC;
}

void set_global_conn(noPollConn *conn)
{
    (void) conn;
}

noPollConn *get_global_conn(void)
{
    return &gNPConn;
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

void getCurrentTime(struct timespec *timer)
{
    (void) timer;
}

long timeValDiff(struct timespec *starttime, struct timespec *finishtime)
{
    (void) starttime; (void) finishtime;
    return 0L;
}


