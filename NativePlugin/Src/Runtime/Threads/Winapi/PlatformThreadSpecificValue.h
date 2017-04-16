#ifndef PLATFORMTHREADSPECIFICVALUE_H
#define PLATFORMTHREADSPECIFICVALUE_H

#if UNITY_DYNAMIC_TLS

#if UNITY_WINRT
#include "PlatformDependent/MetroPlayer/Win32Threads.h"
using win32::TlsAlloc;
using win32::TlsFree;
using win32::TlsGetValue;
using win32::TlsSetValue;
#endif

class PlatformThreadSpecificValue
{
public:
	PlatformThreadSpecificValue();
	~PlatformThreadSpecificValue();

	void* GetValue() const;
	void SetValue(void* value);

private:
	DWORD	m_TLSKey;
};

inline PlatformThreadSpecificValue::PlatformThreadSpecificValue ()
{
	m_TLSKey = TlsAlloc();
}

inline PlatformThreadSpecificValue::~PlatformThreadSpecificValue ()
{
	TlsFree( m_TLSKey );
}

inline void* PlatformThreadSpecificValue::GetValue () const
{
#if UNITY_WIN
	void* result = TlsGetValue(m_TLSKey);
	return result;
#elif UNITY_XENON
	return TlsGetValue(m_TLSKey);	// on XENON TlsGetValue does not call SetLastError
#endif
}

inline void PlatformThreadSpecificValue::SetValue (void* value)
{
	BOOL ok = TlsSetValue( m_TLSKey, value );
}

#else

	#define UNITY_TLS_VALUE(type) __declspec(thread) type

#endif // UNITY_DYNAMIC_TLS

#endif // PLATFORMTHREADSPECIFICVALUE_H
