#ifndef THREAD_H
#define THREAD_H

enum ThreadPriority { kLowPriority = 0, kBelowNormalPriority = 1, kNormalPriority = 2, kHighPriority = 4 };

#if SUPPORT_THREADS

#if !UNITY_PLUGIN && !UNITY_EXTERNAL_TOOL
#include "Configuration/UnityConfigure.h"
#endif

#if UNITY_WIN || UNITY_XENON
#	include "Winapi/PlatformThread.h"
#elif UNITY_POSIX || UNITY_PS3
#	include "Posix/PlatformThread.h"
#else
#	include "PlatformThread.h"
#endif

#define ASSERT_RUNNING_ON_MAIN_THREAD { Assert(Thread::EqualsCurrentThreadIDForAssert(Thread::mainThreadId)); }
#define DEBUG_ASSERT_RUNNING_ON_MAIN_THREAD { DebugAssert(Thread::EqualsCurrentThreadIDForAssert(Thread::mainThreadId)); }

#ifndef DEFAULT_UNITY_THREAD_STACK_SIZE
#define DEFAULT_UNITY_THREAD_STACK_SIZE 0
#endif

#ifndef DEFAULT_UNITY_THREAD_PROCESSOR
#define DEFAULT_UNITY_THREAD_PROCESSOR -1
#endif

#include "Utilities/NonCopyable.h"

/**
 *  A thread.
 */
class EXPORT_COREMODULE Thread : public NonCopyable
{
	friend class ThreadHelper;
	friend class PlatformThread;

public:
	typedef PlatformThread::ThreadID ThreadID;

public:
	Thread();
	~Thread();

	void Run (void* (*entry_point) (void*), void* data, const UInt32 stackSize = DEFAULT_UNITY_THREAD_STACK_SIZE, int processor = DEFAULT_UNITY_THREAD_PROCESSOR);
	bool IsRunning() const { return m_Running; }
	
	// Was the thread told to stop running?
	bool IsQuitSignaled () const { return m_ShouldQuit; }
	// Tells the thread to stop running, the thread main loop is responsible for checking this variable
	void SignalQuit ();
	
	// Signals quit and waits until the thread main function is exited. Must be called for all
	// threads in order to clean up their resources.
	void WaitForExit(bool signalQuit = true);

	// Stores the pointer only
	void SetName (const char* name) { m_Name = name; }

	static void Sleep (double seconds);
	
	void SetPriority (ThreadPriority prior);
	ThreadPriority GetPriority () const { return m_Priority; }
	
	void SetTempAllocatorSize(size_t size){ m_TempAllocSize = size;}

	static void SetCurrentThreadProcessor(int processor);
	
	static ThreadID GetCurrentThreadID();
	static bool EqualsCurrentThreadID(ThreadID thread) { return GetCurrentThreadID() == thread; }
	static bool EqualsCurrentThreadIDForAssert(ThreadID thread) { return EqualsCurrentThreadID(thread); }
	static bool EqualsThreadID (Thread::ThreadID lhs, Thread::ThreadID rhs) { return lhs == rhs; }
	
	void ExternalEarlySetRunningFalse() { m_Running = false; }
	
	static double GetThreadRunningTime(ThreadID thread);
	PlatformThread& GetThread() { return m_Thread; }
	static ThreadID mainThreadId;
	
	static bool CurrentThreadIsMainThread() { return EqualsCurrentThreadID(mainThreadId); }
	
private:
	PlatformThread	m_Thread;

	void*     m_UserData;
	void* (*m_EntryPoint) (void*);

	volatile bool   m_Running;
	volatile bool   m_ShouldQuit;
	ThreadPriority  m_Priority;
	size_t          m_TempAllocSize;
	const char*     m_Name;

	static UNITY_THREAD_FUNCTION_RETURN_SIGNATURE RunThreadWrapper (void* ptr);
};


#else // SUPPORT_THREADS


// No threads in this platform, stub minimal functionality

#define ASSERT_RUNNING_ON_MAIN_THREAD
#define DEBUG_ASSERT_RUNNING_ON_MAIN_THREAD

class Thread {
public:
	static void Sleep (double t) { }
	static bool CurrentThreadIsMainThread() { return true; }
};


#endif //SUPPORT_THREADS

#endif
