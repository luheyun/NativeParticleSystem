#include "PluginPrefix.h"

#if SUPPORT_THREADS

#ifndef MUTEX_API_WINAPI
#define MUTEX_API_WINAPI (UNITY_WIN || UNITY_XENON || UNITY_WINRT)
#endif

#endif // SUPPORT_THREADS

#if MUTEX_API_WINAPI

#include "PlatformMutex.h"

// -------------------------------------------------------------------------------------------------
//  windows

// Note: TryEnterCriticalSection only exists on NT-derived systems.
// But we do not run on Win9x currently anyway, so just accept it.
#if !defined _WIN32_WINNT || _WIN32_WINNT < 0x0400
extern "C" WINBASEAPI BOOL WINAPI TryEnterCriticalSection( IN OUT LPCRITICAL_SECTION lpCriticalSection );
#endif

PlatformMutex::PlatformMutex()
{
#if UNITY_WINRT
	BOOL const result = InitializeCriticalSectionEx(&crit_sec, 0, CRITICAL_SECTION_NO_DEBUG_INFO);
	Assert(FALSE != result);
#else
	InitializeCriticalSectionAndSpinCount(&crit_sec, 200);
#endif
}

PlatformMutex::~PlatformMutex ()
{
	DeleteCriticalSection( &crit_sec );
}

void PlatformMutex::Lock()
{
	EnterCriticalSection( &crit_sec );
}

void PlatformMutex::Unlock()
{
	LeaveCriticalSection( &crit_sec );
}

bool PlatformMutex::TryLock()
{
	return TryEnterCriticalSection( &crit_sec ) ? true : false;
}

#endif // MUTEX_API_WINAPI
