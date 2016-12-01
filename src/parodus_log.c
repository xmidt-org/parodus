#include <stdio.h>
#include <malloc.h>
#include "parodus_log.h"
#include "ParodusInternal.h"


void parodus_log ( int level, const char *msg, ...)
{
	char *pTempChar = NULL;		
	va_list arg;
	int ret = 0;
	unsigned int rdkLogLevel = LOG_DEBUG;

	switch(level)
	{
		case LEVEL_ERROR:
			rdkLogLevel = LOG_ERROR;
			break;

		case LEVEL_INFO:
			rdkLogLevel = LOG_INFO;
			break;

		case LEVEL_DEBUG:
			rdkLogLevel = LOG_DEBUG;
			break;
	}

	if( rdkLogLevel <= LOG_INFO )
	{
	
		pTempChar = (char *)malloc(4096);
		if(pTempChar == NULL)
		{
		    printf("memory allocaion failed inside parodus_log().\n");
		    return;
		}
		else
		{
			va_start(arg, msg);
			ret = vsnprintf(pTempChar, 4096, msg,arg);
			if(ret < 0)
			{
				perror(pTempChar);
			}
			va_end(arg);
			//logging using rdk logger
			#ifdef PARODUS_LOGGER
			RDK_LOG(rdkLogLevel, "LOG.RDK.PARODUS", "%s", pTempChar);
			#endif
			free(pTempChar);
		}
				    
	}
	
	return;	
}



