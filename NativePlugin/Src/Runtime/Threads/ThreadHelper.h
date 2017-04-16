#ifndef THREADHELPER_H
#define THREADHELPER_H

#if SUPPORT_THREADS

#include "Thread.h"

// ThreadHelper is typically implemented on a per-platform basis, as it contains OS
// specific functionality outside regular POSIX / pthread / WinAPI threads.

class ThreadHelper
{
	friend class Thread;
	friend class PlatformThread;

protected:
	static void Sleep(double time);

	static void SetThreadName(const Thread* thread);
	static void SetThreadProcessor(const Thread* thread, int processor);

	static double GetThreadRunningTime(Thread::ThreadID thread);

public:
	static void SetCurrentThreadAffinity(int processorMask) { SetThreadProcessor(NULL,processorMask); }

	// Set the name of the thread this function is called from.  If you are on a
	// thread managed by Unity, you should use the functions on the Thread object
	// associated with your thread.
	static void SetCurrentThreadName(const char* name);

private:

	// Set the name of the given thread if the platform supports it.  Pass -1 if
	// you explicitly want to set the current thread's name.
	// If the platform does not support setting a thread's name by its id but does
	// support naming the current running thread, then the current thread will
	// have its name set.
	static void SetThreadNameInternal(const unsigned int threadID_or_unused, const char* name);

	ThreadHelper();
};

#endif //SUPPORT_THREADS

#endif
