#ifndef PLATFORMTHREAD_H
#define PLATFORMTHREAD_H

#if SUPPORT_THREADS

#include <pthread.h>
#include "Runtime/Utilities/NonCopyable.h"

#if UNITY_ANDROID
#include <sys/types.h>
#endif

class Thread;

#if UNITY_PS3	// module@TODO : Move to Platforms/PS3/Include/PlatformThread.h
#define DEFAULT_UNITY_THREAD_STACK_SIZE 128*1024
#endif

#define UNITY_THREAD_FUNCTION_RETURNTYPE void*
#define UNITY_THREAD_FUNCTION_RETURN_SIGNATURE UNITY_THREAD_FUNCTION_RETURNTYPE

class EXPORT_COREMODULE PlatformThread : public NonCopyable
{
	friend class Thread;
	friend class ThreadHelper;

protected:
	typedef pthread_t ThreadID;

	// Starts a thread to execute within the calling process.
	// Typically maps to 'CreateThread' (WinAPI) or 'pthread_create' (POSIX).
	void Create(const Thread* thread, const UInt32 stackSize, const int processor);
	// To be called from the thread's 'start_routine' (aka RunThreadWrapper)
	// in order to boot-strap the thread (in terms of setting the thread affinity or similar).
	void Enter(const Thread* thread);
	// To be called as final exit/cleanup call when a thread's 'start_routine' (aka RunThreadWrapper) exits.
	// Depending on the backing thread API this function may not return.
	void Exit(const Thread* thread, void* result);
	// The function waits for the thread specified by 'thread' to terminate.
	// Typically maps to 'WaitForSingleObject' (WinAPI) or 'pthread_join' (POSIX).
	void Join(const Thread* thread);
	// Uses the thread->m_Priority to update the thread scheduler settings, if possible.
	// Typically maps to 'SetThreadPriority' (WinAPI) or 'pthread_setschedparam' (POSIX).
	// Depending on the process permissions this call may turn into a no-op.
	void UpdatePriority(const Thread* thread) const;
	// Returns a unique identifier for the currently executing thread.
	static ThreadID GetCurrentThreadID();

private:
	PlatformThread();
	~PlatformThread();

	pthread_t m_Thread;
	int m_DefaultPriority;
	int m_Processor;
#if UNITY_ANDROID
	pid_t m_LinuxTID;
#endif
};

#endif // SUPPORT_THREADS

#endif // PLATFORMTHREAD_H
