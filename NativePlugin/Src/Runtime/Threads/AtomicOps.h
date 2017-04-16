#pragma once
// Primitive atomic instructions as defined by the CPU architecture.

#include "Allocator/MemoryMacros.h"	// defines FORCE_INLINE (?!)

// ADD_NEW_PLATFORM_HERE: review this file

#define ATOMIC_API_GENERIC (UNITY_APPLE || UNITY_WIN || UNITY_XENON || UNITY_ANDROID || UNITY_WEBGL || UNITY_LINUX || UNITY_BB10 || UNITY_WII || UNITY_TIZEN || UNITY_STV)
#if !ATOMIC_API_GENERIC && SUPPORT_THREADS
#	include "PlatformAtomicOps.h"
#else

#ifndef HAVE_ATOMIC64_OPS
#	define HAVE_ATOMIC64_OPS 0
#endif
// This file contains implementations for the platforms we already support.
// Going forward (some of) these implementations must move to the platform specific directories.

#include "Utilities/Utility.h"

#if UNITY_WIN
#include <intrin.h>
typedef char UnityAtomicsTypesAssert_FailsIfIntSize_NEQ_LongSize[sizeof(int) == sizeof(LONG) ? 1 : -1];
#elif UNITY_APPLE
#include <libkern/OSAtomic.h>
#elif UNITY_ANDROID || (UNITY_LINUX && defined(__GNUC__) && defined(__arm__))
// use gcc builtin __sync_*
#elif UNITY_BB10
#include <atomic.h>
#include <arm/smpxchg.h>
#elif UNITY_WEBGL
#include <emscripten/threading.h>
#endif

// AtomicAdd - Returns the new value, after the operation has been performed (as defined by OSAtomicAdd32Barrier)
FORCE_INLINE int AtomicAdd (int volatile* i, int value)
{
#if !SUPPORT_THREADS
	return *i+=value;
#elif UNITY_APPLE
	return OSAtomicAdd32Barrier (value, (int*)i);
#elif UNITY_WIN || UNITY_XENON
	return _InterlockedExchangeAdd ((long volatile*)i, value) + value;
#elif UNITY_LINUX || UNITY_ANDROID || UNITY_TIZEN || UNITY_STV || UNITY_WEBGL
	return __sync_add_and_fetch(i, value);
#elif UNITY_BB10
	return atomic_add_value((unsigned int*)i, value)+value;
#else
	#error "Atomic op undefined for this platform"
#endif
}

// AtomicSub - Returns the new value, after the operation has been performed (as defined by OSAtomicSub32Barrier)
FORCE_INLINE
int AtomicSub (int volatile* i, int value)
{
#if UNITY_LINUX || UNITY_ANDROID || UNITY_TIZEN || UNITY_STV || (UNITY_WEBGL && SUPPORT_THREADS)
	return __sync_sub_and_fetch(i, value);
#elif UNITY_BB10
	return atomic_sub_value((unsigned int*)i, value) - value;
#else
	return AtomicAdd(i, -value);
#endif
}

// AtomicIncrement - Returns the new value, after the operation has been performed (as defined by OSAtomicAdd32Barrier)
FORCE_INLINE
int AtomicIncrement (int volatile* i)
{
#if UNITY_WIN || UNITY_XENON
	return _InterlockedIncrement ((long volatile*)i);
#else
	return AtomicAdd(i, 1);
#endif
}

// AtomicDecrement - Returns the new value, after the operation has been performed (as defined by OSAtomicAdd32Barrier)
FORCE_INLINE
int AtomicDecrement (int volatile* i)
{
#if UNITY_WIN || UNITY_XENON
	return _InterlockedDecrement ((long volatile*)i);
#else
	return AtomicSub(i, 1);
#endif
}

// AtomicCompareExchange - Returns value is the initial value of the Destination pointer (as defined by _InterlockedCompareExchange)
FORCE_INLINE
bool AtomicCompareExchange (int volatile* i, int newValue, int expectedValue)
{
#if !SUPPORT_THREADS
	if (*i == expectedValue)
	{
		*i = newValue;
		return true;
	}
	return false;
#elif UNITY_WIN || UNITY_XENON
	return _InterlockedCompareExchange ((long volatile*)i, (long)newValue, (long)expectedValue) == expectedValue;
#elif UNITY_APPLE
	return OSAtomicCompareAndSwap32Barrier (expectedValue, newValue, reinterpret_cast<volatile int32_t*>(i));
#elif UNITY_LINUX || UNITY_ANDROID || UNITY_BB10 || UNITY_TIZEN || UNITY_STV || UNITY_WEBGL
	return __sync_bool_compare_and_swap(i, expectedValue, newValue);
#elif UNITY_BB10
	return _smp_cmpxchg((unsigned int*)i, expectedValue, newValue) == expectedValue;
#else
	#error "AtomicCompareExchange implementation missing"
#endif
}

//AtomicCompareExchangePointer
FORCE_INLINE
bool AtomicCompareExchangePointer (void* volatile* targetAddress, void* newValue, void* expectedValue)
{
#if !SUPPORT_THREADS
	if (*targetAddress == expectedValue)
	{
		*targetAddress = newValue;
		return true;
	}
	return false;
#elif UNITY_WIN || UNITY_XENON
	return InterlockedCompareExchangePointer (targetAddress, newValue, expectedValue) == expectedValue;
#elif UNITY_APPLE
	return OSAtomicCompareAndSwapPtrBarrier (expectedValue, newValue, targetAddress);
#elif UNITY_LINUX || UNITY_ANDROID || UNITY_BB10 || UNITY_TIZEN || UNITY_STV || UNITY_WEBGL
	return __sync_bool_compare_and_swap(targetAddress, expectedValue, newValue);
#elif UNITY_BB10
	return _smp_cmpxchg(targetAddress, expectedValue, newValue) == expectedValue;
#else
	#error "AtomicCompareExchangePointer implementation missing"
#endif
}

// AtomicExchange - Returns the initial value pointed to by Target (as defined by _InterlockedExchange)
FORCE_INLINE
int AtomicExchange (int volatile* i, int value)
{
#if !SUPPORT_THREADS
	int prev = *i;
	*i = value;
	return prev;
#elif UNITY_WIN || UNITY_XENON
	return (int)_InterlockedExchange ((long volatile*)i, (long)value);
#elif UNITY_WEBGL
	return emscripten_atomic_exchange_u32((void*)i, value);
#elif UNITY_ANDROID || UNITY_TIZEN || UNITY_LINUX || UNITY_STV || UNITY_BB10 || UNITY_APPLE // fallback to loop
	int prev;
	do { prev = *i; }
	while (!AtomicCompareExchange(i, value, prev));
	return prev;
#else
	#error "AtomicExchange implementation missing"
#endif
}

#if UNITY_64

// AtomicAdd - Returns the new value, after the operation has been performed (as defined by OSAtomicAdd64Barrier)
FORCE_INLINE SInt64 AtomicAdd64(SInt64 volatile* i, SInt64 value)
{
#if !SUPPORT_THREADS
	return *i += value;
#elif UNITY_APPLE
	return OSAtomicAdd64Barrier((int64_t) value, (volatile int64_t*) i);
#elif UNITY_WIN || UNITY_XENON
	return _InterlockedExchangeAdd64((__int64 volatile*) i, value) + value;
#elif UNITY_LINUX || UNITY_ANDROID || UNITY_TIZEN || UNITY_STV || UNITY_WEBGL
	return __sync_add_and_fetch(i, value);
#else
	#error "Atomic op undefined for this platform"
#endif
}

// AtomicSub - Returns the new value, after the operation has been performed (as defined by OSAtomicSub64Barrier)
FORCE_INLINE SInt64 AtomicSub64(SInt64 volatile* i, SInt64 value)
{
#if UNITY_LINUX || UNITY_ANDROID || UNITY_TIZEN || UNITY_STV || (UNITY_WEBGL && SUPPORT_THREADS)
	return __sync_sub_and_fetch(i, value);
#else
	return AtomicAdd64(i, -value);
#endif
}

FORCE_INLINE
bool AtomicCompareExchange64 (SInt64 volatile* i, SInt64 newValue, SInt64 expectedValue)
{
#if !SUPPORT_THREADS
	if (*i == expectedValue)
	{
		*i = newValue;
		return true;
	}
	return false;
#elif UNITY_WIN || UNITY_XENON
	// Use intrinsic version (with leading underscore) rather than Win32 API version as
	// that is only available from Vista onwards.
	return _InterlockedCompareExchange64 ((__int64 volatile*) i, (__int64) newValue, (__int64) expectedValue) == expectedValue;
#elif UNITY_APPLE
	return OSAtomicCompareAndSwap64Barrier (expectedValue, newValue, reinterpret_cast<volatile int64_t*>(i));
#elif UNITY_LINUX || UNITY_ANDROID || UNITY_BB10 || UNITY_TIZEN || UNITY_STV || UNITY_WEBGL
	return __sync_bool_compare_and_swap(i, expectedValue, newValue);
#else
	#error "AtomicCompareExchange implementation missing"
#endif
}

FORCE_INLINE SInt64 AtomicExchange64 (SInt64 volatile* i, SInt64 value)
{
#if !SUPPORT_THREADS
	SInt64 prev = *i;
	*i = value;
	return prev;
#elif UNITY_WIN || UNITY_XENON
	return _InterlockedExchange64 ((__int64 volatile*) i, (__int64) value);
#elif UNITY_WEBGL
	return emscripten_atomic_exchange_u64((void*)i, value);
#elif UNITY_ANDROID || UNITY_TIZEN || UNITY_LINUX || UNITY_STV || UNITY_BB10 || UNITY_APPLE // fallback to loop
	SInt64 prev;
	do { prev = *i; }
	while (!AtomicCompareExchange64(i, value, prev));
	return prev;
#else
	#error "AtomicExchange implementation missing"
#endif
}

#endif // UNITY_64

#endif // ATOMIC_API_GENERIC
#undef ATOMIC_API_GENERIC

// AtomicAdd for size_t
FORCE_INLINE size_t AtomicAdd(size_t volatile* i, size_t value)
{
#if UNITY_64
	return (size_t) AtomicAdd64((SInt64 volatile*) i, (SInt64) value);
#else
	return (size_t) AtomicAdd((int volatile*) i, (int) value);
#endif
}

// AtomicSub for size_t
FORCE_INLINE size_t AtomicSub(size_t volatile* i, size_t value)
{
#if UNITY_64
	return (size_t) AtomicSub64((SInt64 volatile*) i, (SInt64) value);
#else
	return (size_t) AtomicSub((int volatile*) i, (int) value);
#endif
}

// AtomicExchange for size_t
FORCE_INLINE bool AtomicExchange(size_t volatile* i, size_t value)
{
#if UNITY_64
	return AtomicExchange64((SInt64 volatile*) i, (SInt64) value);
#else
	return AtomicExchange((int volatile*) i, (int) value);
#endif
}

// AtomicCompareExchange for size_t
FORCE_INLINE bool AtomicCompareExchange(size_t volatile* i, size_t newValue, size_t expectedValue)
{
#if UNITY_64
	return AtomicCompareExchange64((SInt64 volatile*) i, (SInt64) newValue, (SInt64) expectedValue);
#else
	return AtomicCompareExchange((int volatile*) i, (int) newValue, (int) expectedValue);
#endif
}
