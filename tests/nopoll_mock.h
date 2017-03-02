bool LastReasonStatus;
bool close_retry;
char *reconnect_reason;
volatile unsigned int heartBeatTimer;
void *metadataPack;
size_t metaPackSize;
pthread_mutex_t close_mut;
pthread_mutex_t g_mutex;
pthread_cond_t g_cond;
