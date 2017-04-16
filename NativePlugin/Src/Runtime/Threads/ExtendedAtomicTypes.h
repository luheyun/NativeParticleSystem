#pragma once

#if defined (__x86_64__) || defined (_M_X64)

#	include <emmintrin.h>

	/// atomic_word must be 16 bytes aligned if you want to use it with atomic_* ops.
#	if defined(_MSC_VER)
	typedef __int64 atomic_word;
#	else
	typedef long long atomic_word;
#	endif

	/// atomic_word2 must be 16 bytes aligned if you want to use it with atomic_* ops.
	union atomic_word2
	{
		__m128i			v;
		struct
		{
			atomic_word lo, hi;
		};
	};
	#define ATOMIC_HAS_DCAS

#elif defined (__x86__) || defined (__i386__) || defined (_M_IX86)

	/// atomic_word must be 8 bytes aligned if you want to use it with atomic_* ops.
	typedef int atomic_word;

	/// atomic_word2 must be 8 bytes aligned if you want to use it with atomic_* ops.
	union atomic_word2
	{
#	if defined(_MSC_VER)
		__int64 v;
#	else
		long long v;
#	endif
#	if !defined(__SSE2__)
		double d;
#	endif
		struct
		{
			atomic_word lo, hi;
		};
	};
	#define ATOMIC_HAS_DCAS

#elif defined (__arm64__) && (defined(__clang__) || defined(__GNUC__))

	typedef long long atomic_word;
	struct atomic_word2
	{
		atomic_word	lo;
		atomic_word	hi;
	};
#	define ATOMIC_HAS_DCAS
#	define ATOMIC_HAS_LDR

#elif defined(_M_ARM) || (defined (__arm__) && (defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)) && (!UNITY_BLACKBERRY) && (!UNITY_STV_API) && (!UNITY_TIZEN) && (defined(__clang__) || defined(__GNUC__)))

	typedef int atomic_word;
	union atomic_word2
	{
#	if defined(_MSC_VER)
		__int64 v;
#	else
		long long v;
#	endif
		struct
		{
			atomic_word	lo;
			atomic_word	hi;
		};
	};
#	define ATOMIC_HAS_DCAS
#	if !defined(_MSC_VER)
#		define ATOMIC_HAS_LDR
#	endif

#elif UNITY_WINRT && defined(__arm__)

	typedef __int32	atomic_word;
	union atomic_word2
	{
		__int64 v;
		struct
		{
			atomic_word lo;
			atomic_word hi;
		};
	};
#	define ATOMIC_HAS_DCAS

#elif UNITY_PS3

	typedef int atomic_word;
	struct atomic_word2
	{
		atomic_word	lo;
		atomic_word	hi;
	};
	//#define ATOMIC_HAS_DCAS
#	define ATOMIC_HAS_LDR

#elif UNITY_XENON
	typedef int atomic_word;
	union atomic_word2
	{
		__int64 v;
		struct
		{
			atomic_word	lo;
			atomic_word	hi;
		};
	};
	//#define ATOMIC_HAS_DCAS

#elif UNITY_PSP2 || (UNITY_WEBGL && SUPPORT_THREADS)

	#include <stdint.h>
	typedef int32_t atomic_word;
	union atomic_word2
	{
		int64_t  v;
		struct
		{
			atomic_word	lo;
			atomic_word	hi;
		};
	};
#	define ATOMIC_HAS_DCAS
#	define ATOMIC_HAS_LDR

#elif defined (__ppc64__) || defined (_ARCH_PPC64)

	typedef long atomic_word;
#	define ATOMIC_HAS_LDR

#elif defined (__ppc__)

	typedef int atomic_word;
#	define ATOMIC_HAS_LDR

#else

#	if defined(__LP64__)
	typedef long long atomic_word;
#	else
	typedef int atomic_word;
#	endif

	struct atomic_word2
	{
		atomic_word	lo;
		atomic_word	hi;
	};

#endif

#if defined(ATOMIC_HAS_DCAS)

	#define ATOMIC_HAS_QUEUE	2

#elif defined (__arm64__) && (defined(__clang__) || defined(__GNUC__))

	#define ATOMIC_HAS_QUEUE	1

#elif defined (__arm__) && (defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)) && (!UNITY_BLACKBERRY) && (!UNITY_STV_API) && (!UNITY_TIZEN) && (defined (__clang__) || defined (__GNUC__)|| defined (SN_TARGET_PSP2))

	#define ATOMIC_HAS_QUEUE	1

#elif UNITY_PS3

	//	The PS3 is managed explicitly here as although it defines _ARCH_PPC64 (but not __ppc__) and is a 64 bit PPC
	//	it only uses 32-bit pointers so cannot wholly take the _ARCH_PPC64 path and cannot (yet) take the incomplete __ppc__ path
	//	In particular, a custom PS3 implementation of AtomicList::Clear has been provided which may have general __ppc__ utility once progressed
	#define	ATOMIC_HAS_QUEUE	1

//#elif (defined (__ppc64__) || defined (_ARCH_PPC64)) && (defined (__clang__) || defined (__GNUC__))

//	#define ATOMIC_HAS_QUEUE	1

//#elif defined (__ppc__) && (defined (__clang__) || defined (__GNUC__))
//
//	#define ATOMIC_HAS_QUEUE	1
//
#endif
