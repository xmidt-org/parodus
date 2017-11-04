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
 * @file seshat_interface.c
 *
 * @description This decribes interface to register seshat service.
 *
 */
 
#include "seshat_interface.h"
#include "ParodusInternal.h"
#include "config.h"
#include <libseshat.h>

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define SESHAT_SERVICE_NAME                             "Parodus"
/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/*                             Internal Functions                             */
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
/*                             External functions                             */
/*----------------------------------------------------------------------------*/

bool __registerWithSeshat()
{
    char *seshat_url = get_parodus_cfg()->seshat_url;
    char *parodus_url = get_parodus_cfg()->local_url;
    char *discover_url = NULL;
    bool rv = false;

    if( 0 == init_lib_seshat(seshat_url) ) {
        ParodusInfo("seshatlib initialized! (url %s)\n", seshat_url);

        if( 0 == seshat_register(SESHAT_SERVICE_NAME, parodus_url) ) {
            ParodusInfo("seshatlib registered! (url %s)\n", parodus_url);

            discover_url = seshat_discover(SESHAT_SERVICE_NAME);
            if( (NULL != discover_url) && (0 == strcmp(parodus_url, discover_url)) ) {
                ParodusInfo("seshatlib discovered url = %s\n", discover_url);
                rv = true;
            } else {
                ParodusError("seshatlib registration error (url %s)!", discover_url);
            }
            free(discover_url);
        } else {
            ParodusError("seshatlib not registered! (url %s)\n", parodus_url);
        }
    } else {
        ParodusPrint("seshatlib not initialized! (url %s)\n", seshat_url);
    }

    shutdown_seshat_lib();
    return rv;
}
