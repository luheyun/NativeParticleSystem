#include "PluginPrefix.h"

#if SUPPORT_THREADS
#include "Thread.h"
#include "ThreadUtility.h"
#include "ThreadHelper.h"

#if UNITY_APPLE
#include <mach/mach.h>
#include <dlfcn.h>
#endif
#if UNITY_ANDROID
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
#if UNITY_WINRT
#include "PlatformDependent/MetroPlayer/Win32Threads.h"
#endif

void ThreadHelper::Sleep(double seconds)
{
#if UNITY_WINRT
	int milliseconds = (int)(seconds * 1000.0);
	//::SleepEx(milliseconds, true); // Must be alertable so that mono runtime can request thread suspend

	HANDLE const event = CreateEventExW(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
	Assert(NULL != event);

	if (NULL != event)
	{
		WaitForSingleObjectEx(event, milliseconds, TRUE);

		CloseHandle(event);
	}

#elif UNITY_WIN || UNITY_XENON || UNITY_XBOXONE
	int milliseconds = (int)(seconds * 1000.0);
	::SleepEx(milliseconds, true); // Must be alertable so that mono runtime can request thread suspend
#elif THREAD_API_PTHREAD || UNITY_POSIX
	timespec ts;
	int iseconds = FloorfToInt(seconds);
	double micro = (seconds - iseconds) * 1000000.0;
	int nano = (int)micro * 1000;
	ts.tv_sec = iseconds;
	ts.tv_nsec = nano;

	//  nanosleep takes a timespec that is an offset, not
	//  an absolute time.
	nanosleep(&ts, 0); // note: the spleep is aborted if the thread receives a signal. It will return -1 and set errno to EINTR.
#else
#error Unknown OS API - not POSIX nor WinAPI

#endif

}

void ThreadHelper::SetCurrentThreadName(const char* name)
{
	SetThreadNameInternal( -1, name );
}

void ThreadHelper::SetThreadName(const Thread* thread)
{
#if (UNITY_WIN || UNITY_XENON) && !UNITY_WINRT
	SetThreadNameInternal( thread->m_Thread.m_ThreadId, thread->m_Name );
#else
	// Legacy behavior is to set the name of the current thread.
	SetThreadNameInternal( -1, thread->m_Name );
#endif
}

void ThreadHelper::SetThreadNameInternal(const unsigned int threadID, const char* name)
{
	if( !name )
	{
		return;
	}

#if (UNITY_WIN && !UNITY_WINRT)
	const DWORD MS_VC_EXCEPTION=0x406D1388;

	#pragma pack(push,8)
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.
		LPCSTR szName; // Pointer to name (in user addr space).
		DWORD dwThreadID; // Thread ID (-1=caller thread).
		DWORD dwFlags; // Reserved for future use, must be zero.
	} THREADNAME_INFO;
	#pragma pack(pop)

	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = threadID;
	info.dwFlags = 0;

	__try
	{
		RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
#elif UNITY_XENON
	typedef struct tagTHREADNAME_INFO {
		DWORD dwType;     // Must be 0x1000
		LPCSTR szName;    // Pointer to name (in user address space)
		DWORD dwThreadID; // Thread ID (-1 for caller thread)
		DWORD dwFlags;    // Reserved for future use; must be zero
	} THREADNAME_INFO;

	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = threadID;
	info.dwFlags = 0;

	__try
	{
		RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD *)&info );
	}
	__except( GetExceptionCode()==0x406D1388 ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_EXECUTE_HANDLER )
	{
	}
#elif UNITY_APPLE
	// pthread_setname_np is OSX 10.6 and later only
	int (*dynamic_pthread_setname_np)(const char*);
	*reinterpret_cast<void**>(&dynamic_pthread_setname_np) = dlsym(RTLD_DEFAULT, "pthread_setname_np");
	if (dynamic_pthread_setname_np)
		dynamic_pthread_setname_np(name);
#elif UNITY_ANDROID || UNITY_TIZEN
	prctl(PR_SET_NAME, (unsigned long)(name ? name : "<Unknown>"),0,0,0);
#endif
}

void ThreadHelper::SetThreadProcessor(const Thread* thread, int processor)
{
	if (processor == DEFAULT_UNITY_THREAD_PROCESSOR)
		return;
#if UNITY_OSX
	if (thread == NULL ||
		thread->m_Thread.m_Thread == Thread::GetCurrentThreadID())
	{
		#define THREAD_AFFINITY_POLICY         4
		integer_t tap = 1 + processor;
		thread_policy_set(mach_thread_self(), THREAD_AFFINITY_POLICY, (integer_t*) &tap, 1);
	}
#elif UNITY_XBOXONE
	// We interpret 'processor' here as an affinity mask rather than a processor.
	HANDLE hThread = thread == NULL ? GetCurrentThread() : thread->m_Thread.m_Thread;
	Assert( processor > 0 );
	SetThreadAffinityMask(hThread, processor);
#elif UNITY_WIN // NOTE this is defined for XB1 so this needs to be after that in the switch
	// We interpret 'processor' here as processor core
//	HANDLE hThread = thread == NULL ? GetCurrentThread() : thread->m_Thread.m_Thread;
//	DWORD affinity = systeminfo::GetCoreAffinityMask(processor);
//	SetThreadAffinityMask(hThread, affinity);
#elif UNITY_XENON
	HANDLE hThread = thread == NULL ? GetCurrentThread() : thread->m_Thread.m_Thread;
	Assert (processor <= 5);
	XSetThreadProcessor(hThread, processor);
#elif UNITY_ANDROID
	syscall(__NR_sched_setaffinity, thread ? thread->m_Thread.m_LinuxTID : gettid(), sizeof(processor), &processor);
#endif
}

double ThreadHelper::GetThreadRunningTime(Thread::ThreadID thread)
{
#if UNITY_APPLE
	mach_port_t mach_thread = pthread_mach_thread_np(thread);
	thread_basic_info_data_t info;
	mach_msg_type_number_t size = sizeof(thread_basic_info_data_t)/sizeof(integer_t);
	if (thread_info (mach_thread, THREAD_BASIC_INFO, (integer_t*)&info, &size))
		return 0;

	return (double)info.user_time.microseconds / 1000000.0f + info.user_time.seconds
		+ (double)info.system_time.microseconds / 1000000.0f + info.system_time.seconds;
#elif (UNITY_WIN && !UNITY_WINRT) || UNITY_XENON
	double time = 0.0;

	HANDLE threadHandle = OpenThread(THREAD_QUERY_INFORMATION, FALSE, thread);

	if (NULL != threadHandle)
	{
		FILETIME creationTime, exitTime, kernelTime, userTime;

		if (GetThreadTimes(threadHandle, &creationTime, &exitTime, &kernelTime, &userTime))
		{
			ULARGE_INTEGER largeKernelTime = { kernelTime.dwLowDateTime, kernelTime.dwHighDateTime };
			ULARGE_INTEGER largeUserTime = { userTime.dwLowDateTime, userTime.dwHighDateTime };

			time = ((largeKernelTime.QuadPart + largeUserTime.QuadPart) / 10000000.0);
		}

		CloseHandle(threadHandle);
	}
	return time;
#else
	return 0.0;
#endif
}


#endif
