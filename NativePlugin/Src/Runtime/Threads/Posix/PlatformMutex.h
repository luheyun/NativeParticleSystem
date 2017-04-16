#ifndef __PLATFORMMUTEX_H
#define __PLATFORMMUTEX_H

#if SUPPORT_THREADS

#include <pthread.h>
#include "Runtime/Utilities/NonCopyable.h"

/**
 *	A platform/api specific mutex class. Always recursive (a single thread can lock multiple times).
 */
class PlatformMutex : public NonCopyable
{
	friend class Mutex;
protected:
	PlatformMutex();
	~PlatformMutex();

	void Lock();
	void Unlock();
	bool TryLock();

private:

	pthread_mutex_t mutex;
};

#endif // SUPPORT_THREADS
#endif // __PLATFORMMUTEX_H
