#include <stdarg.h>



#define LEVEL_ERROR 0
#define LEVEL_INFO  1
#define LEVEL_DEBUG 2


/**
 * @brief Enables or disables debug logs.
 */
#ifdef PARODUS_LOGGER

#define ParodusError(...)                   parodus_log(LEVEL_ERROR, __VA_ARGS__)
#define ParodusInfo(...)                    parodus_log(LEVEL_INFO, __VA_ARGS__)
#define ParodusPrint(...)                   parodus_log(LEVEL_DEBUG, __VA_ARGS__)

#else

#define ParodusError(...)                   printf("Error: "__VA_ARGS__)
#define ParodusInfo(...)                    printf("Info: "__VA_ARGS__)
#define ParodusPrint(...)                   printf("Debug: "__VA_ARGS__)

#endif
void parodus_log (  int level, const char *fmt, ...);
