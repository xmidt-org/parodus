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
#include "ParodusInternal.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
/* none */

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

/** 
 * @brief createMutex Nopoll create mutex handler
 */ 
noPollPtr createMutex()
{
    pthread_mutexattr_t attr;
    pthread_mutex_t * mutex;
    int rtn;

    mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));

    if (mutex == NULL) {
        ParodusError("Failed to create mutex\n");
        return NULL;
    }
    pthread_mutexattr_init( &attr);
    /*pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK);*/
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE);
    
    /* init the mutex using default values */
    rtn = pthread_mutex_init (mutex, &attr);
    pthread_mutexattr_destroy (&attr);
    if (rtn != 0) {
        ParodusError("Error in init Mutex\n");
        free(mutex);
        return NULL;
    } else {
        ParodusPrint("mutex init successfully\n");
    } 

    return mutex;
}



/** 
 * @brief lockMutex Nopoll lock mutex handler
 */
void lockMutex(noPollPtr _mutex)
{
    int rtn;
    char errbuf[100];

    if (_mutex == NULL) {
        ParodusError("Received null mutex\n");
        return;
    }
    pthread_mutex_t * mutex = _mutex;

    /* lock the mutex */
    rtn = pthread_mutex_lock (mutex);
    if (rtn != 0) {
        strerror_r (rtn, errbuf, 100);
        ParodusError("Error in Lock mutex: %s\n", errbuf);
        /* do some reporting */
        return;
    }
    return;
}


/** 
 * @brief unlockMutex Nopoll unlock mutex handler
 */
void unlockMutex(noPollPtr _mutex)
{
    int rtn;
    char errbuf[100];

    if (_mutex == NULL) {
        ParodusError("Received null mutex\n");
        return;
    }
    pthread_mutex_t * mutex = _mutex;

    /* unlock mutex */
    rtn = pthread_mutex_unlock (mutex);
    if (rtn != 0) {
        /* do some reporting */
        strerror_r (rtn, errbuf, 100);
        ParodusError("Error in unlock mutex: %s\n", errbuf);
        return;
    }
    return;
}



/** 
 * @brief destroyMutex Nopoll destroy mutex handler
 */
void destroyMutex(noPollPtr _mutex)
{
    if (_mutex == NULL) {
        ParodusError("Received null mutex\n");
        return;
    }
    pthread_mutex_t * mutex = _mutex;
    
    if (pthread_mutex_destroy (mutex) != 0) {
        /* do some reporting */
        ParodusError("problem in destroy\n");
        return;
    } else {
        ParodusPrint("Mutex destroyed \n");
    }
    free(mutex);

    return;
}


/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/
/* none */
