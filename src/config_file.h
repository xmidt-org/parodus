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
 */

#ifndef __CONFIG_FILE_H__
#define __CONFIG_FILE_H__


#ifdef __cplusplus
extern "C" {
#endif


/*
 Parses json config_disk_file and fills cfg, 
 returns 0 on success
 a negative number on errors.
 */
int processParodusCfg(ParodusCfg *cfg, char *config_disk_file);


#ifdef __cplusplus
}
#endif

#endif
