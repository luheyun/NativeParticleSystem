#ifndef __THREAD_UTILITY_H
#define __THREAD_UTILITY_H

#if UNITY_APPLE
#include <libkern/OSAtomic.h>
#endif

// Memory barrier.
//
// Necessary to call this when using volatile to order writes/reads in multiple threads.
// ADD_NEW_PLATFORM_HERE
inline void UnityMemoryBarrier()
{
	#if UNITY_WIN || UNITY_XENON
	#ifdef MemoryBarrier
	MemoryBarrier();
	#else
	long temp;
	__asm xchg temp,eax;
	#endif
	
	#elif UNITY_APPLE
	
	OSMemoryBarrier();
	
	#elif UNITY_PS3

	__lwsync();

	#elif UNITY_PSP2

	// Russ TODO - check to see if __builtin_dmb would be sufficient
	__builtin_dsb();

	#elif UNITY_TIZEN || UNITY_STV || UNITY_ANDROID || UNITY_WEBGL

		__sync_synchronize();

	#elif UNITY_LINUX || UNITY_BB10
		#ifdef __arm__
			__asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 5" : : "r" (0) : "memory");
		#else
			__sync_synchronize();
		#endif
	#else
		// include PlatformThreadUtility.h below
	#endif
}

#endif
