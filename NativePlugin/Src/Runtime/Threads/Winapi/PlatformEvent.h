#ifndef __PLATFORMEVENT_H
#define __PLATFORMEVENT_H

// Event synchronization object.

#if SUPPORT_THREADS

#include "Runtime/Utilities/NonCopyable.h"

#if UNITY_WINRT
#include "PlatformDependent/MetroPlayer/Win32Threads.h"
#if UNITY_WP_8_1
__if_not_exists(Sleep)
{
	using win32::Sleep;
};
#endif
#endif

class Event : public NonCopyable
{
public:
	explicit Event();
	~Event();

	void WaitForSignal();
	void Signal();

private:
	HANDLE m_Event;
};

inline Event::Event()
{
#if UNITY_WINRT
	m_Event = CreateEventExW(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
#else
	m_Event = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
}

inline Event::~Event()
{
	if (m_Event != NULL)
		CloseHandle(m_Event);
}

inline void Event::WaitForSignal()
{
	while (1)
	{
		DWORD result = WaitForSingleObjectEx(m_Event, INFINITE, TRUE);
		switch (result)
		{
		case WAIT_OBJECT_0:
			// We got the event
			return;
		case WAIT_IO_COMPLETION:
			// Allow thread to run IO completion task
#if UNITY_WINRT && !UNITY_WP_8_1
			win32::Sleep(1);
#else
			Sleep(1);
#endif
			break;
		default:
			Assert (false);
			break;
		}
	}
}

inline void Event::Signal()
{
	SetEvent(m_Event);
}

#endif // SUPPORT_THREADS

#endif // __PLATFORMEVENT_H
