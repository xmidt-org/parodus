/**
 * Copyright 2022 Comcast Cable Communications Management, LLC
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
 * @file xmidtsend_rbus.h
 *
 * @description This header defines functions required to manage xmidt send messages via rbus.
 *
 */
 
#ifndef _XMIDTSEND_RBUS_H_
#define _XMIDTSEND_RBUS_H_
#include <rbus.h>
#include "config.h"
#include <uuid/uuid.h>
#ifdef __cplusplus
extern "C" {
#endif

#define XMIDT_SEND_METHOD "Device.X_RDK_Xmidt.SendData"
#define INPARAMS_PATH   "/tmp/inparams.txt"

#define CLOUD_ACK_TIMEOUT_SEC       7
#define CRITICAL_QOS_EXPIRE_TIME    30*60
#define HIGH_QOS_EXPIRE_TIME        25*60
#define MEDIUM_QOS_EXPIRE_TIME      20*60
#define LOW_QOS_EXPIRE_TIME         15*60

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct XmidtMsg__
{
	wrp_msg_t *msg;
	rbusMethodAsyncHandle_t asyncHandle;
	int state;
	long long enqueueTime;
	long long sentTime;
	struct XmidtMsg__ *next;
} XmidtMsg;

typedef struct CloudAck__
{
	char *transaction_id;
	int qos;
	int rdr;
	struct CloudAck__ *next;
} CloudAck;

typedef enum
{
    DELIVERED_SUCCESS = 0,
    INVALID_MSG_TYPE,
    MISSING_SOURCE,
    MISSING_DEST,
    MISSING_CONTENT_TYPE,
    MISSING_PAYLOAD,
    MISSING_PAYLOADLEN,
    INVALID_CONTENT_TYPE,
    ENQUEUE_FAILURE = 100,
    CLIENT_DISCONNECT = 101,
    QUEUE_SIZE_EXCEEDED = 102,
    WRP_ENCODE_FAILURE = 103,
    MSG_PROCESSING_FAILED = 104
} XMIDT_STATUS;

typedef enum
{
    PENDING = 0,
    SENT,
    DELETE
} MSG_STATUS;
/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

rbusHandle_t get_parodus_rbus_Handle(void);
void addToXmidtUpstreamQ(wrp_msg_t * msg, rbusMethodAsyncHandle_t asyncHandle);
void* processXmidtUpstreamMsg();
void processXmidtData();
int processData(XmidtMsg *Datanode, wrp_msg_t * msg, rbusMethodAsyncHandle_t asyncHandle);
void sendXmidtEventToServer(XmidtMsg *msgnode, wrp_msg_t * msg, rbusMethodAsyncHandle_t asyncHandle);
int checkInputParameters(rbusObject_t inParams);
char* generate_transaction_uuid();
void parseRbusInparamsToWrp(rbusObject_t inParams, char *trans_id, wrp_msg_t **eventMsg);
void createOutParamsandSendAck(wrp_msg_t *msg, rbusMethodAsyncHandle_t asyncHandle, char *errorMsg, int statuscode, rbusError_t error);
int validateXmidtData(wrp_msg_t * eventMsg, char **errorMsg, int *statusCode);
void printSendMsgData(char* status, int qos, char* dest, char* transaction_uuid);
bool highQosValueCheck(int qos);
void waitTillConnectionIsUp();
void printRBUSParams(rbusObject_t params, char* file_path);
void addToCloudAckQ(char *transaction_id, int qos, int rdr);
int checkCloudACK(XmidtMsg *xmdnode, rbusMethodAsyncHandle_t asyncHandle);
int updateXmidtState(XmidtMsg * temp, int state);
void print_xmidMsg_list();
int deleteCloudACKNode(char* trans_id);
int deleteFromXmidtQ(XmidtMsg **next_node);
int checkCloudConn();
void checkMaxQandOptimize();
void checkMsgExpiry();
void mapXmidtStatusToStatusMessage(int status, char **message);
#ifdef __cplusplus
}
#endif


#endif /* _XMIDTSEND_RBUS_H_ */

