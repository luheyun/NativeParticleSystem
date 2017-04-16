#ifndef __PROFILERMUTEX_H
#define __PROFILERMUTEX_H

#include "Mutex.h"
#include "Thread.h"

#if ENABLE_PROFILER
#include "Runtime/Profiler/Profiler.h"
#include "Runtime/Profiler/TimeHelper.h"
#endif

#define THREAD_LOCK_WARNINGS 0

// Some platform might want to do some checking/show errors (for example on disc eject) instead of just locking up indefinitely.
#if UNITY_PLATFORM_PROFILER_MUTEX 
#include "Threads/PlatformProfilerMutex.h"
#else
#define PLATFORM_PROFILER_MUTEX_LOCK(mutex)		mutex.Lock()
#define PLATFORM_PROFILER_MUTEX_AUTOLOCK		Mutex::AutoLock
#endif

#if THREAD_LOCK_WARNINGS || (ENABLE_PROFILER && SUPPORT_THREADS)

#define ACQUIRE_AUTOLOCK(mutex,profilerInformation) ProfilerMutexAutoLock acquireAutoLock_##mutex (mutex, #mutex, profilerInformation, __FILE__, __LINE__)
#define ACQUIRE_AUTOLOCK_WARN_MAIN_THREAD(mutex,profilerInformation) ProfilerMutexAutoLock acquireAutoLock_##mutex (mutex, #mutex, profilerInformation, __FILE__, __LINE__)
#define LOCK_MUTEX(mutex,profilerInformation) ProfilerMutexLock(mutex,#mutex,profilerInformation,__FILE__,__LINE__)

inline void ProfilerMutexLock (Mutex& mutex, char const* mutexName, ProfilerInformation& information, char const* file, int line)
{
#if ENABLE_PROFILER || THREAD_LOCK_WARNINGS

	if (mutex.TryLock())
		return;

	PROFILER_AUTO(information, NULL)

	#if THREAD_LOCK_TIMING
	TimeFormat beforeLock = GetProfilerTime();
	#endif
	PLATFORM_PROFILER_MUTEX_LOCK(mutex);

	#if THREAD_LOCK_TIMING
	float time = AbsoluteTimeToMilliseconds (ELAPSED_TIME(beforeLock));
	if (time > 0.1F && Thread::CurrentThreadIsMainThread())
	{
		DebugStringToFile (Format("Mutex '%s' obtained after %f ms.", mutexName, time), 0, file, line, kScriptingWarning);
	}
	#endif

	// Without profiler, just lock
	#else
	PLATFORM_PROFILER_MUTEX_LOCK(mutex);
	#endif
	
}

class ProfilerMutexAutoLock
{
public:
	ProfilerMutexAutoLock (Mutex& mutex, char const* mutexName, ProfilerInformation& profilerInformation, char const* file, int line)
	:	m_Mutex (&mutex)
	{
		ProfilerMutexLock(mutex, mutexName, profilerInformation, file, line);
	}

	~ProfilerMutexAutoLock()
	{
		m_Mutex->Unlock();
	}
	
private:
	ProfilerMutexAutoLock(const ProfilerMutexAutoLock&);
	ProfilerMutexAutoLock& operator=(const ProfilerMutexAutoLock&);
	
private:
	Mutex*	m_Mutex;
};

#else

#define ACQUIRE_AUTOLOCK(mutex,profilerInformation) PLATFORM_PROFILER_MUTEX_AUTOLOCK acquireAutoLock_##mutex (mutex)
#define ACQUIRE_AUTOLOCK_WARN_MAIN_THREAD(mutex,profilerInformation) PLATFORM_PROFILER_MUTEX_AUTOLOCK acquireAutoLock_##mutex (mutex)
#define LOCK_MUTEX(mutex,profilerInformation) PLATFORM_PROFILER_MUTEX_LOCK(mutex)


#endif

#endif
