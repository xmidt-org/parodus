#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <cJSON.h>
#include <nopoll.h>
#include <sys/sysinfo.h>
#include "wss_mgr.h"
#include <pthread.h>
#include <math.h>
#include <nopoll.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <wrp-c.h>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>

#include "ParodusInternal.h"
#include "time.h"
#include "config.h"
#include "parodus_log.h"

#ifdef __cplusplus
extern "C" {
#endif
    
int createNopollConnection(noPollCtx *);
void close_and_unref_connection(noPollConn *);
int sendResponse(noPollConn * conn,void *str, size_t bufferSize);
void setMessageHandlers();

noPollConn *get_global_conn(void);
void set_global_conn(noPollConn *);

extern pthread_mutex_t g_mutex;
extern pthread_cond_t g_cond;
extern pthread_mutex_t close_mut;
extern bool close_retry;
extern bool LastReasonStatus;
extern char *reconnect_reason;

extern  volatile unsigned int heartBeatTimer;
extern  ParodusMsg *ParodusMsgQ;
extern UpStreamMsg *UpStreamMsgQ;

extern reg_list_item_t * head;
extern int numOfClients;


#ifdef __cplusplus
}
#endif
    
#endif
