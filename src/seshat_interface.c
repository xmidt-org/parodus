/**
 * @file seshat_interface.c
 *
 * @description This decribes interface to register seshat service.
 *
 * Copyright (c) 2015  Comcast
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
