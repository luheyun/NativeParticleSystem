#include "UnityPrefix.h"

#if SUPPORT_THREADS

#ifndef MUTEX_API_PTHREAD
#define MUTEX_API_PTHREAD UNITY_POSIX
#endif

#endif // SUPPORT_THREADS

#if MUTEX_API_PTHREAD
// -------------------------------------------------------------------------------------------------
//  pthreads

#include "PlatformMutex.h"

PlatformMutex::PlatformMutex ( )
{
	pthread_mutexattr_t    attr;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
	pthread_mutex_init(&mutex, &attr);
	pthread_mutexattr_destroy(&attr);
}

PlatformMutex::~PlatformMutex ()
{
	pthread_mutex_destroy(&mutex);
}

void PlatformMutex::Lock()
{
	pthread_mutex_lock(&mutex);
}

void PlatformMutex::Unlock()
{
	pthread_mutex_unlock(&mutex);
}

bool PlatformMutex::TryLock()
{
	return pthread_mutex_trylock(&mutex) == 0;
}

#endif // MUTEX_API_PTHREAD
