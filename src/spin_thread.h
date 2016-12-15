#ifndef _SPIN_THREAD_H_
#define _SPIN_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

void StartThread(void *(*start_routine) (void *));


#ifdef __cplusplus
}
#endif

#endif


