// configure Unity
// UNITY_RELEASE is true for all non-debug builds
// e.g. everything built by TeamCity
#ifdef _DEBUG
	#define DEBUGMODE 1
	#ifdef UNITY_RELEASE
	#undef UNITY_RELEASE
	#endif
	#define UNITY_RELEASE 0
#else
	#if !defined(DEBUGMODE)
	#if UNITY_EDITOR
		#define DEBUGMODE 1
	#else
		#define DEBUGMODE 0
	#endif
	#endif
	#ifdef UNITY_RELEASE
	#undef UNITY_RELEASE
	#endif
	#define UNITY_RELEASE 1
#endif


// turn off some warnings
#pragma warning(disable:4267) // conversion from size_t to int, possible loss of data
#pragma warning(disable:4244) // conversion from int|double to float, possible loss of data
#pragma warning(disable:4800) // forcing value to bool true/false (performance warning)
#pragma warning(disable:4101) // unreferenced local variable
#pragma warning(disable:4018) // signed/unsigned mismatch
#pragma warning(disable:4675) // resolved overload was found by argument-dependent lookup
#pragma warning(disable:4138) // * / found outside of comment
#pragma warning(disable:4554) // check operator precedence for possible error

#pragma warning(disable:4530) // exception handler used
#pragma warning(disable:4355) //'this' : used in base member initializer list

#if !defined(UNITY_WIN_API_SUBSET)
// Don't include sdkddkver.h, because we might accidentally use API not available on older system
#if UNITY_EDITOR
	// Include APIs from Windows 7
	#define NTDDI_VERSION 0x06010000
	#define _WIN32_WINNT 0x0601
	#define WINVER 0x0601
#else
	// Include APIs from Windows XP SP2 API and up. Needed for 64 bit interlocking operations.
	#define NTDDI_VERSION 0x05020200
	#define _WIN32_WINNT 0x0502
	#define WINVER 0x0502
#endif
#endif

#define WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif

#define OEMRESOURCE
#include <windows.h>

#if defined(_MSC_VER) && (_MSC_VER < 1800)
#define va_copy(a,z) ((void)((a)=(z)))
#endif

#define UNITY_METRO_8_0 0
#define UNITY_METRO_8_1 0
#define UNITY_WP_8_1 0
#define UNITY_UAP 0

#define UNITY_METRO_VS2012 Error - do not use this - Use UNITY_METRO_8_0 instead
#define UNITY_METRO_VS2013 Error - do not use this - Use UNITY_METRO_8_1 instead

#ifndef UNITY_WP_8_0
#define UNITY_WP_8_0 0
#endif

// size_t, ptrdiff_t
#include <cstddef>
