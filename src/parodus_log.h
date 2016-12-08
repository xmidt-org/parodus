#include <stdarg.h>



#define LEVEL_ERROR 0
#define LEVEL_INFO  1
#define LEVEL_DEBUG 2


/**
 * @brief Enables or disables debug logs.
 */

#define ParodusError(...)                   printf("Error: "__VA_ARGS__)
#define ParodusInfo(...)                    printf("Info: "__VA_ARGS__)
#define ParodusPrint(...)                   printf("Debug: "__VA_ARGS__)

