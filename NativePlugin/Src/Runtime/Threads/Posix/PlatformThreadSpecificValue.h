#ifndef PLATFORMTHREADSPECIFICVALUE_H
#define PLATFORMTHREADSPECIFICVALUE_H

#if UNITY_DYNAMIC_TLS

#include <pthread.h>
#include "Runtime/Utilities/LogAssert.h"
#include "Runtime/Utilities/Utility.h"

class PlatformThreadSpecificValue
{
public:
	PlatformThreadSpecificValue();
	~PlatformThreadSpecificValue();

	void* GetValue() const;
	void SetValue(void* value);

private:
	pthread_key_t m_TLSKey;
};

inline PlatformThreadSpecificValue::PlatformThreadSpecificValue()
{
	int rc = pthread_key_create(&m_TLSKey, NULL);
	DebugAssert (rc == 0);
	UNUSED(rc);
}

inline PlatformThreadSpecificValue::~PlatformThreadSpecificValue()
{
	int rc = pthread_key_delete(m_TLSKey);
	DebugAssert (rc == 0);
	UNUSED(rc);
}

#if UNITY_OSX || UNITY_IOS || UNITY_TVOS
inline void * apple_pthread_getspecific_direct(unsigned long slot)
{
	void *ret;
#if defined(__i386__) || defined(__x86_64__)
	__asm__("mov %%gs:%1, %0" : "=r" (ret) : "m" (*(void **)(slot * sizeof(void *))));
#elif (defined(__arm__) && (defined(_ARM_ARCH_6) || defined(_ARM_ARCH_5)))
	void **__pthread_tsd;
#if defined(__arm__) && defined(_ARM_ARCH_6)
	uintptr_t __pthread_tpid;
	__asm__("mrc p15, 0, %0, c13, c0, 3" : "=r" (__pthread_tpid));
	__pthread_tsd = (void**)(__pthread_tpid & ~0x3ul);
#elif defined(__arm__) && defined(_ARM_ARCH_5)
	register uintptr_t __pthread_tpid asm ("r9");
	__pthread_tsd = (void**)__pthread_tpid;
#endif
	ret = __pthread_tsd[slot];
#elif defined(__arm64__)
	void* pthread_getspecific(pthread_key_t key); //TODO
	ret = pthread_getspecific(slot);
#else
#error no _pthread_getspecific_direct implementation for this arch
#endif
	return ret;
}

/* To be used with static constant keys only */
__inline__ static void apple_pthread_setspecific_direct(unsigned long slot, void * val)
{
#if defined(__i386__)
#if defined(__PIC__)
	__asm__("movl %1,%%gs:%0" : "=m" (*(void **)(slot * sizeof(void *))) : "rn" (val));
#else
	__asm__("movl %1,%%gs:%0" : "=m" (*(void **)(slot * sizeof(void *))) : "ri" (val));
#endif
#elif defined(__x86_64__)
	/* PIC is free and cannot be disabled, even with: gcc -mdynamic-no-pic ... */
	__asm__("movq %1,%%gs:%0" : "=m" (*(void **)(slot * sizeof(void *))) : "rn" (val));
#elif (defined(__arm__) && (defined(_ARM_ARCH_6) || defined(_ARM_ARCH_5)))
	void **__pthread_tsd;
#if defined(__arm__) && defined(_ARM_ARCH_6)
	uintptr_t __pthread_tpid;
	__asm__("mrc p15, 0, %0, c13, c0, 3" : "=r" (__pthread_tpid));
	__pthread_tsd = (void**)(__pthread_tpid & ~0x3ul);
#elif defined(__arm__) && defined(_ARM_ARCH_5)
	register uintptr_t __pthread_tpid asm ("r9");
	__pthread_tsd = (void**)__pthread_tpid;
#endif
	__pthread_tsd[slot] = val;
#elif defined(__arm64__)
	int pthread_setspecific(pthread_key_t key, const void* value); //TODO
	pthread_setspecific(slot, val);
#else
#error no _pthread_setspecific_direct implementation for this arch
#endif
}

#endif


inline void* PlatformThreadSpecificValue::GetValue() const
{
#if !UNITY_LINUX && !UNITY_WEBGL
	// 0 is a valid key on Linux and POSIX specifies keys as opaque objects,
	// so technically we have no business snopping in them anyway...
	DebugAssert (m_TLSKey != 0);
#endif

#if UNITY_OSX || UNITY_IOS || UNITY_TVOS
	return apple_pthread_getspecific_direct(m_TLSKey);
#else
	return pthread_getspecific(m_TLSKey);
#endif
}

inline void PlatformThreadSpecificValue::SetValue(void* value)
{
#if !UNITY_LINUX && !UNITY_WEBGL
	// 0 is a valid key on Linux and POSIX specifies keys as opaque objects,
	// so technically we have no business snopping in them anyway...
	DebugAssert (m_TLSKey != 0);
#endif
	#if UNITY_OSX || UNITY_IOS
	apple_pthread_setspecific_direct(m_TLSKey, value);
	#else
	pthread_setspecific(m_TLSKey, value);
	#endif
}

#else

	#error "POSIX doesn't define a static TLS path"

#endif // UNITY_DYNAMIC_TLS

#endif // PLATFORMTHREADSPECIFICVALUE_H
