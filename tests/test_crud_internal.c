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
#include <string.h>
#include <cJSON.h>
#include <unistd.h> 

#include <wrp-c.h>
#include "../src/crud_tasks.h"
#include "../src/config.h"
#include "../src/crud_internal.h"
#include "../src/connection.h"
#include "../src/close_retry.h"

char *get_global_reconnect_reason()
{
    return "parodus_stopping";
}

void set_global_reconnect_reason(char *reason)
{
	UNUSED(reason);
}

void set_global_reconnect_status(bool status)
{
	UNUSED(status);
}

void test_writeToJSON_Failure()
{
	int ret = -1;
	ret = writeToJSON(NULL);
	assert_int_equal (ret, 0);
}

void test_writeToJSON()
{
	int ret = -1;
	FILE *fp;
	ParodusCfg cfg;
    memset(&cfg,0,sizeof(cfg));
    cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	ret = writeToJSON("testData");
	assert_int_equal (ret, 1);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);
}

void test_readFromJSON_Failure()
{
	int ret = -1;
	ret = readFromJSON(NULL);
	assert_int_equal (ret, 0);
}

void test_readFromJSON()
{
	int write_ret = -1;
	int read_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	char *data = NULL;
	int ch_count = 0;

	ParodusCfg cfg;
    memset(&cfg,0,sizeof(cfg));
    cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata = strdup("testData");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	read_ret = readFromJSON(&data);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		fseek(fp, 0, SEEK_END);
		ch_count = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		data = (char *) malloc(sizeof(char) * (ch_count + 1));
		fread(data, 1, ch_count,fp);
		(data)[ch_count] ='\0';
		assert_string_equal (data, testdata);
		assert_int_equal (read_ret, 1);

		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	if(data !=NULL)
		free(data);
	if(testdata !=NULL)
		free(testdata);
}

void test_retrieveFromMemory()
{
	int ret = -1;
	cJSON *jsonresponse;
	ParodusCfg *cfg = NULL;
	cfg= (ParodusCfg *) malloc(sizeof(ParodusCfg));
 
    parStrncpy(cfg->hw_model, "X5001",sizeof(cfg->hw_model));
    parStrncpy(cfg->hw_serial_number, "Fer23u948590",sizeof(cfg->hw_model));
    parStrncpy(cfg->hw_manufacturer, "ARRISGroup,Inc.", sizeof(cfg->hw_manufacturer));
    parStrncpy(cfg->hw_mac, "123567892366",sizeof(cfg->hw_mac));
    parStrncpy(cfg->hw_last_reboot_reason, "unknown",sizeof(cfg->hw_last_reboot_reason));
    parStrncpy(cfg->fw_name, "TG1682_DEV_master_2016000000sdy", sizeof(cfg->fw_name));
    cfg->webpa_ping_timeout=180;
    parStrncpy(cfg->webpa_interface_used, "br0",sizeof(cfg->webpa_interface_used));   
    parStrncpy(cfg->webpa_url, "http://127.0.0.1", sizeof(cfg->webpa_url));
    parStrncpy(cfg->webpa_uuid, "1234567-345456546", sizeof(cfg->webpa_uuid));
    parStrncpy(cfg->webpa_protocol , "PARODUS-2.0", sizeof(cfg->webpa_protocol));
    cfg->webpa_backoff_max=0;
    cfg->boot_time=1234;
	set_parodus_cfg(cfg);
	ret = retrieveFromMemory("hw-model", &jsonresponse );
	assert_int_equal (ret, 0);
	ret = retrieveFromMemory("hw-serial-number", &jsonresponse );
	assert_int_equal (ret, 0);
	ret = retrieveFromMemory("hw-mac", &jsonresponse );
	assert_int_equal (ret, 0);
	ret = retrieveFromMemory("hw-manufacturer", &jsonresponse );
	assert_int_equal (ret, 0);
	ret = retrieveFromMemory("hw-last-reboot-reason", &jsonresponse );
	assert_int_equal (ret, 0);
	ret = retrieveFromMemory("fw-name", &jsonresponse );
	assert_int_equal (ret, 0);
	ret = retrieveFromMemory("webpa-ping-timeout", &jsonresponse );
	assert_int_equal (ret, 0);
	ret = retrieveFromMemory("boot-time", &jsonresponse );
	assert_int_equal (ret, 0);
	ret = retrieveFromMemory("webpa-uuid", &jsonresponse );
	assert_int_equal (ret, 0);
	ret = retrieveFromMemory("webpa-url", &jsonresponse );
	assert_int_equal (ret, 0);
	ret = retrieveFromMemory("webpa-protocol", &jsonresponse );
	assert_int_equal (ret, 0);
	ret = retrieveFromMemory("webpa-interface-used", &jsonresponse );
	assert_int_equal (ret, 0);
	ret = retrieveFromMemory("webpa-backoff-max", &jsonresponse );
	assert_int_equal (ret, 0);
	free(cfg);
}

void test_retrieveFromMemoryFailure()
{
	int ret = -1;
	cJSON *jsonresponse;
	ParodusCfg cfg;
	set_parodus_cfg(&cfg);
	ret = retrieveFromMemory("hw-model", &jsonresponse );
	assert_int_equal (ret, -1);
	ret = retrieveFromMemory("hw-serial-number", &jsonresponse );
	assert_int_equal (ret, -1);
	ret = retrieveFromMemory("hw-mac", &jsonresponse );
	assert_int_equal (ret, -1);
	ret = retrieveFromMemory("hw-manufacturer", &jsonresponse );
	assert_int_equal (ret, -1);
	ret = retrieveFromMemory("hw-last-reboot-reason", &jsonresponse );
	assert_int_equal (ret, -1);
	ret = retrieveFromMemory("fw-name", &jsonresponse );
	assert_int_equal (ret, -1);
	ret = retrieveFromMemory("webpa-ping-timeout", &jsonresponse );
	assert_int_equal (ret, 0);
	ret = retrieveFromMemory("boot-time", &jsonresponse );
	assert_int_equal (ret, 0);
	ret = retrieveFromMemory("webpa-uuid", &jsonresponse );
	assert_int_equal (ret, -1);
	ret = retrieveFromMemory("webpa-url", &jsonresponse );
	assert_int_equal (ret, -1);
	ret = retrieveFromMemory("webpa-protocol", &jsonresponse );
	assert_int_equal (ret, -1);
	ret = retrieveFromMemory("webpa-interface-used", &jsonresponse );
	assert_int_equal (ret, -1);
	ret = retrieveFromMemory("webpa-backoff-max", &jsonresponse );
	assert_int_equal (ret, 0);
	ret = retrieveFromMemory("webpa-invalid", &jsonresponse );
	assert_int_equal (ret, -1);
}

void test_createObjectInvalidReq()
{
    int ret = 0;
    wrp_msg_t *reqMsg = NULL;
    wrp_msg_t *respMsg = NULL;
    FILE *fp;
    ParodusCfg cfg;
    memset(&cfg,0,sizeof(cfg));
    cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);

	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
    reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
    memset(reqMsg, 0, sizeof(wrp_msg_t));

	reqMsg->msg_type = 5;
    reqMsg->u.crud.transaction_uuid = strdup("1234");
    reqMsg->u.crud.source = strdup("tag-update");
    reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag");
    respMsg->msg_type = 5;
    ret = createObject(reqMsg, &respMsg);
    assert_int_equal (respMsg->u.crud.status, 400);
    assert_int_equal (ret, -1);
    fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
   		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_createObjectInvalid_JsonEmpty()
{
	int ret = 0;
	FILE *fp;
	int write_ret = -1;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));

	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));

	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);

	write_ret = writeToJSON(NULL);
	assert_int_equal (write_ret, 0);
	reqMsg->msg_type = 5;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag");
	respMsg->msg_type = 5;
	ret = createObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 400);

	assert_int_equal (ret, -1);

	if(cfg.crud_config_file !=NULL)
	free(cfg.crud_config_file);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}


void test_createObjectInvalid_JsonNonEmpty()
{
	int ret = 0;
	FILE *fp;
	int write_ret = -1;
	char * testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));

	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);

	testdata = strdup("testData");	
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);

	reqMsg->msg_type = 5;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag");
	respMsg->msg_type = 5;
	ret = createObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 500);
	assert_int_equal (ret, -1);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}


void test_createObjectInvalid_JsonParseErr()
{
	int ret = 0;
	FILE *fp;
	int write_ret = -1;
	char * testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
	memset(reqMsg, 0, sizeof(wrp_msg_t));

	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));

	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);

	testdata = strdup("{testData:{");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);

	reqMsg->msg_type = 5;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag");
	respMsg->msg_type = 5;
	ret = createObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 500);
	assert_int_equal (ret, -1);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_createObject_destNull()
{
    int ret = 0;
    int write_ret = -1;
    FILE *fp;
    wrp_msg_t *reqMsg = NULL;
    reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
    memset(reqMsg, 0, sizeof(wrp_msg_t));
    wrp_msg_t *respMsg = NULL;
    respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
    memset(respMsg, 0, sizeof(wrp_msg_t));
    ParodusCfg cfg;
    memset(&cfg,0,sizeof(cfg));
    cfg.crud_config_file = strdup("parodus_cfg.json");
    set_parodus_cfg(&cfg);
    write_ret = writeToJSON(NULL);
    assert_int_equal (write_ret, 0);    
    reqMsg->msg_type = 5;
    reqMsg->u.crud.transaction_uuid = strdup("1234");
    reqMsg->u.crud.source = strdup("tag-update");
    respMsg->msg_type = 5;
    ret = createObject(reqMsg, &respMsg);

    assert_int_equal (respMsg->u.crud.status, 400);
    assert_int_equal (ret, -1);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
   		free(cfg.crud_config_file);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_createObject_JsonParse()
{
  int ret = 0;
  int write_ret = -1;
  FILE *fp;
  char *testdata = NULL;
 
  wrp_msg_t *reqMsg = NULL;
  reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
  memset(reqMsg, 0, sizeof(wrp_msg_t));    
 
  wrp_msg_t *respMsg = NULL;
  respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
  memset(respMsg, 0, sizeof(wrp_msg_t));    
 
  ParodusCfg cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.crud_config_file = strdup("parodus_cfg.json");
  set_parodus_cfg(&cfg);     
  testdata=strdup("{ \"expires\" : 1522451870 }");  
  write_ret = writeToJSON(testdata);
  assert_int_equal (write_ret, 1);    
  reqMsg->msg_type = 5;
  reqMsg->u.crud.transaction_uuid = strdup("1234");
  reqMsg->u.crud.source = strdup("tag-update");
  reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test");
  
  respMsg->msg_type = 5;
  ret = createObject(reqMsg, &respMsg);
 
  assert_int_equal (respMsg->u.crud.status, 400);
  assert_int_equal (ret, -1);
 
   fp = fopen(cfg.crud_config_file, "r");
   if (fp != NULL)
   {
       system("rm parodus_cfg.json");
       fclose(fp);
   }  
   if(cfg.crud_config_file !=NULL)
   free(cfg.crud_config_file);
   
   wrp_free_struct(reqMsg);
   wrp_free_struct(respMsg);
  
}

void test_UnsupportedDestination()
{
  int ret = 0;
  int write_ret = -1;
  FILE *fp;
  char *testdata = NULL;

  wrp_msg_t *reqMsg = NULL;
  reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(reqMsg, 0, sizeof(wrp_msg_t));

  wrp_msg_t *respMsg = NULL;
  respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(respMsg, 0, sizeof(wrp_msg_t));

  ParodusCfg cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.crud_config_file = strdup("parodus_cfg.json");
  set_parodus_cfg(&cfg);
  testdata=strdup("{ \"expires\" : 1522451870 }");
  write_ret = writeToJSON(testdata);
  assert_int_equal (write_ret, 1);
  reqMsg->msg_type = 5;
  reqMsg->u.crud.transaction_uuid = strdup("1234");
  reqMsg->u.crud.source = strdup("tag-update");
  reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags/test1/test2");
  reqMsg->u.crud.payload = strdup("{ \"expires\" : 1522451870 }");

  respMsg->msg_type = 5;
  ret = createObject(reqMsg, &respMsg);
  assert_int_equal (respMsg->u.crud.status, 400);
  assert_int_equal (ret, -1);


   fp = fopen(cfg.crud_config_file, "r");
   if (fp != NULL)
   {
       system("rm parodus_cfg.json");
       fclose(fp);
   }
   if(cfg.crud_config_file !=NULL)
   free(cfg.crud_config_file);

   wrp_free_struct(reqMsg);
   wrp_free_struct(respMsg);

} 

void test_createObject_withProperPayload()
{
  int ret = 0;
  int write_ret = -1;
  FILE *fp;
  char *testdata = NULL;
 
  wrp_msg_t *reqMsg = NULL;
  reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
  memset(reqMsg, 0, sizeof(wrp_msg_t));    
 
  wrp_msg_t *respMsg = NULL;
  respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
  memset(respMsg, 0, sizeof(wrp_msg_t));    
 
  ParodusCfg cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.crud_config_file = strdup("parodus_cfg.json");
  set_parodus_cfg(&cfg);     
  testdata=strdup("{ \"expires\" : 1522451870 }");  
  write_ret = writeToJSON(testdata);
  assert_int_equal (write_ret, 1);    
  reqMsg->msg_type = 5;
  reqMsg->u.crud.transaction_uuid = strdup("1234");
  reqMsg->u.crud.source = strdup("tag-update");
  reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test");
  reqMsg->u.crud.payload = strdup("{ \"expires\" : 1522451870, \"key1\":\"value1\", \"key2\":1223 }");
  
  respMsg->msg_type = 5;
  ret = createObject(reqMsg, &respMsg);
 
  assert_int_equal (respMsg->u.crud.status, 201);
  assert_int_equal (ret, 0);
 
  
   fp = fopen(cfg.crud_config_file, "r");
   if (fp != NULL)
   {
       system("rm parodus_cfg.json");
       fclose(fp);
   }  
   if(cfg.crud_config_file !=NULL)
   free(cfg.crud_config_file);
   
   wrp_free_struct(reqMsg);
   wrp_free_struct(respMsg);
  
} 

void test_createObject_InvalidExpiresKey()
{
  int ret = 0;
  int write_ret = -1;
  FILE *fp;
  char *testdata = NULL;
  wrp_msg_t *reqMsg = NULL;
  reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(reqMsg, 0, sizeof(wrp_msg_t));
  wrp_msg_t *respMsg = NULL;
  respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(respMsg, 0, sizeof(wrp_msg_t));
  ParodusCfg cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.crud_config_file = strdup("parodus_cfg.json");
  set_parodus_cfg(&cfg);
  testdata=strdup("{ \"expires\" : 1522451870 }");
  write_ret = writeToJSON(testdata);
  assert_int_equal (write_ret, 1);
  reqMsg->msg_type = 5;
  reqMsg->u.crud.transaction_uuid = strdup("1234");
  reqMsg->u.crud.source = strdup("tag-update");
  reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test");
  reqMsg->u.crud.payload = strdup("{ \"expires\" : 0 }");

  respMsg->msg_type = 5;
  ret = createObject(reqMsg, &respMsg);
  assert_int_equal (respMsg->u.crud.status, 400);
  assert_int_equal (ret, -1);

   fp = fopen(cfg.crud_config_file, "r");
   if (fp != NULL)
   {
       system("rm parodus_cfg.json");
       fclose(fp);
   }
   if(cfg.crud_config_file !=NULL)
   free(cfg.crud_config_file);
   wrp_free_struct(reqMsg);
   wrp_free_struct(respMsg);
}

void err_createObject_checkExpiresKey()
{
  int ret = 0;
  int write_ret = -1;
  FILE *fp;
  char *testdata = NULL;
  wrp_msg_t *reqMsg = NULL;
  reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(reqMsg, 0, sizeof(wrp_msg_t));
  wrp_msg_t *respMsg = NULL;
  respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(respMsg, 0, sizeof(wrp_msg_t));
  ParodusCfg cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.crud_config_file = strdup("parodus_cfg.json");
  set_parodus_cfg(&cfg);
  testdata=strdup("{ \"expires\" : \"1522451870\" }");

  write_ret = writeToJSON(testdata);
  assert_int_equal (write_ret, 1);
  reqMsg->msg_type = 5;
  reqMsg->u.crud.transaction_uuid = strdup("1234");
  reqMsg->u.crud.source = strdup("tag-update");
  reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test");
  reqMsg->u.crud.payload = strdup("{ \"exps\" : 1000 }");
  respMsg->msg_type = 5;
  ret = createObject(reqMsg, &respMsg);

  assert_int_equal (respMsg->u.crud.status, 400);
  assert_int_equal (ret, -1);

   fp = fopen(cfg.crud_config_file, "r");
   if (fp != NULL)
   {
       system("rm parodus_cfg.json");
       fclose(fp);
   }
   if(cfg.crud_config_file !=NULL)
   free(cfg.crud_config_file);
   wrp_free_struct(reqMsg);
   wrp_free_struct(respMsg);
}

void err_createObject_EmptyKey()
{
  int ret = 0;
  int write_ret = -1;
  FILE *fp;
  char *testdata = NULL;
  wrp_msg_t *reqMsg = NULL;
  reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(reqMsg, 0, sizeof(wrp_msg_t));
  wrp_msg_t *respMsg = NULL;
  respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(respMsg, 0, sizeof(wrp_msg_t));

  ParodusCfg cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.crud_config_file = strdup("parodus_cfg.json");
  set_parodus_cfg(&cfg);
  testdata=strdup("{ \"expires\" : \"1522451870\" }");
  write_ret = writeToJSON(testdata);
  assert_int_equal (write_ret, 1);
  reqMsg->msg_type = 5;
  reqMsg->u.crud.transaction_uuid = strdup("1234");
  reqMsg->u.crud.source = strdup("tag-update");
  reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test");
  reqMsg->u.crud.payload = strdup("{ \"expires\" : 1000, \"\":\"Value\" }");

  respMsg->msg_type = 5;
  ret = createObject(reqMsg, &respMsg);
  assert_int_equal (respMsg->u.crud.status, 400);
  assert_int_equal (ret, -1);

   fp = fopen(cfg.crud_config_file, "r");
   if (fp != NULL)
   {
       system("rm parodus_cfg.json");
       fclose(fp);
   }
   if(cfg.crud_config_file !=NULL)
   free(cfg.crud_config_file);
   wrp_free_struct(reqMsg);
   wrp_free_struct(respMsg);
}

void err_createObject_EmptyKeyValue()
{
  int ret = 0;
  int write_ret = -1;
  FILE *fp;
  char *testdata = NULL;
  wrp_msg_t *reqMsg = NULL;
  reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(reqMsg, 0, sizeof(wrp_msg_t));
  wrp_msg_t *respMsg = NULL;
  respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(respMsg, 0, sizeof(wrp_msg_t));
  ParodusCfg cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.crud_config_file = strdup("parodus_cfg.json");
  set_parodus_cfg(&cfg);
  testdata=strdup("{ \"expires\" : \"1522451870\" }");
  write_ret = writeToJSON(testdata);
  assert_int_equal (write_ret, 1);
  reqMsg->msg_type = 5;
  reqMsg->u.crud.transaction_uuid = strdup("1234");
  reqMsg->u.crud.source = strdup("tag-update");
  reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test");
  reqMsg->u.crud.payload = strdup("{ \"expires\" : 1000, \"key\":\"\" }");

  respMsg->msg_type = 5;
  ret = createObject(reqMsg, &respMsg);
  assert_int_equal (respMsg->u.crud.status, 400);
  assert_int_equal (ret, -1);

   fp = fopen(cfg.crud_config_file, "r");
   if (fp != NULL)
   {
       system("rm parodus_cfg.json");
       fclose(fp);
   }
   if(cfg.crud_config_file !=NULL)
   free(cfg.crud_config_file);
   wrp_free_struct(reqMsg);
   wrp_free_struct(respMsg);
}

void err_createObject_InvalidType()
{
  int ret = 0;
  int write_ret = -1;
  FILE *fp;
  char *testdata = NULL;
  wrp_msg_t *reqMsg = NULL;
  reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(reqMsg, 0, sizeof(wrp_msg_t));
  wrp_msg_t *respMsg = NULL;
  respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(respMsg, 0, sizeof(wrp_msg_t));
  ParodusCfg cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.crud_config_file = strdup("parodus_cfg.json");
  set_parodus_cfg(&cfg);
  testdata=strdup("{ \"expires\" : 1522451870 }");
  write_ret = writeToJSON(testdata);
  assert_int_equal (write_ret, 1);
  reqMsg->msg_type = 5;
  reqMsg->u.crud.transaction_uuid = strdup("1234");
  reqMsg->u.crud.source = strdup("tag-update");
  reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test");
  reqMsg->u.crud.payload = strdup("{ \"expires\" : 123, \"data\":[\"value\"] }");
  respMsg->msg_type = 5;
  ret = createObject(reqMsg, &respMsg);

  assert_int_equal (respMsg->u.crud.status, 400);
  assert_int_equal (ret, -1);

   fp = fopen(cfg.crud_config_file, "r");
   if (fp != NULL)
   {
       system("rm parodus_cfg.json");
       fclose(fp);
   }
   if(cfg.crud_config_file !=NULL)
   free(cfg.crud_config_file);
   wrp_free_struct(reqMsg);
   wrp_free_struct(respMsg);
}

void test_createObject_withWrongPayload()
{
  int ret = 0;
  int write_ret = -1;
  FILE *fp;
  char *testdata = NULL;
 
  wrp_msg_t *reqMsg = NULL;
  reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
  memset(reqMsg, 0, sizeof(wrp_msg_t));    
 
  wrp_msg_t *respMsg = NULL;
  respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
  memset(respMsg, 0, sizeof(wrp_msg_t));    
 
  ParodusCfg cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.crud_config_file = strdup("parodus_cfg.json");
  set_parodus_cfg(&cfg);     
  testdata=strdup("{ \"expires\" : 1522451870 }");  
  write_ret = writeToJSON(testdata);
  assert_int_equal (write_ret, 1);    
  reqMsg->msg_type = 5;
  reqMsg->u.crud.transaction_uuid = strdup("1234");
  reqMsg->u.crud.source = strdup("tag-update");
  reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test");
  reqMsg->u.crud.payload = strdup("{ \"expires\" : 1522451");
  
  respMsg->msg_type = 5;
  ret = createObject(reqMsg, &respMsg);
 
  assert_int_equal (respMsg->u.crud.status, 400);
  assert_int_equal (ret, -1);
 
  
   fp = fopen(cfg.crud_config_file, "r");
   if (fp != NULL)
   {
       system("rm parodus_cfg.json");
       fclose(fp);
   }  
   if(cfg.crud_config_file !=NULL)
   free(cfg.crud_config_file);
   
   wrp_free_struct(reqMsg);
   wrp_free_struct(respMsg);
  
} 

void test_createObject_multipleObjects()
{
  int ret = 0;
  int write_ret = -1;
  FILE *fp;
  char *testdata = NULL;
 
  wrp_msg_t *reqMsg = NULL;
  reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
  memset(reqMsg, 0, sizeof(wrp_msg_t));    
 
  wrp_msg_t *respMsg = NULL;
  respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
  memset(respMsg, 0, sizeof(wrp_msg_t));    
 
  ParodusCfg cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.crud_config_file = strdup("parodus_cfg.json");
  set_parodus_cfg(&cfg);     
  testdata=strdup("{\"tags\":{\"test1\":{\"expires\":1522451870}}}");  
  write_ret = writeToJSON(testdata);
  assert_int_equal (write_ret, 1);    
  reqMsg->msg_type = 5;
  reqMsg->u.crud.transaction_uuid = strdup("1234");
  reqMsg->u.crud.source = strdup("tag-update");
  reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test2");
  reqMsg->u.crud.payload = strdup("{ \"expires\" : 123, \"key\":\"value\", \"key2\": 1234 }");
  
  respMsg->msg_type = 5;
  ret = createObject(reqMsg, &respMsg);
 
  assert_int_equal (respMsg->u.crud.status, 201);
  assert_int_equal (ret, 0);
 
  
   fp = fopen(cfg.crud_config_file, "r");
   if (fp != NULL)
   {
       system("rm parodus_cfg.json");
       fclose(fp);
   }  
   if(cfg.crud_config_file !=NULL)
   free(cfg.crud_config_file);
   
   wrp_free_struct(reqMsg);
   wrp_free_struct(respMsg);
  
} 

void err_createObject_multipleObjectsInvalidType()
{
  int ret = 0;
  int write_ret = -1;
  FILE *fp;
  char *testdata = NULL;
  wrp_msg_t *reqMsg = NULL;
  reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(reqMsg, 0, sizeof(wrp_msg_t));
  wrp_msg_t *respMsg = NULL;
  respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(respMsg, 0, sizeof(wrp_msg_t));
  ParodusCfg cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.crud_config_file = strdup("parodus_cfg.json");
  set_parodus_cfg(&cfg);
  testdata=strdup("{\"tags\":{\"test1\":{\"expires\":1522451870}}}");
  write_ret = writeToJSON(testdata);
  assert_int_equal (write_ret, 1);
  reqMsg->msg_type = 5;
  reqMsg->u.crud.transaction_uuid = strdup("1234");
  reqMsg->u.crud.source = strdup("tag-update");
  reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test2");
  reqMsg->u.crud.payload = strdup("{ \"expires\" : 123, \"key\":\"value\", \"key\":[\"value\"] }");
  respMsg->msg_type = 5;
  ret = createObject(reqMsg, &respMsg);

  assert_int_equal (respMsg->u.crud.status, 400);
  assert_int_equal (ret, -1);

   fp = fopen(cfg.crud_config_file, "r");
   if (fp != NULL)
   {
       system("rm parodus_cfg.json");
       fclose(fp);
   }
   if(cfg.crud_config_file !=NULL)
   free(cfg.crud_config_file);
   wrp_free_struct(reqMsg);
   wrp_free_struct(respMsg);
}

void test_createObject_existingObj()
{
  int ret = 0;
  int write_ret = -1;
  FILE *fp;
  char *testdata = NULL;
 
  wrp_msg_t *reqMsg = NULL;
  reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
  memset(reqMsg, 0, sizeof(wrp_msg_t));    
 
  wrp_msg_t *respMsg = NULL;
  respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );  
  memset(respMsg, 0, sizeof(wrp_msg_t));    
 
  ParodusCfg cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.crud_config_file = strdup("parodus_cfg.json");
  set_parodus_cfg(&cfg);     
  testdata=strdup("{\"tags\":{\"test\":{\"expires\":152245}, \"test1\":{\"expires\":1522451870}}}");  
  write_ret = writeToJSON(testdata);
  assert_int_equal (write_ret, 1);    
  reqMsg->msg_type = 5;
  reqMsg->u.crud.transaction_uuid = strdup("1234");
  reqMsg->u.crud.source = strdup("tag-update");
  reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test1");
  reqMsg->u.crud.payload = strdup("{ \"expires\" : 1522451}");
  
  respMsg->msg_type = 5;
  ret = createObject(reqMsg, &respMsg);
 
  assert_int_equal (respMsg->u.crud.status, 409);
  assert_int_equal (ret, -1);
 
  
   fp = fopen(cfg.crud_config_file, "r");
   if (fp != NULL)
   {
       system("rm parodus_cfg.json");
       fclose(fp);
   }  
   if(cfg.crud_config_file !=NULL)
   free(cfg.crud_config_file);
   
   wrp_free_struct(reqMsg);
   wrp_free_struct(respMsg);
  
}

void test_createObject_jsonFailure()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;

	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test\":{\"expires\":152245}, \"test1\":{\"expires\":1522451870}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	free(cfg.crud_config_file);
	cfg.crud_config_file = NULL;
	set_parodus_cfg(&cfg);
	reqMsg->msg_type = 5;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test1");
	reqMsg->u.crud.payload = strdup("{ \"expires\" : 152}");

	respMsg->msg_type = 5;
	ret = createObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 500);
	assert_int_equal (ret, -1);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);

}

void test_createObject_invalid()
{
  int ret = 0;
  int write_ret = -1;
  FILE *fp;
  char *testdata = NULL;

  wrp_msg_t *reqMsg = NULL;
  reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(reqMsg, 0, sizeof(wrp_msg_t));
  wrp_msg_t *respMsg = NULL;
  respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(respMsg, 0, sizeof(wrp_msg_t));

  ParodusCfg cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.crud_config_file = strdup("parodus_cfg.json");
  set_parodus_cfg(&cfg);
  testdata=strdup("{ \"expires\" : 1522451870 }");
  write_ret = writeToJSON(testdata);
  assert_int_equal (write_ret, 1);
  reqMsg->msg_type = 5;
  reqMsg->u.crud.transaction_uuid = strdup("1234");
  reqMsg->u.crud.source = strdup("tag-update");
  reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags/test");
  reqMsg->u.crud.payload = strdup("{ \"expires\" : 1522451}");
  respMsg->msg_type = 5;
  ret = createObject(reqMsg, &respMsg);
  assert_int_equal (respMsg->u.crud.status, 400);
  assert_int_equal (ret, -1);

   fp = fopen(cfg.crud_config_file, "r");
   if (fp != NULL)
   {
       system("rm parodus_cfg.json");
       fclose(fp);
   }
   if(cfg.crud_config_file !=NULL)
   free(cfg.crud_config_file);
   wrp_free_struct(reqMsg);
   wrp_free_struct(respMsg);
}

void test_retrieveObject_JsonEmpty()
{
	int ret = 0;
	FILE *fp;
	int write_ret = -1;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));

	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));

	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);

	write_ret = writeToJSON(NULL);
	assert_int_equal (write_ret, 0);
	reqMsg->msg_type = 6;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
	respMsg->msg_type = 6;
	ret = retrieveObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 500);

	assert_int_equal (ret, -1);

	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_retrieveObject_destNull()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata;

	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));

	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test\":{\"expires\":123}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 6;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	respMsg->msg_type = 6;
	ret = retrieveObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	if(testdata!=NULL)
		free(testdata);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_retrieveObjectWithNoConfigJson()
{
    int ret = 0;
    wrp_msg_t *reqMsg = NULL;
    wrp_msg_t *respMsg = NULL;
    ParodusCfg cfg;
    memset(&cfg,0,sizeof(cfg));

	respMsg = ( wrp_msg_t *)malloc(sizeof( wrp_msg_t ));
	memset(respMsg, 0, sizeof(wrp_msg_t));
    reqMsg = ( wrp_msg_t *)malloc(sizeof( wrp_msg_t ));
    memset(reqMsg, 0, sizeof(wrp_msg_t));

	reqMsg->msg_type = 6;
    reqMsg->u.crud.transaction_uuid = strdup("1234");
    reqMsg->u.crud.source = strdup("tag-update");
    reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
    respMsg->msg_type = 6;
    ret = retrieveObject(reqMsg, &respMsg);
    assert_int_equal (respMsg->u.crud.status, 500);
    assert_int_equal (ret, -1);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_retrieveObject_JsonParseErr()
{
	int ret = 0;
	FILE *fp;
	int write_ret = -1;
	char * testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));

	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));

	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);

	testdata = strdup("{testData:{");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);

	reqMsg->msg_type = 6;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
	respMsg->msg_type = 6;
	ret = retrieveObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 500);
	assert_int_equal (ret, -1);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_retrieveObject_tags()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;

	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));

	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test1\":{\"expires\":1522451870, \"data1\" : \"key1\", \"data2\":1234}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 6;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
	respMsg->msg_type = 6;
	ret = retrieveObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 200);
	assert_int_equal (ret, 0);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}


void test_retrieveObject_withTagsEmpty()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;

	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 6;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag");
	respMsg->msg_type = 6;
	ret = retrieveObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_retrieveObject_testObj()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));

	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test\":{\"expires\":152245}, \"test1\":{\"expires\":152, \"data\" : \"key\"}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 6;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test1");
	respMsg->msg_type = 6;
	ret = retrieveObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 200);
	assert_int_equal (ret, 0);
	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_retrieveObject_nonexistObj()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));

	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test1\":{\"expires\":1522451870}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 6;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test");
	respMsg->msg_type = 6;
	ret = retrieveObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);

}

void test_retrieveObject_tagsFailure()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"test\":{}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 6;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
	respMsg->msg_type = 6;
	ret = retrieveObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);

}

void test_retrieveObject_invalid()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test1\":{\"expires\":1522451870}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 6;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags/test1");
	respMsg->msg_type = 6;
	ret = retrieveObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);

}


void test_retrieveObject_readOnlyObj()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	parStrncpy(cfg.hw_model, "X5001",sizeof(cfg.hw_model));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test1\":{\"expires\":1522451870}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 6;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/hw-model");
	respMsg->msg_type = 6;
	ret = retrieveObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 200);
	assert_int_equal (ret, 0);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_retrieveObject_cloud_status()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.cloud_status = CLOUD_STATUS_ONLINE;
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test1\":{\"expires\":1522451870}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 6;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/cloud-status");
	respMsg->msg_type = 6;
	ret = retrieveObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 200);
	assert_int_equal (ret, 0);
	assert_string_equal(get_cloud_status(), CLOUD_STATUS_ONLINE);
	assert_int_equal (respMsg->u.crud.payload_size, 25);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void err_retrieveObject_cloud_statusNULL()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.cloud_status = NULL;
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test1\":{\"expires\":1522451870}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 6;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/cloud-status");
	respMsg->msg_type = 6;
	ret = retrieveObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	assert_int_equal (respMsg->u.crud.payload_size, 0);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void err_retrieveObject_cloud_statusEmpty()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.cloud_status = "";
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test1\":{\"expires\":1522451870}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 6;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/cloud-status");
	respMsg->msg_type = 6;
	ret = retrieveObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	assert_int_equal (respMsg->u.crud.payload_size, 0);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_retrieveObject_readOnlyFailure()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	parStrncpy(cfg.hw_model, "X5001",sizeof(cfg.hw_model));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test1\":{\"expires\":1522451870}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 6;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/webpa-invalid");

	respMsg->msg_type = 6;
	ret = retrieveObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);

}

void test_updateObject_JsonEmpty()
{
	int ret = 0;
	FILE *fp;
	int write_ret = -1;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));

	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));

	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);

	write_ret = writeToJSON(NULL);
	assert_int_equal (write_ret, 0);
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test");
	reqMsg->u.crud.payload = strdup("{ \"expires\" : 1522451}");
	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 201);

	assert_int_equal (ret, 0);

	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_updateObject_destNull()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata;

	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test\":{\"expires\":123}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	if(testdata!=NULL)
		free(testdata);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);

}

void test_updateObjectWithNoConfigJson()
{
    int ret = 0;
    wrp_msg_t *reqMsg = NULL;
    wrp_msg_t *respMsg = NULL;
    ParodusCfg cfg;
    memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	respMsg = ( wrp_msg_t *)malloc(sizeof( wrp_msg_t ));
	memset(respMsg, 0, sizeof(wrp_msg_t));
    reqMsg = ( wrp_msg_t *)malloc(sizeof( wrp_msg_t ));
    memset(reqMsg, 0, sizeof(wrp_msg_t));

	reqMsg->msg_type = 7;
    reqMsg->u.crud.transaction_uuid = strdup("1234");
    reqMsg->u.crud.source = strdup("tag-update");
    reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test1");
    reqMsg->u.crud.payload = strdup("{ \"expires\" : 1522451}");
    respMsg->msg_type = 7;
    ret = updateObject(reqMsg, &respMsg);
    assert_int_equal (respMsg->u.crud.status, 201);
    assert_int_equal (ret, 0);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_updateObject_JsonParseErr()
{
	int ret = 0;
	FILE *fp;
	int write_ret = -1;
	char * testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));

	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));

	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);

	testdata = strdup("{testData:{");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);

	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 500);
	assert_int_equal (ret, -1);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_updateObject_existingObj()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test\":{\"expires\":152245}, \"test1\":{\"expires\":1522451870}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test1");
	reqMsg->u.crud.payload = strdup("{ \"expires\" : 152,  \"data\" : \"key\"}");
	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 200);
	assert_int_equal (ret, 0);
	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);

}

void err_updateObject_existingObj_InvalidType()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test\":{\"expires\":152245}, \"test1\":{\"expires\":1522451870}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test1");
	reqMsg->u.crud.payload = strdup("{ \"expires\" : 152,  \"data\" : [\"key\"]}");
	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);

}


void test_updateObject_NonExistingObj()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test\":{\"expires\":152245}, \"test1\":{\"expires\":1522451870}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test2");
	reqMsg->u.crud.payload = strdup("{ \"expires\" : 15256, \"data\":\"key\"}");
	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 201);
	assert_int_equal (ret, 0);
	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void err_updateObject_NonExisting_InvalidType()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test\":{\"expires\":152245}, \"test1\":{\"expires\":1522451870}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test2");
	reqMsg->u.crud.payload = strdup("{ \"expires\" : 15256, \"data\":[\"key\"]}");
	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_updateObject_withWrongPayload()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;

	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{ \"expires\" : 1522451870 }");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test");
	reqMsg->u.crud.payload = strdup("{ \"expires\" : 1522451");

	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_updateObject_NullPayload()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{ \"expires\" : 1522451870 }");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags/test");
	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);

}

void test_updateObject_jsonFailure()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;

	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test\":{\"expires\":152245}, \"test1\":{\"expires\":1522451870}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	free(cfg.crud_config_file);
	cfg.crud_config_file = NULL;
	set_parodus_cfg(&cfg);
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test1");
	reqMsg->u.crud.payload = strdup("{ \"expires\" : 152}");

	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 500);
	assert_int_equal (ret, -1);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);

}

void test_updateObject_invalid()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;

	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{ \"expires\" : 1522451870 }");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags/test");
	reqMsg->u.crud.payload = strdup("{ \"expires\" : 1522451}");

	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_updateObject_InvalidExpiresKey()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;

	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{ \"expires\" : 1522451870 }");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test");
	reqMsg->u.crud.payload = strdup("{ \"expires\" : 0}");

	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void err_updateObject_checkExpiresKey()
{
  int ret = 0;
  int write_ret = -1;
  FILE *fp;
  char *testdata = NULL;
  wrp_msg_t *reqMsg = NULL;
  reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(reqMsg, 0, sizeof(wrp_msg_t));
  wrp_msg_t *respMsg = NULL;
  respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(respMsg, 0, sizeof(wrp_msg_t));
  ParodusCfg cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.crud_config_file = strdup("parodus_cfg.json");
  set_parodus_cfg(&cfg);
  testdata=strdup("{ \"expires\" : \"1522451870\" }");
  write_ret = writeToJSON(testdata);
  assert_int_equal (write_ret, 1);
  reqMsg->msg_type = 7;
  reqMsg->u.crud.transaction_uuid = strdup("1234");
  reqMsg->u.crud.source = strdup("tag-update");
  reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test");
  reqMsg->u.crud.payload = strdup("{ \"exps\" : 1000 }");
  respMsg->msg_type = 7;
  ret = updateObject(reqMsg, &respMsg);

  assert_int_equal (respMsg->u.crud.status, 400);
  assert_int_equal (ret, -1);

   fp = fopen(cfg.crud_config_file, "r");
   if (fp != NULL)
   {
       system("rm parodus_cfg.json");
       fclose(fp);
   }
   if(cfg.crud_config_file !=NULL)
   free(cfg.crud_config_file);
   wrp_free_struct(reqMsg);
   wrp_free_struct(respMsg);
}

void err_updateObject_EmptyKey()
{
  int ret = 0;
  int write_ret = -1;
  FILE *fp;
  char *testdata = NULL;
  wrp_msg_t *reqMsg = NULL;
  reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(reqMsg, 0, sizeof(wrp_msg_t));
  wrp_msg_t *respMsg = NULL;
  respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(respMsg, 0, sizeof(wrp_msg_t));
  ParodusCfg cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.crud_config_file = strdup("parodus_cfg.json");
  set_parodus_cfg(&cfg);
  testdata=strdup("{ \"expires\" : \"1522451870\" }");
  write_ret = writeToJSON(testdata);
  assert_int_equal (write_ret, 1);
  reqMsg->msg_type = 7;
  reqMsg->u.crud.transaction_uuid = strdup("1234");
  reqMsg->u.crud.source = strdup("tag-update");
  reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test");
  reqMsg->u.crud.payload = strdup("{ \"expires\" : 1000, \"\":\"Value\" }");
  respMsg->msg_type = 7;
  ret = updateObject(reqMsg, &respMsg);

  assert_int_equal (respMsg->u.crud.status, 400);
  assert_int_equal (ret, -1);
   fp = fopen(cfg.crud_config_file, "r");
   if (fp != NULL)
   {
       system("rm parodus_cfg.json");
       fclose(fp);
   }
   if(cfg.crud_config_file !=NULL)
   free(cfg.crud_config_file);
   wrp_free_struct(reqMsg);
   wrp_free_struct(respMsg);
}

void err_updateObject_EmptyKeyValue()
{
  int ret = 0;
  int write_ret = -1;
  FILE *fp;
  char *testdata = NULL;
  wrp_msg_t *reqMsg = NULL;
  reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(reqMsg, 0, sizeof(wrp_msg_t));
  wrp_msg_t *respMsg = NULL;
  respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(respMsg, 0, sizeof(wrp_msg_t));
  ParodusCfg cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.crud_config_file = strdup("parodus_cfg.json");
  set_parodus_cfg(&cfg);
  testdata=strdup("{ \"expires\" : \"1522451870\" }");
  write_ret = writeToJSON(testdata);
  assert_int_equal (write_ret, 1);
  reqMsg->msg_type = 7;
  reqMsg->u.crud.transaction_uuid = strdup("1234");
  reqMsg->u.crud.source = strdup("tag-update");
  reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test");
  reqMsg->u.crud.payload = strdup("{ \"expires\" : 1000, \"key\":\"\" }");
  respMsg->msg_type = 7;
  ret = updateObject(reqMsg, &respMsg);
  assert_int_equal (respMsg->u.crud.status, 400);
  assert_int_equal (ret, -1);

   fp = fopen(cfg.crud_config_file, "r");
   if (fp != NULL)
   {
       system("rm parodus_cfg.json");
       fclose(fp);
   }
   if(cfg.crud_config_file !=NULL)
   free(cfg.crud_config_file);
   wrp_free_struct(reqMsg);
   wrp_free_struct(respMsg);
}

void err_updateObject_InvalidType()
{
  int ret = 0;
  int write_ret = -1;
  FILE *fp;
  char *testdata = NULL;
  wrp_msg_t *reqMsg = NULL;
  reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(reqMsg, 0, sizeof(wrp_msg_t));
  wrp_msg_t *respMsg = NULL;
  respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
  memset(respMsg, 0, sizeof(wrp_msg_t));
  ParodusCfg cfg;
  memset(&cfg,0,sizeof(cfg));
  cfg.crud_config_file = strdup("parodus_cfg.json");
  set_parodus_cfg(&cfg);
  testdata=strdup("{ \"expires\" : 1522451870 }");
  write_ret = writeToJSON(testdata);
  assert_int_equal (write_ret, 1);
  reqMsg->msg_type = 7;
  reqMsg->u.crud.transaction_uuid = strdup("1234");
  reqMsg->u.crud.source = strdup("tag-update");
  reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test");
  reqMsg->u.crud.payload = strdup("{ \"expires\" : 123, \"data1\": \"key1\", \"data\":[\"value\"] }");
  respMsg->msg_type = 7;
  ret = updateObject(reqMsg, &respMsg);

  assert_int_equal (respMsg->u.crud.status, 400);
  assert_int_equal (ret, -1);

   fp = fopen(cfg.crud_config_file, "r");
   if (fp != NULL)
   {
       system("rm parodus_cfg.json");
       fclose(fp);
   }
   if(cfg.crud_config_file !=NULL)
   free(cfg.crud_config_file);
   wrp_free_struct(reqMsg);
   wrp_free_struct(respMsg);
}

void err_updateObject_cloud_disconnectInvalid()
{
	int ret = 0;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("dns:tr1d1um.webpa.comcast.net/config");
	reqMsg->u.crud.dest = strdup("mac:14cfe2142145/parodus/cloud-disconnect");
	reqMsg->u.crud.payload = strdup("{ \"disconnection-reason\": \"***\" }");
	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void err_updateObject_cloud_disconnectNULL()
{
	int ret = 0;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("dns:tr1d1um.webpa.comcast.net/config");
	reqMsg->u.crud.dest = strdup("mac:14cfe2142145/parodus/cloud-disconnect");
	reqMsg->u.crud.payload = strdup("{ \"disconnection-reason\": \"\" }");
	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void err_updateObject_no_cloud_disconnectReason()
{
	int ret = 0;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("dns:tr1d1um.webpa.comcast.net/config");
	reqMsg->u.crud.dest = strdup("mac:14cfe2142145/parodus/cloud-disconnect");
	reqMsg->u.crud.payload = strdup("{ \"discon-val\": \"XPC\" }");
	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void err_updateObject_cloud_disconnectNoPayload()
{
	int ret = 0;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("dns:tr1d1um.webpa.comcast.net/config");
	reqMsg->u.crud.dest = strdup("mac:14cfe2142145/parodus/cloud-disconnect");
	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);

}

void err_updateObject_cloud_disconnectInvalidPayload()
{
	int ret = 0;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("dns:tr1d1um.webpa.comcast.net/config");
	reqMsg->u.crud.dest = strdup("mac:14cfe2142145/parodus/cloud-disconnect");
	reqMsg->u.crud.payload = strdup("{ \"disconnection-reason\"\"XPC\" }");
	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void err_updateObject_cloud_disconnectInvalidType()
{
	int ret = 0;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("dns:tr1d1um.webpa.comcast.net/config");
	reqMsg->u.crud.dest = strdup("mac:14cfe2142145/parodus/cloud-disconnect");
	reqMsg->u.crud.payload = strdup("{ \"disconnection-reason\":123 }");
	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void err_updateObject_cloud_disconnectFailure()
{
	int ret = 0;
	wrp_msg_t *reqMsg = NULL;
	set_close_retry();
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("dns:tr1d1um.webpa.comcast.net/config");
	reqMsg->u.crud.dest = strdup("mac:14cfe2142145/parodus/cloud-disconnect");
	reqMsg->u.crud.payload = strdup("{ \"disconnection-reason\":\"XPC09\" }");
	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 500);
	assert_int_equal (ret, -1);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
	reset_close_retry();
}

void test_updateObject_cloud_disconnect()
{
	int ret = 0;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	reqMsg->msg_type = 7;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("dns:tr1d1um.webpa.comcast.net/config");
	reqMsg->u.crud.dest = strdup("mac:14cfe2142145/parodus/cloud-disconnect");
	reqMsg->u.crud.payload = strdup("{ \"disconnection-reason\":\"XPC\" }");
	respMsg->msg_type = 7;
	ret = updateObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 200);
	assert_int_equal (ret, 0);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_deleteObject_JsonEmpty()
{
	int ret = 0;
	FILE *fp;
	int write_ret = -1;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));

	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));

	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);

	write_ret = writeToJSON(NULL);
	assert_int_equal (write_ret, 0);
	reqMsg->msg_type = 8;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
	respMsg->msg_type = 8;
	ret = deleteObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 500);

	assert_int_equal (ret, -1);

	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_deleteObject_destNull()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata;

	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test\":{\"expires\":123}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 8;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	respMsg->msg_type = 8;
	ret = deleteObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	if(testdata!=NULL)
		free(testdata);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);

}

void test_deleteObjectWithNoConfigJson()
{
    int ret = 0;
    wrp_msg_t *reqMsg = NULL;
    wrp_msg_t *respMsg = NULL;
    ParodusCfg cfg;
    memset(&cfg,0,sizeof(cfg));

	respMsg = ( wrp_msg_t *)malloc(sizeof( wrp_msg_t ));
	memset(respMsg, 0, sizeof(wrp_msg_t));
    reqMsg = ( wrp_msg_t *)malloc(sizeof( wrp_msg_t ));  
    memset(reqMsg, 0, sizeof(wrp_msg_t));

	reqMsg->msg_type = 8;
    reqMsg->u.crud.transaction_uuid = strdup("1234");
    reqMsg->u.crud.source = strdup("tag-update");
    reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
    respMsg->msg_type = 8;
    ret = deleteObject(reqMsg, &respMsg);
    assert_int_equal (respMsg->u.crud.status, 500);
    assert_int_equal (ret, -1);
	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_deleteObject_JsonParseErr()
{
	int ret = 0;
	FILE *fp;
	int write_ret = -1;
	char * testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));

	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));

	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);

	testdata = strdup("{testData:{");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);

	reqMsg->msg_type = 8;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
	respMsg->msg_type = 8;
	ret = deleteObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 500);
	assert_int_equal (ret, -1);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_deleteObject_tags()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;

	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test1\":{\"expires\":1522451870}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 8;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");

	respMsg->msg_type = 8;
	ret = deleteObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_deleteObject_testObj()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test\":{\"expires\":152245}, \"test1\":{\"expires\":1522451870}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 8;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test1");

	respMsg->msg_type = 8;
	ret = deleteObject(reqMsg, &respMsg);

	assert_int_equal (respMsg->u.crud.status, 200);
	assert_int_equal (ret, 0);
	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_deleteObject_NonExistObj()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{\"test1\":{\"expires\":1522451870}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 8;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test2");
	respMsg->msg_type = 8;
	ret = deleteObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_deleteObject_withTagsEmpty()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"tags\":{}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 8;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tag/test");
	respMsg->msg_type = 8;
	ret = deleteObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);
	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

void test_deleteObject_tagsFailure()
{
	int ret = 0;
	int write_ret = -1;
	FILE *fp;
	char *testdata = NULL;
	wrp_msg_t *reqMsg = NULL;
	reqMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(reqMsg, 0, sizeof(wrp_msg_t));
	wrp_msg_t *respMsg = NULL;
	respMsg = ( wrp_msg_t *)malloc( sizeof( wrp_msg_t ) );
	memset(respMsg, 0, sizeof(wrp_msg_t));
	ParodusCfg cfg;
	memset(&cfg,0,sizeof(cfg));
	cfg.crud_config_file = strdup("parodus_cfg.json");
	set_parodus_cfg(&cfg);
	testdata=strdup("{\"test\":{}}}");
	write_ret = writeToJSON(testdata);
	assert_int_equal (write_ret, 1);
	reqMsg->msg_type = 8;
	reqMsg->u.crud.transaction_uuid = strdup("1234");
	reqMsg->u.crud.source = strdup("tag-update");
	reqMsg->u.crud.dest = strdup("mac:14xxx/parodus/tags");
	respMsg->msg_type = 8;
	ret = deleteObject(reqMsg, &respMsg);
	assert_int_equal (respMsg->u.crud.status, 400);
	assert_int_equal (ret, -1);

	fp = fopen(cfg.crud_config_file, "r");
	if (fp != NULL)
	{
		system("rm parodus_cfg.json");
		fclose(fp);
	}
	if(cfg.crud_config_file !=NULL)
		free(cfg.crud_config_file);

	wrp_free_struct(reqMsg);
	wrp_free_struct(respMsg);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_writeToJSON_Failure),
        cmocka_unit_test(test_writeToJSON),
        cmocka_unit_test(test_readFromJSON_Failure),
        cmocka_unit_test(test_readFromJSON),
        cmocka_unit_test(test_retrieveFromMemory),
        cmocka_unit_test(test_retrieveFromMemoryFailure),
        
        cmocka_unit_test(test_createObjectInvalidReq),
        cmocka_unit_test(test_createObjectInvalid_JsonEmpty),
        cmocka_unit_test(test_createObjectInvalid_JsonNonEmpty),
        cmocka_unit_test(test_createObjectInvalid_JsonParseErr),
        cmocka_unit_test(test_createObject_destNull),
        cmocka_unit_test(test_createObject_JsonParse),
        cmocka_unit_test(test_UnsupportedDestination),
        cmocka_unit_test(test_createObject_withProperPayload),
        cmocka_unit_test(test_createObject_withWrongPayload),
        cmocka_unit_test(test_createObject_multipleObjects),
        cmocka_unit_test(test_createObject_existingObj),
        cmocka_unit_test(test_createObject_jsonFailure),
        cmocka_unit_test(test_createObject_invalid),
        cmocka_unit_test(test_createObject_InvalidExpiresKey),
        cmocka_unit_test(err_createObject_checkExpiresKey),
        cmocka_unit_test(err_createObject_EmptyKey),
        cmocka_unit_test(err_createObject_EmptyKeyValue),
        cmocka_unit_test(err_createObject_InvalidType),
        cmocka_unit_test(err_createObject_multipleObjectsInvalidType),
        
        cmocka_unit_test(test_retrieveObject_JsonEmpty),
        cmocka_unit_test(test_retrieveObject_destNull),
        cmocka_unit_test(test_retrieveObjectWithNoConfigJson),
        cmocka_unit_test(test_retrieveObject_JsonParseErr),
        cmocka_unit_test(test_retrieveObject_tags),
        cmocka_unit_test(test_retrieveObject_withTagsEmpty),
        cmocka_unit_test(test_retrieveObject_testObj),
        cmocka_unit_test(test_retrieveObject_nonexistObj),
        cmocka_unit_test(test_retrieveObject_tagsFailure),
        cmocka_unit_test(test_retrieveObject_invalid),
        cmocka_unit_test(test_retrieveObject_readOnlyObj),
        cmocka_unit_test(test_retrieveObject_readOnlyFailure),
        
        cmocka_unit_test(test_retrieveObject_cloud_status),
        cmocka_unit_test(err_retrieveObject_cloud_statusNULL),
        cmocka_unit_test(err_retrieveObject_cloud_statusEmpty),

        cmocka_unit_test(test_updateObject_JsonEmpty),
        cmocka_unit_test(test_updateObject_destNull),
        cmocka_unit_test(test_updateObjectWithNoConfigJson),
        cmocka_unit_test(test_updateObject_JsonParseErr),
        cmocka_unit_test(test_updateObject_NonExistingObj),
        cmocka_unit_test(test_updateObject_existingObj),
        cmocka_unit_test(test_updateObject_withWrongPayload),
        cmocka_unit_test(test_updateObject_NullPayload),
        cmocka_unit_test(test_updateObject_jsonFailure),
        cmocka_unit_test(test_updateObject_invalid),
        cmocka_unit_test(test_updateObject_InvalidExpiresKey),
        cmocka_unit_test(err_updateObject_checkExpiresKey),
        cmocka_unit_test(err_updateObject_EmptyKey),
        cmocka_unit_test(err_updateObject_EmptyKeyValue),
        cmocka_unit_test(err_updateObject_InvalidType),
        cmocka_unit_test(err_updateObject_NonExisting_InvalidType),
        cmocka_unit_test(err_updateObject_existingObj_InvalidType),
        
        cmocka_unit_test(test_updateObject_cloud_disconnect),
        cmocka_unit_test(err_updateObject_cloud_disconnectNULL),
        cmocka_unit_test(err_updateObject_cloud_disconnectInvalid),
        cmocka_unit_test(err_updateObject_cloud_disconnectFailure),
        cmocka_unit_test(err_updateObject_no_cloud_disconnectReason),
        cmocka_unit_test(err_updateObject_cloud_disconnectNoPayload),
        cmocka_unit_test(err_updateObject_cloud_disconnectInvalidType),
        cmocka_unit_test(err_updateObject_cloud_disconnectInvalidPayload),

        cmocka_unit_test(test_deleteObject_JsonEmpty),
        cmocka_unit_test(test_deleteObject_destNull),
        cmocka_unit_test(test_deleteObjectWithNoConfigJson),
        cmocka_unit_test(test_deleteObject_JsonParseErr),
        cmocka_unit_test(test_deleteObject_tags),
        cmocka_unit_test(test_deleteObject_testObj),
        cmocka_unit_test(test_deleteObject_NonExistObj),
        cmocka_unit_test(test_deleteObject_withTagsEmpty),
        cmocka_unit_test(test_deleteObject_tagsFailure)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
