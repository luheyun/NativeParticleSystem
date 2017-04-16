#pragma once

#include "Utilities/NonCopyable.h"

#define SUPPORT_WORKER_THREAD_MUTEX_LOCK_ASSERT (SUPPORT_THREADS && !UNITY_RELEASE && DEBUGMODE)

#if SUPPORT_THREADS
#if UNITY_WIN || UNITY_XENON
#	include "Winapi/PlatformMutex.h"
#elif UNITY_POSIX
#	include "Posix/PlatformMutex.h"
#else
#	include "PlatformMutex.h"
#endif
#endif // SUPPORT_THREADS

/**
 *	A mutex class. Always recursive (a single thread can lock multiple times).
 */
class Mutex : NonCopyable
{
public:
	
	class AutoLock : NonCopyable
	{
		Mutex& m_Mutex;

	public:
		AutoLock(Mutex& mutex) : m_Mutex(mutex) { m_Mutex.Lock(); }
		~AutoLock() { m_Mutex.Unlock(); }
	};

	Mutex();
	~Mutex();
	
	void Lock();
	void Unlock();

	// Returns true if locking succeeded
	bool TryLock();

	// Returns true if the mutex is currently locked
	bool IsLocked();
	
	void BlockUntilUnlocked();

	// On worker threads, we do not allow any mutex lock calls (using mutex locks can cause significant performance issues).
	// Thus all code must be designed to not cause any locks on worker threads. However, in cases where we do not control
	// the code (such as Physx), we can instantiate an AutoAllowWorkerThreadLock on the stack to temporarily allow the locks
	// for certain operations.
	class AutoAllowWorkerThreadLock : NonCopyable
	{
#	if SUPPORT_WORKER_THREAD_MUTEX_LOCK_ASSERT
		int m_NewRefCount, m_OldRefCount;

	public:
		AutoAllowWorkerThreadLock(bool allow = true);
		~AutoAllowWorkerThreadLock();
#	else
	public:
		AutoAllowWorkerThreadLock(bool = true) { }
#	endif
	};

private:

#	if SUPPORT_THREADS
	PlatformMutex m_Mutex;
#	endif
};
