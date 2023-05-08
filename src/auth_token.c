/**
 * Copyright 2015 Comcast Cable Communications Management, LLC
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
 * @file auth_token.c
 *
 * @description This file is to fetch authorization token during parodus cloud connection.
 *
 */

#include <stdio.h>
#include <fcntl.h> 
#include "config.h"
#include "connection.h"
#include "auth_token.h"
#include "ParodusInternal.h"
#include <cjwt/cjwt.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <uuid/uuid.h>

#define MAX_BUF_SIZE	        256
#define CURL_TIMEOUT_SEC	25L
#define MAX_CURL_RETRY_COUNT 	3

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
void createCurlheader(struct curl_slist *list, struct curl_slist **header_list);
long g_response_code;
/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/
int getGlobalResponseCode()
{
	return g_response_code;
}
/*
* @brief Initialize curl object with required options. create newToken using libcurl.
* @param[out] newToken auth token string obtained from JWT curl response
* @param[in] len total token size
* @param[in] r_count Number of curl retries on ipv4 and ipv6 mode during failure
* @return returns 0 if success, otherwise failed to fetch auth token and will be retried.
*/

int requestNewAuthToken(char *newToken, size_t len, int r_count)
{
	CURL *curl;
	CURLcode res;
	CURLcode time_res;
	struct curl_slist *list = NULL;
	struct curl_slist *headers_list = NULL;

	double total;

	struct token_data data;
	data.size = 0;
	data.data = newToken;

	curl = curl_easy_init();
	if(curl)
	{
		data.data[0] = '\0';

		createCurlheader(list, &headers_list);

		curl_easy_setopt(curl, CURLOPT_URL, get_parodus_cfg()->token_server_url);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT_SEC);

		if(get_parodus_cfg()->webpa_interface_used !=NULL && strlen(get_parodus_cfg()->webpa_interface_used) >0)
		{
			curl_easy_setopt(curl, CURLOPT_INTERFACE, get_parodus_cfg()->webpa_interface_used);
		}
		/* set callback for writing received data */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_fn);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers_list);

		/* setting curl resolve option as default mode.
		If any failure, retry with v4 first and then v6 mode. */

		if(r_count == 1)
		{
			ParodusInfo("curl Ip resolve option set as V4 mode\n");
			curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
		}
		else if(r_count == 2)
		{
			ParodusInfo("curl Ip resolve option set as V6 mode\n");
			curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6);
		}
		else
		{
			ParodusInfo("curl Ip resolve option set as default mode\n");
			curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_WHATEVER);
		}

		/* set the cert for client authentication */
		curl_easy_setopt(curl, CURLOPT_SSLCERT, get_parodus_cfg()->client_cert_path);

		curl_easy_setopt(curl, CURLOPT_CAINFO, get_parodus_cfg()->cert_path);

		/* disconnect if it is failed to validate server's cert */
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);

		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &g_response_code);
		ParodusInfo("themis curl response %d http_code %d\n", res, g_response_code);

		time_res = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total);
		if(time_res == 0)
		{
			ParodusInfo("curl response Time: %.1f seconds\n", total);
		}
		curl_slist_free_all(headers_list);
		if(res != 0)
		{
			ParodusError("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			curl_easy_cleanup(curl);
			data.size = 0;
			memset (data.data, 0, len);
			return -1;
		}
		else
		{
			if(getGlobalResponseCode() == 200)
			{
				ParodusInfo("cURL success\n");
			}
			else
			{
				ParodusError("Failed response from auth token server %s\n", data.data);
				curl_easy_cleanup(curl);
				data.size = 0;
				memset (data.data, 0, len);
				return -1;
			}
		}
		curl_easy_cleanup(curl);
	}
	else
	{
		ParodusError("curl init failure\n");
		data.size = 0;
		memset (data.data, 0, len);
		return -1;
	}

	return 0;
}

/*
* @brief Fetches authorization token and update to parodus config.
  This will do curl retry in case of any failure till it reaches max curl retry count.
 * @param[in] cfg Global parodus config structure to update webpa_auth_token
*/

void getAuthToken(ParodusCfg *cfg)
{
	int status = -1;
	int retry_count = 0;

	memset (cfg->webpa_auth_token, 0, sizeof(cfg->webpa_auth_token));
	if( cfg->hw_mac != NULL && strlen(cfg->hw_mac) !=0 )
	{
		if( cfg->client_cert_path !=NULL && strlen(cfg->client_cert_path) !=0 )
		{
			while(1)
			{
				//Fetch new auth token using libcurl
				status = requestNewAuthToken(cfg->webpa_auth_token, sizeof(cfg->webpa_auth_token), retry_count);
				if(status == 0)
				{
					ParodusInfo("cfg->webpa_auth_token created successfully\n");
					break;
				}
				else
				{
					ParodusError("Failed to create new token\n");
					retry_count++;
					ParodusError("Curl execution is failed, retry attempt: %d\n", retry_count);
				}

				if(retry_count == MAX_CURL_RETRY_COUNT)
				{
					ParodusError("Curl retry is reached to max %d attempts, proceeding without token\n", retry_count);
					break;
				}
			}
		}
		else
		{
			ParodusError("client_cert_path is NULL, failed to fetch auth token\n");
		}
	}
	else
	{
		ParodusError("hw_mac is NULL, failed to fetch auth token\n");
	}
}

/* @brief callback function for writing libcurl received data
 * @param[in] buffer curl delivered data which need to be saved.
 * @param[in] size size is always 1
 * @param[in] nmemb size of delivered data
 * @param[out] data curl response data saved.
*/
size_t write_callback_fn(void *buffer, size_t size, size_t nmemb, struct token_data *data)
{
    ParodusCfg *cfg;
    size_t max_data_size = sizeof (cfg->webpa_auth_token);
    size_t index = data->size;
    size_t n = (size * nmemb);

    data->size += n;

    if (data->size >= max_data_size) {
        ParodusError("Auth token data overruns buffer\n");
	data->size = 0;
        return 0;
    }

    memcpy((data->data + index), buffer, n);
    data->data[data->size] = '\0';

    return n;
}

/* @brief function to generate random uuid.
*/
char* generate_trans_uuid()
{
	char *transID = NULL;
	uuid_t transaction_Id;
	char *trans_id = NULL;
	trans_id = (char *)malloc(37);
	uuid_generate_random(transaction_Id);
	uuid_unparse(transaction_Id, trans_id);

	if(trans_id !=NULL)
	{
		transID = trans_id;
	}
	return transID;
}

/* @brief function to create curl header contains mac, serial number and uuid.
 * @param[in] h the auth headers to populate
 * @param[in] list temp curl header list
 * @param[out] header_list output curl header list
*/
void createCurlheader(struct curl_slist *list, struct curl_slist **header_list)
{
    char buf[MAX_BUF_SIZE];
    char *uuid = NULL;

    snprintf(buf, MAX_BUF_SIZE, "X-Xmidt-Mac-Address: %s", get_parodus_cfg()->hw_mac);
    ParodusPrint("mac_header formed %s\n", buf);
    list = curl_slist_append(list, buf);

    snprintf(buf, MAX_BUF_SIZE, "X-Xmidt-Serial-Number: %s", get_parodus_cfg()->hw_serial_number);
    ParodusPrint("serial_header formed %s\n", buf);
    list = curl_slist_append(list, buf);

    uuid = generate_trans_uuid();
    if(uuid !=NULL) {
        snprintf(buf, MAX_BUF_SIZE, "X-Xmidt-Uuid: %s", uuid);
        ParodusInfo("uuid_header formed %s\n", buf);
        list = curl_slist_append(list, buf);
        free(uuid);
    } else {
        ParodusError("Failed to generate transaction_uuid\n");
    }

    snprintf(buf, MAX_BUF_SIZE, "X-Xmidt-Partner-Id: %s", get_parodus_cfg()->partner_id);
    ParodusInfo("partnerid_header formed %s\n", buf);
    list = curl_slist_append(list, buf);

    snprintf(buf, MAX_BUF_SIZE, "X-Xmidt-Hardware-Model: %s", get_parodus_cfg()->hw_model);
    list = curl_slist_append(list, buf);

    snprintf(buf, MAX_BUF_SIZE, "X-Xmidt-Hardware-Manufacturer: %s", get_parodus_cfg()->hw_manufacturer);
    list = curl_slist_append(list, buf);

    snprintf(buf, MAX_BUF_SIZE, "X-Xmidt-Firmware-Name: %s", get_parodus_cfg()->fw_name);
    list = curl_slist_append(list, buf);

    snprintf(buf, MAX_BUF_SIZE, "X-Xmidt-Protocol: %s", get_parodus_cfg()->webpa_protocol);
    list = curl_slist_append(list, buf);

    snprintf(buf, MAX_BUF_SIZE, "X-Xmidt-Interface-Used: %s", get_parodus_cfg()->webpa_interface_used);
    list = curl_slist_append(list, buf);

    snprintf(buf, MAX_BUF_SIZE, "X-Xmidt-Last-Reboot-Reason: %s", get_parodus_cfg()->hw_last_reboot_reason);
    list = curl_slist_append(list, buf);

    snprintf(buf, MAX_BUF_SIZE, "X-Xmidt-Last-Reconnect-Reason: %s", get_global_reconnect_reason());
    list = curl_slist_append(list, buf);

    snprintf(buf, MAX_BUF_SIZE, "X-Xmidt-Boot-Retry-Wait: %d", get_parodus_cfg()->boot_retry_wait);
    list = curl_slist_append(list, buf);

    *header_list = list;
}
