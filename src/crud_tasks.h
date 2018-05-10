/**
 * Copyright 2016 Comcast Cable Communications Management, LLC
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
 
 
#include <wrp-c.h>
#include "ParodusInternal.h"

/**
 * @brief processCrudRequest function to process CRUD operations.
 *
 * @note processCrudRequest function allocates memory for response, 
 * caller needs to free the response (resMsg) in both success and failure case.
 * @param[in] decoded request in wrp_msg_t structure format
 * @param[out] resulting wrp_msg_t structure as a response 
 * @return  0 in success case and -1 in error case
 */
int processCrudRequest(wrp_msg_t * reqMsg, wrp_msg_t **resMsg);
