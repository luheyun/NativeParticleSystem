#ifndef __PLATFORMSEMAPHORE_H
#define __PLATFORMSEMAPHORE_H

#if SUPPORT_THREADS

#include "Utilities/NonCopyable.h"

#if UNITY_WINRT
#include "PlatformDependent/MetroPlayer/Win32Threads.h"
#endif

class PlatformSemaphore : public NonCopyable
{
	friend class Semaphore;
protected:
	void Create();
	void Destroy();
	
	void WaitForSignal();
	void Signal();
	void Reset() { Destroy(); Create(); }
	
public:
	HANDLE GetHandle() const { return m_Semaphore; }

private:
	HANDLE	m_Semaphore;
};

	inline void PlatformSemaphore::Create()
	{
#if UNITY_WINRT
		m_Semaphore = CreateSemaphoreExW(NULL, 0, INT_MAX, NULL, 0, (STANDARD_RIGHTS_REQUIRED | SEMAPHORE_MODIFY_STATE | SYNCHRONIZE));
#elif defined(UNITY_WIN_API_SUBSET)
		m_Semaphore = CreateSemaphoreEx(NULL, 0, INT_MAX, NULL, 0, (STANDARD_RIGHTS_REQUIRED | SEMAPHORE_MODIFY_STATE | SYNCHRONIZE));
#else
		m_Semaphore = CreateSemaphoreA( NULL, 0, INT_MAX, NULL );
		
#endif
	}
	inline void PlatformSemaphore::Destroy(){ if( m_Semaphore ) CloseHandle(m_Semaphore); }
	inline void PlatformSemaphore::WaitForSignal()
	{
		while (1)
		{
			DWORD result = WaitForSingleObjectEx( m_Semaphore, INFINITE, TRUE );
			switch (result)
			{
			case WAIT_OBJECT_0:
				// We got the signal
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
				break;
			}
		}
	}
	inline void PlatformSemaphore::Signal() { ReleaseSemaphore( m_Semaphore, 1, NULL ); }

#endif // SUPPORT_THREADS

#endif // __PLATFORMSEMAPHORE_H
