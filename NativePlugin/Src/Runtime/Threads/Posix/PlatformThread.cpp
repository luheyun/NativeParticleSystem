#include "UnityPrefix.h"

#if SUPPORT_THREADS

#ifndef THREAD_API_PTHREAD
#define THREAD_API_PTHREAD (UNITY_POSIX || UNITY_PS3)
#endif

#endif // SUPPORT_THREADS

#if THREAD_API_PTHREAD

#include "PlatformThread.h"
#include "Runtime/Threads/Thread.h"
#include "Runtime/Threads/ThreadHelper.h"

#include "Runtime/Utilities/Word.h"
//#include "Runtime/Utilities/Utility.h"

// module@TODO : Move this to PlatformThread.h
#if UNITY_PS3
#	include <sys/timer.h>
#	include "pthread_ext/pthread_ext.h"
#	define pthread_create pthread_ext_create
#endif

#if UNITY_ANDROID
#include <unistd.h>
#include <jni.h>
	JavaVM* GetJavaVm();
#endif

PlatformThread::PlatformThread()
:	m_Thread((ThreadID)NULL)
,	m_DefaultPriority()
,	m_Processor()
#if UNITY_ANDROID
,	m_LinuxTID()
#endif
{
}

PlatformThread::~PlatformThread()
{
	AssertMsg(m_Thread == (ThreadID)NULL, "***Thread was not cleaned up!***");
}


void PlatformThread::Create(const Thread* thread, const UInt32 stackSize, const int processor)
{
	DebugAssertMsg(m_Thread == (ThreadID)NULL,
				   "Attempting to create a thread on top of non-cleanep up thread");
	m_DefaultPriority = 0;
	m_Processor = processor;

	if(stackSize)
	{
		pthread_attr_t attr;
		memset(&attr, 0, sizeof(attr));

// module@TODO : Implement pthread_attr_init/pthread_attr_setstacksize in PlatformThread.h
#if UNITY_PS3
		attr.stacksize = stackSize;
		attr.name = "_UNITY_";
#else
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, stackSize);
#endif
		pthread_create(&m_Thread, &attr, Thread::RunThreadWrapper, (void*)thread);
	}
	else {
		pthread_create(&m_Thread, NULL, Thread::RunThreadWrapper, (void*)thread);
	}

#if UNITY_APPLE || UNITY_PS3
	struct sched_param param;
	int outputPolicy;
	int err = pthread_getschedparam(m_Thread, &outputPolicy, &param);
	AssertFormatMsg(err == 0, "Error querying default thread scheduling params: %d", err);
	if (err == 0)
		m_DefaultPriority = param.sched_priority;
#endif

	if (thread->m_Priority != kNormalPriority)
		UpdatePriority(thread);
}

void PlatformThread::Enter(const Thread* thread)
{
#if UNITY_ANDROID
	m_LinuxTID = gettid();
#endif
	ThreadHelper::SetThreadProcessor(thread, m_Processor);
}

void PlatformThread::Exit(const Thread* thread, void* result)
{
#if UNITY_ANDROID
	GetJavaVm()->DetachCurrentThread();
	pthread_exit(result);
#endif
}

void PlatformThread::Join(const Thread* thread)
{
	if (Thread::EqualsCurrentThreadID(m_Thread))
	{
		ErrorStringMsg("***Thread '%s' tried to join itself!***", thread->m_Name);
	}

	if (m_Thread)
	{
		int error = pthread_join(m_Thread, NULL);
		if (error)
			ErrorString(Format("Error joining threads: %d", error));

		m_Thread = 0;
	}
}

void PlatformThread::UpdatePriority(const Thread* thread) const
{
#if UNITY_BB10

	// For BB10 the user Unity is run as lacks permission to set priority.

#else	// Default POSIX impl below

	ThreadPriority p = thread->m_Priority;

	struct sched_param param;
	int policy;
	/*int err = */pthread_getschedparam(m_Thread, &policy, &param);
	// Known failure! described in http://fogbugz.unity3d.com/default.asp?649821
	// disable assertion for now, since it is causing automation instabilities.
	// AssertFormatMsg(err == 0, "Error querying default thread scheduling params: %d", err);
#if UNITY_PS3
	const int min = 3071;
	const int max = 0;
#else
	const int min = sched_get_priority_min(policy);
	const int max = sched_get_priority_max(policy);
#endif

	int iPriority;
	switch (p)
	{
		case kLowPriority:
			iPriority = min;
			break;

		case kBelowNormalPriority:
			// TODO: Do we have inverted scale on PS3? Should we do min -...?
			iPriority = min + (m_DefaultPriority-min)/2;
			break;

		case kNormalPriority:
			iPriority = m_DefaultPriority;
			break;

		case kHighPriority:
			iPriority = max;
			break;

		default:
			iPriority = min;
			AssertString("Undefined thread priority");
			break;
	}

	if (param.sched_priority != iPriority)
	{
		param.sched_priority = iPriority;
		/*int err =*/pthread_setschedparam(m_Thread, policy, &param);
		// Known failure! described in http://fogbugz.unity3d.com/default.asp?649821
		// disable assertion for now, since it is causing automation instabilities.
		// AssertFormatMsg(err == 0, "Error setting thread scheduling params: %d", err);
	}
#endif
}

PlatformThread::ThreadID PlatformThread::GetCurrentThreadID()
{
	return (ThreadID)pthread_self();
}

#endif // THREAD_API_PTHREAD
