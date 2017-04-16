#include "PluginPrefix.h"

#if SUPPORT_THREADS

#ifndef THREAD_API_WINAPI
#define THREAD_API_WINAPI (UNITY_WIN || UNITY_XENON || UNITY_WINRT)
#endif

#endif // SUPPORT_THREADS

#if THREAD_API_WINAPI

#include "PlatformThread.h"
#include "Runtime/Threads/Thread.h"
#include "Runtime/Threads/ThreadHelper.h"

#include "Runtime/Utilities/Word.h"

// module@TODO : Move this to PlatformThread.h
#if UNITY_WINRT
#include "PlatformDependent/MetroPlayer/Win32Threads.h"
using Windows::System::Threading::WorkItemPriority;
#endif

// This file is used by platforms like Xbox and even Sony platforms (for CGBatch library?)
// But they don't have winutils::ErrorCodeToMsg implemented
#if UNITY_EDITOR || UNITY_STANDALONE_WIN
#include "PlatformDependent/Win/WinUtils.h"
#define THREAD_ERROR_TEXT() (winutils::ErrorCodeToMsg(GetLastError()).c_str())
#else
#define THREAD_ERROR_TEXT() ""
#endif

PlatformThread::PlatformThread()
:	m_Thread(NULL)
#if !UNITY_WINRT
,	m_ThreadId(0)
#endif
{
}

PlatformThread::~PlatformThread()
{
}


void PlatformThread::Create(const Thread* thread, const UInt32 stackSize, const int processor)
{
#if UNITY_WINRT
	WorkItemPriority priority;
	ThreadPriority const threadPriority = thread->GetPriority();
	switch (threadPriority)
	{
	case kLowPriority:
	case kBelowNormalPriority:
		priority = WorkItemPriority::Low;
		break;
	case kNormalPriority:
		priority = WorkItemPriority::Normal;
		break;
	case kHighPriority:
		priority = WorkItemPriority::High;
		break;
	default:
		AssertString(Format("Unsupported thread priorty (%d). Defaulting to normal.", threadPriority));
		priority = WorkItemPriority::Normal;
		break;
	}
	m_Thread = win32::CreateThread(Thread::RunThreadWrapper, (LPVOID)thread, priority);
#else // UNITY_WINRT
	DWORD creationFlags = 0;

	m_Thread = ::CreateThread(NULL, stackSize, Thread::RunThreadWrapper, (LPVOID) thread, creationFlags, &m_ThreadId);

#endif // UNITY_WINRT

}

void PlatformThread::Enter(const Thread* thread)
{
	if (thread->m_Priority != kNormalPriority)
		UpdatePriority(thread);
}

void PlatformThread::Exit(const Thread* thread, void* result)
{
}

void PlatformThread::Join(const Thread* thread)
{
#if !UNITY_WINRT		// Why doesn't WINRT store the thread ID ?
	if (Thread::EqualsCurrentThreadID(m_ThreadId))
	{
		//ErrorStringMsg("***Thread '%s' tried to join itself!***", thread->m_Name);
	}
#endif

	if (thread->m_Running)
	{
		DWORD waitResult = WaitForSingleObjectEx(m_Thread, INFINITE, FALSE);
	}

	if (m_Thread != NULL)
	{
		BOOL closeResult = CloseHandle(m_Thread);
	}
	m_Thread = NULL;
}

void PlatformThread::UpdatePriority(const Thread* thread) const
{
#if !UNITY_WINRT
	ThreadPriority p = thread->m_Priority;
	int iPriority;
	switch (p)
	{
	case kLowPriority:
			iPriority = THREAD_PRIORITY_LOWEST;
			break;

		case kBelowNormalPriority:
			iPriority = THREAD_PRIORITY_BELOW_NORMAL;
			break;

		case kNormalPriority:
			iPriority = THREAD_PRIORITY_NORMAL;
			break;
		case kHighPriority:
			iPriority = THREAD_PRIORITY_HIGHEST;
			break;

		default:
			break;
	}

	int res = SetThreadPriority(m_Thread, iPriority);
// Known failure! described in http://fogbugz.unity3d.com/default.asp?649821
// disable assertion for now, since it is causing automation instabilities.
// AssertFormatMsg (res != 0, "SetThreadPriority failed, name = '%s', thread = 0x%08X, priority = %d, error: (%s)", thread->m_Name, thread, p, THREAD_ERROR_TEXT());
#endif
}

PlatformThread::ThreadID PlatformThread::GetCurrentThreadID()
{
	return GetCurrentThreadId();
}

#endif // THREAD_API_PTHREAD
