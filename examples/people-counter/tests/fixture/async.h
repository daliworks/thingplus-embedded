#ifndef _ASYNC_H_
#define _ASYNC_H_

extern "C" {
#include <pthread.h>
}

static pthread_mutex_t _asyncronous_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void asyncronous_wait(void)
{
	pthread_mutex_lock(&_asyncronous_mutex);
}

static inline void asyncronous_done(void)
{
	pthread_mutex_unlock(&_asyncronous_mutex);
}
#define asyncronous_setup asyncronous_wait 
#define asyncronous_teardown asyncronous_done

#endif //#ifndef _ASYNC_H_
