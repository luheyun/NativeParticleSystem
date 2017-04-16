#ifndef PLATFORMTHREAD_H
#define PLATFORMTHREAD_H

#if SUPPORT_THREADS

#include "Utilities/NonCopyable.h"

class Thread;

// The function signature is actually important here,
// and on Windows it must match the signature of LPTHREAD_START_ROUTINE,
// and no other way will be "ok".
// It does not suffice to cast to the more "general" version, because
// then you will run into ESP check failures sooner or later. Probably sooner.
#define UNITY_THREAD_FUNCTION_RETURNTYPE DWORD
#define UNITY_THREAD_FUNCTION_RETURN_SIGNATURE UNITY_THREAD_FUNCTION_RETURNTYPE WINAPI

class EXPORT_COREMODULE PlatformThread : public NonCopyable
{
	friend class Thread;
	friend class ThreadHelper;

protected:
	typedef DWORD ThreadID;

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

#if UNITY_WINRT

	HANDLE m_Thread;

#elif UNITY_WIN || UNITY_XENON

	HANDLE m_Thread;
	DWORD m_ThreadId;

#endif
};

#endif // SUPPORT_THREADS

#endif // PLATFORMTHREAD_H
