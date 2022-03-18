/**
 *  Copyright 2010-2016 Comcast Cable Communications Management, LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <malloc.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <nopoll.h>
#include <wrp-c.h>
#include <nanomsg/nn.h>

#include "../src/upstream.h"
#include "../src/config.h"
#include "../src/client_list.h"
#include "../src/ParodusInternal.h"
#include "../src/partners_check.h"
#include "../src/close_retry.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
#define GOOD_CLIENT_URL "tcp://127.0.0.1:6667"

static noPollConn *conn;
static char *reconnect_reason = "webpa_process_starts";
bool g_shutdown  = false;
static ParodusCfg parodusCfg;
extern size_t metaPackSize;
extern UpStreamMsg *UpStreamMsgQ;
int numLoops = 1;
int deviceIDNull =0;
char webpa_interface[64]={'\0'};
wrp_msg_t *reg_msg = NULL;
extern pthread_mutex_t nano_mut;
extern pthread_cond_t nano_con;
pthread_mutex_t config_mut=PTHREAD_MUTEX_INITIALIZER;
/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
reg_list_item_t *get_reg_list()
{
  reg_list_item_t *item = get_global_node();
  release_global_node();
  return item;
}

noPollConn *get_global_conn()
{
    return conn;   
}

char *get_global_reconnect_reason()
{
    return reconnect_reason;
}

void addCRUDmsgToQueue(wrp_msg_t *crudMsg)
{
	(void)crudMsg;
	function_called();
	return;
}

void sendMessage(noPollConn *conn, void *msg, size_t len)
{
    (void) conn; (void) msg; (void) len;
    function_called();
}

void set_parodus_cfg(ParodusCfg *cfg)
{
    memcpy(&parodusCfg, cfg, sizeof(ParodusCfg));
}

ParodusCfg *get_parodus_cfg(void) 
{
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	parStrncpy(cfg.hw_mac , "14cfe2142xxx", sizeof(cfg.hw_mac));
	if(deviceIDNull)
	{
		parStrncpy(cfg.hw_mac , "", sizeof(cfg.hw_mac));
	}
	set_parodus_cfg(&cfg);
    return &parodusCfg;
}

char *getWebpaInterface(void)
{
    ParodusCfg cfg;
    memset(&cfg,0,sizeof(cfg));
    #ifdef WAN_FAILOVER_SUPPORTED
        parStrncpy(cfg.webpa_interface_used , "wl0", sizeof(cfg.webpa_interface_used));
    #else
        parStrncpy(cfg.webpa_interface_used , "eth0", sizeof(cfg.webpa_interface_used));
    #endif
    set_parodus_cfg(&cfg);
    #ifdef WAN_FAILOVER_SUPPORTED	
	ParodusPrint("WAN_FAILOVER_SUPPORTED mode \n");
	pthread_mutex_lock (&config_mut);	
	parStrncpy(webpa_interface, get_parodus_cfg()->webpa_interface_used, sizeof(webpa_interface));
	pthread_mutex_unlock (&config_mut);
    #else
	parStrncpy(webpa_interface, get_parodus_cfg()->webpa_interface_used, sizeof(webpa_interface));
    #endif
	return webpa_interface;
}

/*-------------------------------------------
int nn_connect (int s, const char *addr)
{
    (void) s; (void) addr;
    printf ("nn_connect, socket %d\n", s);
    return 1;
}
---------------------------------------------*/

ssize_t wrp_pack_metadata( const data_t *packData, void **data )
{
    (void) packData; (void) data;
    function_called();
    
    return (ssize_t)mock();
}

size_t appendEncodedData( void **appendData, void *encodedBuffer, size_t encodedSize, void *metadataPack, size_t metadataSize )
{
    (void) encodedBuffer; (void) encodedSize; (void) metadataPack; (void) metadataSize;
    function_called();
    char *data = (char *) malloc (sizeof(char) * 100);
    parStrncpy(data, "AAAAAAAAYYYYIGkYTUYFJH", 100);
    *appendData = data;
    return (size_t)mock();
}

int nn_send (int s, const void *buf, size_t len, int flags)
{
    UNUSED(s); UNUSED(buf); UNUSED(len); UNUSED(flags);
    function_called();
    return (int)mock();
}

int pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex)
{
    UNUSED(cond); UNUSED(mutex);
    function_called();
    return (int)mock();
}

ssize_t wrp_to_struct( const void *bytes, const size_t length, const enum wrp_format fmt, wrp_msg_t **msg )
{
    UNUSED(bytes); UNUSED(length); UNUSED(fmt);
    function_called();
    *msg = reg_msg;
    return (ssize_t)mock();
}

void wrp_free_struct( wrp_msg_t *msg )
{
    UNUSED(msg);
    function_called();
}

int nn_freemsg (void *msg)
{
    UNUSED(msg);
    function_called();
    return (int)mock();
}

int validate_partner_id(wrp_msg_t *msg, partners_t **partnerIds)
{
    UNUSED(msg); UNUSED(partnerIds);
    function_called();
    return (int) mock();
}

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/


void test_message()
{
    metaPackSize = 20;
    UpStreamMsgQ = (UpStreamMsg *) malloc(sizeof(UpStreamMsg));
    UpStreamMsgQ->msg = "First Message";
    UpStreamMsgQ->len = 13;
    UpStreamMsgQ->next = NULL;

    numLoops = 1;
    reg_msg = (wrp_msg_t *) malloc(sizeof(wrp_msg_t));
    memset(reg_msg,0,sizeof(wrp_msg_t));
    reg_msg->msg_type = WRP_MSG_TYPE__SVC_REGISTRATION;
    reg_msg->u.reg.service_name = "config";
    reg_msg->u.reg.url = GOOD_CLIENT_URL;

    will_return(wrp_to_struct, 12);
    expect_function_call(wrp_to_struct);

    will_return(nn_send, 1);
    expect_function_call(nn_send);

    will_return(nn_freemsg, 0);
    expect_function_call(nn_freemsg);
    expect_function_call(wrp_free_struct);

    processUpstreamMessage();
    free(reg_msg);
    free(UpStreamMsgQ);
}

void test_processUpstreamMessage()
{
  int last_sock = -1;
  reg_list_item_t * reg_item = get_reg_list ();

  assert_null (reg_item);
  test_message();
  reg_item = get_reg_list ();
  assert_non_null (reg_item);
  if (NULL == reg_item)
    return;

  last_sock = reg_item->sock;
  test_message ();
  assert_int_equal (get_numOfClients(), 1);
  if (get_numOfClients() != 1)
    return;
  reg_item = get_reg_list ();
  assert_int_equal (last_sock, reg_item->sock);
  if (last_sock != reg_item->sock)
    return;

  test_message ();
  assert_int_equal (get_numOfClients(), 1);
  if (get_numOfClients() != 1)
    return;
  reg_item = get_reg_list ();
  assert_int_equal (last_sock, reg_item->sock);

}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_processUpstreamMessage)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
