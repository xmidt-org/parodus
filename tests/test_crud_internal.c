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
    //memset(cfg,0,sizeof(cfg));
     
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
	ret = retrieveFromMemory("webpa-inteface-used", &jsonresponse );
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
	ret = retrieveFromMemory("webpa-inteface-used", &jsonresponse );
	assert_int_equal (ret, -1);
	ret = retrieveFromMemory("webpa-backoff-max", &jsonresponse );
	assert_int_equal (ret, 0);
	ret = retrieveFromMemory("webpa-invalid", &jsonresponse );
	assert_int_equal (ret, -1);
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
        
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
