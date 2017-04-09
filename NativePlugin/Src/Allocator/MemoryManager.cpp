#include "UnityPrefix.h"
#include "Runtime/Allocator/MemoryManager.h"
#include "Runtime/Allocator/AllocationHeader.h"
#include "Runtime/Profiler/MemoryProfiler.h"
#include "Runtime/Utilities/MemoryUtilities.h"
#include "Runtime/Utilities/Argv.h"
#include "Runtime/Threads/AtomicOps.h"

#include <limits>

#if UNITY_LINUX || UNITY_TIZEN || UNITY_STV
#include <malloc.h> // memalign
#endif

// under new clang -fpermissive is no longer available, and it is quite strict about proper signatures of global new/delete
// eg throw() is reserved for nonthrow only
#if !defined(STRICTCPP_NEW_DELETE_SIGNATURES)
#define STRICTCPP_NEW_DELETE_SIGNATURES UNITY_OSX || UNITY_STV || (UNITY_LINUX && ENABLE_UNIT_TESTS)
#endif // STRICTCPP_NEW_DELETE_SIGNATURES

#if STRICTCPP_NEW_DELETE_SIGNATURES
	#define THROWING_NEW_THROW throw(std::bad_alloc)
#else
	#define THROWING_NEW_THROW throw()
#endif

#define ENABLE_DISALLOW_ALLOCATIONS_ON_THREAD_CHECK (DEBUGMODE || UNITY_EDITOR)

#define STOMP_MEMORY_ON_ALLOC   (!UNITY_RELEASE && (USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER))
#define STOMP_MEMORY_ON_DEALLOC (!UNITY_RELEASE && (USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER))

// This is disabled by default because it makes everything insanely slow, but is still quite useful for debugging purposes.
//#define STOMP_TEMP_MEMORY_ON_DEALLOC (!UNITY_RELEASE && (USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER))
#define STOMP_TEMP_MEMORY_ON_DEALLOC 0

// Controls override of all allocators for all labels (including temp labels) to DebugAllocator.
// The feature is available only if PLATFORM_SUPPORTS_DEBUG_ALLOCATOR is 1.
#define USE_DEBUG_ALLOCATOR 0
// Protection mode of DebugAllocator for buffer overrun issues (see DebugAllocator::ProtectionMode):
// 0: No protection
// 1: Write protection (allow out of bound reads)
// 2: Full protection
// Deleted memory is always fully protected!
#define DEBUG_ALLOCATOR_BUFFER_OVERRUN_PROTECTION_MODE DebugAllocator::kReadWriteProtection


#if UNITY_XENON

#include "PlatformDependent/Xbox360/Source/XenonMemory.h"

#define UNITY_LL_ALLOC(l, s, a) xenon::trackedMalloc(s, a)
#define UNITY_LL_REALLOC(l, p, s, a) xenon::trackedRealloc(p, s, a)
#define UNITY_LL_FREE(l, p) xenon::trackedFree(p)

#elif UNITY_PS4

void* operator new (size_t size, size_t align) { return GetMemoryManager().Allocate (size == 0 ? 4 : size, align, kMemNewDelete); }
void* operator new [] (size_t size, size_t align) { return GetMemoryManager().Allocate (size == 0 ? 4 : size, align, kMemNewDelete); }

#include "PlatformDependent/PS4/Source/Allocator/PS4Memory.h"

#define UNITY_LL_ALLOC(l,s,a) ::memalign(a, s)
#define UNITY_LL_REALLOC(l,p,s, a) ::reallocalign(p, s, a)
#define UNITY_LL_FREE(l,p) ::free(p)

#elif UNITY_PS3

void* operator new (size_t size, size_t align) throw() { return GetMemoryManager().Allocate (size == 0 ? 4 : size, align, kMemNewDelete); }
void* operator new [] (size_t size, size_t align) throw() { return GetMemoryManager().Allocate (size == 0 ? 4 : size, align, kMemNewDelete); }

#include "Allocator/PS3Memory.h"

#define UNITY_LL_ALLOC(l,s,a) PS3Memory::Alloc(l,s,a)
#define UNITY_LL_REALLOC(l,p,s, a) PS3Memory::Realloc(l,p,s,a)
#define UNITY_LL_FREE(l,p) PS3Memory::Free(l, p)

#elif UNITY_PSP2

void* operator new (size_t size, size_t align) throw() { return GetMemoryManager().Allocate (size == 0 ? 4 : size, align, kMemNewDelete); }
void* operator new [] (size_t size, size_t align) throw() { return GetMemoryManager().Allocate (size == 0 ? 4 : size, align, kMemNewDelete); }

#include "PlatformDependent/PSP2Player/Source/Allocator/PSP2Memory.h"

#define UNITY_LL_ALLOC(l,s,a) PSP2_LL_ALLOC(l,s,a)
#define UNITY_LL_REALLOC(l,p,s, a) PSP2_LL_REALLOC(l,p,s,a)
#define UNITY_LL_FREE(l,p) PSP2_LL_FREE(l, p)

#elif UNITY_ANDROID || UNITY_LINUX || UNITY_TIZEN || UNITY_STV

#define UNITY_LL_ALLOC(l,s,a) ::memalign(a, s)
#define UNITY_LL_REALLOC(l,p,s,a) ::realloc(p, s)
#define UNITY_LL_FREE(l,p) ::free(p)

#elif UNITY_WIN

#define UNITY_LL_ALLOC(l,s,a) ::_aligned_malloc(s, a)
#define UNITY_LL_REALLOC(l,p,s,a) ::_aligned_realloc(p, s, a)
#define UNITY_LL_FREE(l,p) ::_aligned_free(p)

#if UNITY_XBOXONE

#include "PlatformDependent/XboxOne/Source/Allocator/XboxOneGpuAllocator.h"

#endif

#elif UNITY_USE_PLATFORM_MEMORY			// define UNITY_USE_PLATFORM_MEMORY in PlatformPrefixConfigure.h

#include "Allocator/PlatformMemory.h"

#else

#define UNITY_LL_ALLOC(l,s,a) ::malloc(s)
#define UNITY_LL_REALLOC(l,p,s,a) ::realloc(p, s)
#define UNITY_LL_FREE(l,p) ::free(p)

#endif

const size_t kMaxAllocatorOverhead = 64 * 1024;
const char OverflowInMemoryAllocatorString[] = "Overflow in memory allocator.";

void* operator new (size_t size, MemLabelRef label, int align, const char* areaName, const char* objectName, const char* file, int line)
{
	void* p = malloc_internal (size, align, label, kAllocateOptionNone, file, line);
#if ENABLE_MEM_PROFILER
	BaseAllocator* alloc = GetMemoryManager().GetAllocator(label);
	GetMemoryProfiler()->RegisterRootAllocation(p, size, alloc, areaName, objectName);
	push_allocation_root(p, alloc, true);
#endif
	return p;
}

void* operator new (size_t size, MemLabelRef label, int align, const char* file, int line)
{
	void* p = malloc_internal (size, align, label, kAllocateOptionNone, file, line);
	return p;
}

#if UNITY_PS3		// ps3 new can pass in 2 parameters on the front ... size and alignment
void* operator new (size_t size, size_t alignment, MemLabelRef label, int align, const char* areaName, const char* objectName, const char* file, int line)
{
	void* p = malloc_internal (size, align, label, kAllocateOptionNone, file, line);
#if ENABLE_MEM_PROFILER
	BaseAllocator* alloc = GetMemoryManager().GetAllocator(label);
	GetMemoryProfiler()->RegisterRootAllocation(p, size, alloc, areaName, objectName);
	push_allocation_root(p, alloc, true);
#endif
	return p;
}

void* operator new (size_t size, size_t alignment, MemLabelRef label, int align, const char* file, int line)
{
	void* p = malloc_internal (size, align, label, kAllocateOptionNone, file, line);
	return p;
}
#endif

void operator delete (void* p, MemLabelRef label, int /*align*/, const char* /*areaName*/, const char* /*objectName*/, const char* /*file*/, int /*line*/) { free_alloc_internal (p, label); }
void operator delete (void* p, MemLabelRef label, int /*align*/, const char* /*file*/, int /*line*/) { free_alloc_internal (p, label); }

#if ENABLE_MEMORY_MANAGER
#include "Runtime/Allocator/DebugAllocator.h"
#include "Runtime/Allocator/DynamicHeapAllocator.h"
#include "Runtime/Allocator/DualThreadAllocator.h"
#include "Runtime/Allocator/LowLevelDefaultAllocator.h"
#include "Runtime/Allocator/TLSAllocator.h"
#include "Runtime/Allocator/StackAllocator.h"
#include "Runtime/Allocator/BucketAllocator.h"
#include "Runtime/Allocator/ThreadsafeLinearAllocator.h"
#include "Runtime/Allocator/UnityDefaultAllocator.h"
#include "Runtime/Threads/Thread.h"
#include "Runtime/Threads/ThreadSpecificValue.h"
#include "Runtime/Utilities/Word.h"

#if UNITY_IPHONE || UNITY_TVOS
	#include "PlatformDependent/iPhonePlayer/iPhoneNewLabelAllocator.h"
#endif

#include <map>

typedef DualThreadAllocator< DynamicHeapAllocator< LowLevelAllocator > > MainThreadAllocator;
typedef TLSAllocator< StackAllocator > TempTLSAllocator;

static MemoryManager* g_MemoryManager = NULL;

// new override does not work on mac together with pace
#ifndef UNITY_ALLOC_ALLOW_NEWDELETE_OVERRIDE
#define UNITY_ALLOC_ALLOW_NEWDELETE_OVERRIDE !((UNITY_OSX || UNITY_LINUX) && UNITY_EDITOR) && !(UNITY_WEBGL) && !(UNITY_TIZEN)
#endif

#if UNITY_ALLOC_ALLOW_NEWDELETE_OVERRIDE
void* operator new (size_t size) THROWING_NEW_THROW { return GetMemoryManager().Allocate (size==0?4:size, kDefaultMemoryAlignment, kMemNewDelete, kAllocateOptionNone, "Overloaded New"); }
void* operator new [] (size_t size) THROWING_NEW_THROW { return GetMemoryManager().Allocate (size==0?4:size, kDefaultMemoryAlignment, kMemNewDelete, kAllocateOptionNone, "Overloaded New[]"); }
void operator delete (void* p) throw() { GetMemoryManager().Deallocate (p, kMemNewDelete); }
void operator delete [] (void* p) throw() { GetMemoryManager().Deallocate (p, kMemNewDelete); }

void* operator new (size_t size, const std::nothrow_t&) throw() { return GetMemoryManager().Allocate (size, kDefaultMemoryAlignment, kMemNewDelete, kAllocateOptionNone, "Overloaded New"); }
void* operator new [] (size_t size, const std::nothrow_t&) throw() { return GetMemoryManager().Allocate (size, kDefaultMemoryAlignment, kMemNewDelete, kAllocateOptionNone, "Overloaded New[]"); };
void operator delete (void* p, const std::nothrow_t&) throw() { GetMemoryManager().Deallocate (p, kMemNewDelete); }
void operator delete [] (void* p, const std::nothrow_t&) throw() { GetMemoryManager().Deallocate (p, kMemNewDelete); }
#endif

#if UNITY_EDITOR
static const size_t kDynamicHeapChunkSize = 16*1024*1024;
static const size_t kTempAllocatorMainSize = 16*1024*1024;
#else
static const size_t kDynamicHeapChunkSize = 4*1024*1024;
static const size_t kTempAllocatorMainSize = 1*1024*1024;
#endif

#if ENABLE_MEMORY_MANAGER

void PrintShortMemoryStats(core::string& str, MemLabelRef label);

static int AlignUp(int value, int alignment)
{
	UInt32 ptr = value;
	UInt32 bitMask = (alignment - 1);
	UInt32 lowBits = ptr & bitMask;
	UInt32 adjust = ((alignment - lowBits) & bitMask);
	return adjust;
}

void* GetPreallocatedMemory(int size)
{
	const size_t numAllocators = 20; // should be configured per platform
	const size_t additionalStaticMem = sizeof(UnityDefaultAllocator<void>) * numAllocators;

	// preallocated memory for memorymanager and allocators

	// Any use of HEAP_NEW must be accounted for in this calculation
	static const int preallocatedSize = sizeof(MemoryManager) + sizeof(TempTLSAllocator) + sizeof(BucketAllocator) + additionalStaticMem;
#if UNITY_PS3
	static char __attribute__((aligned(16))) g_MemoryBlockForMemoryManager[preallocatedSize];
#elif UNITY_XENON
	static char __declspec(align(16)) g_MemoryBlockForMemoryManager[preallocatedSize];
#else
	static char ALIGN_TYPE(16) g_MemoryBlockForMemoryManager[preallocatedSize];
#endif
	static char* g_MemoryBlockPtr = g_MemoryBlockForMemoryManager;

	size += AlignUp(size, 16);

	void* ptr = g_MemoryBlockPtr;
	g_MemoryBlockPtr+=size;
	// Ensure that there is enough space on the preallocated block
	if(g_MemoryBlockPtr > g_MemoryBlockForMemoryManager + preallocatedSize)
	{
		int* i = 0;
		*i = 10;
		// Use of HEAP_NEW does not line up with preallocatedSize. Count uses of HEAP_NEW
		// on this platform and account for it above.
		return NULL;
	}
	return ptr;
}
#endif

#if UNITY_WIN && !UNITY_UAP && ENABLE_MEM_PROFILER && !defined(UNITY_WIN_API_SUBSET)
#define _CRTBLD
#include <..\crt\src\dbgint.h>
_CRT_ALLOC_HOOK pfnOldCrtAllocHook;
int catchMemoryAllocHook(int allocType, void *userData, size_t size, int blockType, long requestNumber, const unsigned char	*filename, int lineNumber);
#endif

void* malloc_internal(size_t size, size_t align, MemLabelRef label, int allocateOptions, const char* file, int line)
{
	return GetMemoryManager ().Allocate(size, align, label, allocateOptions, file, line);
}

void* calloc_internal(size_t count, size_t size, int align, MemLabelRef label, int allocateOptions, const char* file, int line)
{
	void* ptr = GetMemoryManager ().Allocate(size*count, align, label, allocateOptions, file, line);
	if (ptr) memset (ptr, 0, size*count);
	return ptr;
}

void* realloc_internal(void* ptr, size_t size, int align, MemLabelRef label, int allocateOptions, const char* file, int line)
{
	return GetMemoryManager ().Reallocate(ptr, size, align, label, allocateOptions, file, line);
}

void free_internal(void* ptr)
{
	// used for mac, since malloc is not hooked from the start, so a number of mallocs will have passed through
	// before we wrap. This results in pointers being freed, that were not allocated with the memorymanager
	// therefore we need the alloc->Contains check()
	GetMemoryManager().Deallocate (ptr);
}

void free_alloc_internal(void* ptr, MemLabelRef label)
{
	GetMemoryManager().Deallocate (ptr, label);
}

#if (UNITY_OSX && UNITY_EDITOR)

#include <malloc/malloc.h>
#include <mach/vm_map.h>
void *(*systemMalloc)(malloc_zone_t *zone, size_t size);
void *(*systemCalloc)(malloc_zone_t *zone, size_t num_items, size_t size);
void *(*systemValloc)(malloc_zone_t *zone, size_t size);
void *(*systemRealloc)(malloc_zone_t *zone, void* ptr, size_t size);
void *(*systemMemalign)(malloc_zone_t *zone, size_t align, size_t size);
void  (*systemFree)(malloc_zone_t *zone, void *ptr);
void  (*systemFreeSize)(malloc_zone_t *zone, void *ptr, size_t size);

void* my_malloc(malloc_zone_t *zone, size_t size)
{
	void* ptr = (*systemMalloc)(zone,size);
	AtomicAdd(&MemoryManager::m_LowLevelAllocated, (*malloc_default_zone()->size)(zone,ptr));
	return ptr;

}

void* my_calloc(malloc_zone_t *zone, size_t num_items, size_t size)
{
	void* ptr = (*systemCalloc)(zone,num_items,size);
	AtomicAdd(&MemoryManager::m_LowLevelAllocated, (*malloc_default_zone()->size)(zone,ptr));
	return ptr;

}

void* my_valloc(malloc_zone_t *zone, size_t size)
{
	void* ptr = (*systemValloc)(zone,size);
	AtomicAdd(&MemoryManager::m_LowLevelAllocated, (*malloc_default_zone()->size)(zone,ptr));
	return ptr;
}

void* my_realloc(malloc_zone_t *zone, void* ptr, size_t size)
{
	AtomicSub(&MemoryManager::m_LowLevelAllocated, (*malloc_default_zone()->size)(zone,ptr));
	void* newptr = (*systemRealloc)(zone,ptr,size);
	AtomicAdd(&MemoryManager::m_LowLevelAllocated, (*malloc_default_zone()->size)(zone,newptr));
	return newptr;
}

void* my_memalign(malloc_zone_t *zone, size_t align, size_t size)
{
	void* ptr = (*systemMemalign)(zone,align,size);
	AtomicAdd(&MemoryManager::m_LowLevelAllocated, (*malloc_default_zone()->size)(zone,ptr));
	return ptr;
}

void my_free(malloc_zone_t *zone, void *ptr)
{
	AtomicSub(&MemoryManager::m_LowLevelAllocated, (*malloc_default_zone()->size)(zone,ptr));
	systemFree(zone,ptr);
}

void my_free_definite_size(malloc_zone_t *zone, void *ptr, size_t size)
{
	AtomicSub(&MemoryManager::m_LowLevelAllocated, (*malloc_default_zone()->size)(zone,ptr));
	systemFreeSize(zone,ptr,size);
}

#endif

void InitializeMemory()
{
#if (UNITY_OSX && UNITY_EDITOR)

	UInt32 osxversion = 0;
	Gestalt(gestaltSystemVersion, (MacSInt32 *) &osxversion);

	// overriding malloc_zone on osx 10.5 causes unity not to start up.
	if(osxversion >= 0x01060)
	{
		malloc_zone_t* dz = malloc_default_zone();

		systemMalloc = dz->malloc;
		systemCalloc = dz->calloc;
		systemValloc = dz->valloc;
		systemRealloc = dz->realloc;
		systemMemalign = dz->memalign;
		systemFree = dz->free;
		systemFreeSize = dz->free_definite_size;

		if(dz->version>=8)
			vm_protect(mach_task_self(), (uintptr_t)dz, sizeof(malloc_zone_t), 0, VM_PROT_READ | VM_PROT_WRITE);//remove the write protection

		dz->malloc=&my_malloc;
		dz->calloc=&my_calloc;
		dz->valloc=&my_valloc;
		dz->realloc=&my_realloc;
		dz->memalign=&my_memalign;
		dz->free=&my_free;
		dz->free_definite_size=&my_free_definite_size;

		if(dz->version>=8)
			vm_protect(mach_task_self(), (uintptr_t)dz, sizeof(malloc_zone_t), 0, VM_PROT_READ);//put the write protection back

	}
#endif
}

MemoryManager& GetMemoryManager()
{
	if (g_MemoryManager == NULL){
		InitializeMemory();
		g_MemoryManager = HEAP_NEW(MemoryManager)();
	}
	return *g_MemoryManager ;
}

volatile size_t MemoryManager::m_LowLevelAllocated = 0;
volatile size_t MemoryManager::m_RegisteredGfxDriverMemory = 0;

#if	ENABLE_MEMORY_MANAGER

#if ENABLE_DISALLOW_ALLOCATIONS_ON_THREAD_CHECK
UNITY_TLS_VALUE(bool) s_DisallowAllocationsOnThread;
inline void MemoryManager::CheckDisalowAllocation()
{
	DebugAssert(IsActive());

	// Some codepaths in Unity disallow allocations. For example when a GC is running all threads are stopped.
	// If we allowed allocations we would allow for very hard to find race conditions.
	// Thus we explicitly check for it in debug builds.
	if (s_DisallowAllocationsOnThread)
	{
		s_DisallowAllocationsOnThread = false;
		FatalErrorMsg("CheckDisalowAllocation. Allocating memory when it is not allowed to allocate memory.\n");
	}
}

#else
inline void MemoryManager::CheckDisalowAllocation()
{
}
#endif

// Verifies ptr correctness before reallocation/deallocation
static void VerifyPtrIntegrity(BaseAllocator* alloc, const void* ptr)
{
#if USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER
	AssertMsg(alloc->ValidateIntegrity(ptr), "Invalid memory pointer was detected!");
#endif
}

//-----------------------------------------------------------------------------

MemoryManager::MemoryManager()
: m_NumAllocators(0)
, m_IsInitialized(false)
, m_IsActive(false)
, m_IsInitializedDebugAllocator(false)
, m_FrameTempAllocator(NULL)
, m_BucketAllocator(NULL)
, m_InitialFallbackTempAllocationsCount(0)
{
#if UNITY_WIN && ENABLE_MEM_PROFILER
//	pfnOldCrtAllocHook	= _CrtSetAllocHook(catchMemoryAllocHook);
#endif

	memset (m_Allocators, 0, sizeof(m_Allocators));
	memset (m_MainAllocators, 0, sizeof(m_MainAllocators));
	memset (m_ThreadAllocators, 0, sizeof(m_ThreadAllocators));
	memset (m_AllocatorMap, 0, sizeof(m_AllocatorMap));
	
	// Main thread will not have a valid TLSAlloc until ThreadInitialize() is called!
	m_InitialFallbackAllocator = HEAP_NEW(DynamicHeapAllocator<LowLevelAllocator>) (1024*1024, 0, true, NULL, "ALLOC_FALLBACK");

	m_NextFreeCustomAllocatorIndex = 0;
	for(intptr_t i = 0; i < kMaxCustomAllocators; i++)
		m_CustomAllocators[i] = (BaseAllocator*)(i+1);

	for (int i = 0; i < kMemLabelCount; i++)
		m_AllocatorMap[i].alloc = m_InitialFallbackAllocator;
	m_PS3DelayedReleaseIndex = -1;
}

void MemoryManager::StaticInitialize()
{
	GetMemoryManager().ThreadInitialize(kTempAllocatorMainSize);
}

void MemoryManager::StaticDestroy()
{
#if !UNITY_OSX && !UNITY_PS3 // PS3 PRX shutdown crashes - Static destructors as we remap new/delete (STL containers in PRX map through into these)
	// not able to destroy profiler and memorymanager on osx because apple.coreaudio is still running a thread.
	// FMOD is looking into shutting this down properly. Until then, don't cleanup
#if ENABLE_MEM_PROFILER
	MemoryProfiler::StaticDestroy();
#endif
	GetMemoryManager().ThreadCleanup();
#endif
}

void MemoryManager::InitializeMainThreadAllocators()
{
	// We MUST not have any allocations labeled by kMemTempAlloc at the initialization point
	Assert(m_InitialFallbackTempAllocationsCount == 0);

	// Check if we force DebugAllocator initialization
	if (USE_DEBUG_ALLOCATOR || HasARGV("debugallocator"))
	{
		m_IsInitializedDebugAllocator = InitializeDebugAllocator();
		AssertMsg(m_IsInitializedDebugAllocator, "Unable to initialize DebugAllocator. Guarded allocations are disabled!");
	}

	// Use default allocators overwise
	if (!m_IsInitializedDebugAllocator)
		InitializeDefaultAllocators();

	// Always create TempTLSAllocator to avoid branching in release players
	m_AllocatorMap[kMemTempAllocId].alloc = m_FrameTempAllocator = HEAP_NEW(TempTLSAllocator)("ALLOC_TEMP_THREAD");
	m_Allocators[m_NumAllocators++] = m_FrameTempAllocator;
	
	m_IsInitialized = true;
	m_IsActive = true;

#if ENABLE_MEM_PROFILER
	MemoryProfiler::StaticInitialize();
#endif
}

void MemoryManager::InitializeDefaultAllocators()
{
	BucketAllocator* bucketAllocator = NULL;
#if USE_BUCKET_ALLOCATOR
	static const int kBucketAllocatorGranularity = 16;
	static const int kBucketAllocatorBucketsCount = 8;
#if UNITY_EDITOR
	static const int kBucketAllocatorLargeBlockSize = 32 * 1024 * 1024;
	static const int kBucketAllocatorLargeBlocksCount = 4;
#elif UNITY_PS3
	static const int kBucketAllocatorLargeBlockSize = 1 * 1024 * 1024;  // Optimal against OS kernel allocs
	static const int kBucketAllocatorLargeBlocksCount = 16;             // Realistically will typically not rise above 8 before system memory runs out
#else
#if ENABLE_MEM_PROFILER
	static const int kBucketAllocatorLargeBlockSize = 16 * 1024 * 1024;
#else
	static const int kBucketAllocatorLargeBlockSize = 4 * 1024 * 1024;
#endif
	static const int kBucketAllocatorLargeBlocksCount = 1;
#endif
	m_BucketAllocator = bucketAllocator = HEAP_NEW(BucketAllocator)("ALLOC_BUCKET",
		kBucketAllocatorGranularity, kBucketAllocatorBucketsCount, kBucketAllocatorLargeBlockSize, kBucketAllocatorLargeBlocksCount);
#endif // USE_BUCKET_ALLOCATOR

#if (UNITY_WIN && !UNITY_WP_8_1 && (!UNITY_UAP || !defined(__arm__))) || UNITY_OSX || UNITY_PS4 || TARGET_IPHONE_SIMULATOR || UNITY_LINUX
	BaseAllocator* defaultThreadAllocator = NULL;
	m_MainAllocators[m_NumAllocators] = HEAP_NEW(DynamicHeapAllocator<LowLevelAllocator>) (kDynamicHeapChunkSize, 1024, false, NULL, "ALLOC_DEFAULT_MAIN");
	m_ThreadAllocators[m_NumAllocators] = HEAP_NEW(DynamicHeapAllocator<LowLevelAllocator>) (1024 * 1024, 1024, true, NULL, "ALLOC_DEFAULT_THREAD");
	BaseAllocator* defaultAllocator = m_Allocators[m_NumAllocators] = HEAP_NEW(MainThreadAllocator)("ALLOC_DEFAULT", bucketAllocator, m_MainAllocators[m_NumAllocators], m_ThreadAllocators[m_NumAllocators]);
	defaultThreadAllocator = m_ThreadAllocators[m_NumAllocators];
	m_NumAllocators++;
#elif (UNITY_PS3)
	BaseAllocator* defaultAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(DynamicHeapAllocator<LowLevelAllocator>)(1 * 1024 * 1024, 256, true, bucketAllocator, "ALLOC_DEFAULT");
#elif (UNITY_XENON)
	BaseAllocator* defaultAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(DynamicHeapAllocator<LowLevelAllocator>)(8 * 1024 * 1024, 1024, true, bucketAllocator, "ALLOC_DEFAULT");
#else
	BaseAllocator* defaultAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(UnityDefaultAllocator<LowLevelAllocator>)("ALLOC_DEFAULT");
#endif

	for (int i = 0; i < kMemLabelCount; i++)
		m_AllocatorMap[i].alloc = defaultAllocator;

#if (UNITY_WIN || UNITY_PS4 || UNITY_XBOXONE || UNITY_LINUX || UNITY_OSX)
	m_AllocatorMap[kMemTempJobAllocId].alloc = m_Allocators[m_NumAllocators++] = HEAP_NEW(ThreadsafeLinearAllocator) (1 * 1024 * 1024, 64, "ALLOC_TEMP_JOB");
#else
	m_AllocatorMap[kMemTempJobAllocId].alloc = m_Allocators[m_NumAllocators++] = HEAP_NEW(ThreadsafeLinearAllocator) (256 * 1024, 64, "ALLOC_TEMP_JOB");
#endif

#if UNITY_IPHONE || UNITY_TVOS
	m_AllocatorMap[kMemNewDeleteId].alloc = m_Allocators[m_NumAllocators++] = HEAP_NEW(IphoneNewLabelAllocator);
#endif

#if (UNITY_WIN && !UNITY_WP_8_1 && (!UNITY_UAP || !defined(__arm__))) || UNITY_OSX || TARGET_IPHONE_SIMULATOR || UNITY_LINUX
	m_MainAllocators[m_NumAllocators] = HEAP_NEW(DynamicHeapAllocator<LowLevelAllocator>) (kDynamicHeapChunkSize, 0, false, NULL, "ALLOC_GFX_MAIN");
	m_ThreadAllocators[m_NumAllocators] = HEAP_NEW(DynamicHeapAllocator<LowLevelAllocator>) (1024 * 1024, 0, true, NULL, "ALLOC_GFX_THREAD");
	BaseAllocator* gfxAllocator = m_Allocators[m_NumAllocators] = HEAP_NEW(MainThreadAllocator)("ALLOC_GFX", bucketAllocator, m_MainAllocators[m_NumAllocators], m_ThreadAllocators[m_NumAllocators]);
	BaseAllocator* gfxThreadAllocator = m_ThreadAllocators[m_NumAllocators];
	m_NumAllocators++;

	m_MainAllocators[m_NumAllocators] = HEAP_NEW(DynamicHeapAllocator<LowLevelAllocator>) (8 * 1024 * 1024, 0, false, NULL, "ALLOC_CACHEOBJECTS_MAIN");
	m_ThreadAllocators[m_NumAllocators] = HEAP_NEW(DynamicHeapAllocator<LowLevelAllocator>) (2 * 1024 * 1024, 0, true, NULL, "ALLOC_CACHEOBJECTS_THREAD");
	BaseAllocator* cacheAllocator = m_Allocators[m_NumAllocators] = HEAP_NEW(MainThreadAllocator)("ALLOC_CACHEOBJECTS", bucketAllocator, m_MainAllocators[m_NumAllocators], m_ThreadAllocators[m_NumAllocators]);
	m_NumAllocators++;

	m_MainAllocators[m_NumAllocators] = HEAP_NEW(DynamicHeapAllocator<LowLevelAllocator>) (kDynamicHeapChunkSize, 0, false, NULL, "ALLOC_TYPETREE_MAIN");
	m_ThreadAllocators[m_NumAllocators] = HEAP_NEW(DynamicHeapAllocator<LowLevelAllocator>) (1024 * 1024, 0, true, NULL, "ALLOC_TYPETREE_THREAD");
	BaseAllocator* typetreeAllocator = m_Allocators[m_NumAllocators] = HEAP_NEW(MainThreadAllocator)("ALLOC_TYPETREE", bucketAllocator, m_MainAllocators[m_NumAllocators], m_ThreadAllocators[m_NumAllocators]);
	m_NumAllocators++;

	m_MainAllocators[m_NumAllocators] = HEAP_NEW(DynamicHeapAllocator<LowLevelAllocator>) (4 * 1024 * 1024, 0, false, NULL, "ALLOC_PROFILER_MAIN");
	m_ThreadAllocators[m_NumAllocators] = HEAP_NEW(DynamicHeapAllocator<LowLevelAllocator>) (4 * 1024 * 1024, 0, true, NULL, "ALLOC_PROFILER_THREAD");
	BaseAllocator* profilerAllocator = m_Allocators[m_NumAllocators] = HEAP_NEW(MainThreadAllocator)("ALLOC_PROFILER", bucketAllocator, m_MainAllocators[m_NumAllocators], m_ThreadAllocators[m_NumAllocators]);
	m_NumAllocators++;

	m_AllocatorMap[kMemDynamicGeometryId].alloc
		= m_AllocatorMap[kMemImmediateGeometryId].alloc
		= m_AllocatorMap[kMemGeometryId].alloc
		= m_AllocatorMap[kMemVertexDataId].alloc
		= m_AllocatorMap[kMemBatchedGeometryId].alloc
		= m_AllocatorMap[kMemTextureId].alloc = gfxAllocator;

	m_AllocatorMap[kMemTypeTreeId].alloc = typetreeAllocator;

	m_AllocatorMap[kMemThreadId].alloc = defaultThreadAllocator;
	m_AllocatorMap[kMemGfxThreadId].alloc = gfxThreadAllocator;

	m_AllocatorMap[kMemTextureCacheId].alloc = cacheAllocator;
	m_AllocatorMap[kMemSerializationId].alloc = cacheAllocator;
	m_AllocatorMap[kMemFileId].alloc = cacheAllocator;

	m_AllocatorMap[kMemProfilerId].alloc = profilerAllocator;
	m_AllocatorMap[kMemMemoryProfilerId].alloc = profilerAllocator;
	m_AllocatorMap[kMemMemoryProfilerStringId].alloc = profilerAllocator;

#if UNITY_XBOXONE

#if XBOXONE_D3D_FAST_SEMANTICS
	BaseAllocator* gpuAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(XboxOneGpuAllocator)(8*1024*1024, 0, false, NULL, "ALLOC_GPU");
	m_AllocatorMap[kMemXboxOneGpuMemoryId].alloc = gpuAllocator;
#endif

#endif

#elif UNITY_XENON

#if 1
	// DynamicHeapAllocator uses TLSF pools which are O(1) constant time for alloc/free
	// It should be used for high alloc/dealloc traffic
	BaseAllocator* dynAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(DynamicHeapAllocator<LowLevelAllocator>)(32 * 1024 * 1024, 0, true, bucketAllocator, "ALLOC_TINYBLOCKS");
	m_AllocatorMap[kMemBaseObjectId].alloc = dynAllocator;
	m_AllocatorMap[kMemAnimationId].alloc = dynAllocator;
	m_AllocatorMap[kMemSTLId].alloc = dynAllocator;
	m_AllocatorMap[kMemNewDeleteId].alloc = dynAllocator;
#else
	BaseAllocator* gameObjectAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(UnityDefaultAllocator<LowLevelAllocator>)("ALLOC_GAMEOBJECT");
	BaseAllocator* gfxAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(UnityDefaultAllocator<LowLevelAllocator>)("ALLOC_GFX");

	BaseAllocator* profilerAllocator;
#if XBOX_USE_DEBUG_MEMORY
	if (xenon::GetIsDebugMemoryEnabled())
		profilerAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(DynamicHeapAllocator<LowLevelAllocatorDebugMem>)(16 * 1024 * 1024, 0, true, NULL, "ALLOC_PROFILER");
	else
#endif
		profilerAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(UnityDefaultAllocator<LowLevelAllocator>)("ALLOC_PROFILER");

	m_AllocatorMap[kMemDynamicGeometryId].alloc
		= m_AllocatorMap[kMemImmediateGeometryId].alloc
		= m_AllocatorMap[kMemGeometryId].alloc
		= m_AllocatorMap[kMemVertexDataId].alloc
		= m_AllocatorMap[kMemBatchedGeometryId].alloc
		= m_AllocatorMap[kMemTextureId].alloc = gfxAllocator;

	m_AllocatorMap[kMemBaseObjectId].alloc = gameObjectAllocator;

	m_AllocatorMap[kMemProfilerId].alloc = profilerAllocator;
	m_AllocatorMap[kMemMemoryProfilerId].alloc = profilerAllocator;
	m_AllocatorMap[kMemMemoryProfilerStringId].alloc = profilerAllocator;
#endif

#elif UNITY_PS4

	BaseAllocator* profilerAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(DynamicHeapAllocator<LowLevelAllocator>)(32 * 1024 * 1024, 0, true, bucketAllocator, "ALLOC_PROFILER");
	size_t gfxmemsize = PS4GetGarlicHeapSize();
	BaseAllocator* gfxAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(PS4UnityAllocator)("ALLOC_GFX", gfxmemsize, PLATFORM_GFXALLOCATOR_MEMORYTYPE, PLATFORM_GFXALLOCATOR_MEMORYMODE);

	m_AllocatorMap[kMemGPUMemoryId].alloc = gfxAllocator;
	m_AllocatorMap[kMemPS4ShaderUcodeId].alloc = gfxAllocator;

	m_AllocatorMap[kMemProfilerId].alloc =
		m_AllocatorMap[kMemMemoryProfilerId].alloc =
		m_AllocatorMap[kMemMemoryProfilerStringId].alloc = profilerAllocator;

#elif UNITY_PS3

	BaseAllocator* delayedReleaseAllocator = m_Allocators[m_PS3DelayedReleaseIndex = m_NumAllocators++] = HEAP_NEW(PS3DelayedReleaseAllocator("PS3_DELAYED_RELEASE_ALLOCATOR_PROXY", defaultAllocator));
	//	These three labels require a delayed release proxy allocator to ensure we don't immediately free them on calls to ::deallocate. This would cause issues if the gfx hware was still using them
	m_AllocatorMap[kMemSkinningId].alloc = m_AllocatorMap[kMemVertexDataId].alloc = m_AllocatorMap[kMemPS3DelayedReleaseId].alloc = delayedReleaseAllocator;

#elif UNITY_PSP2

	BaseAllocator* gameObjectAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(UnityDefaultAllocator<LowLevelAllocator>)("ALLOC_GAMEOBJECT");
	BaseAllocator* gfxAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(UnityDefaultAllocator<LowLevelAllocator>)("ALLOC_GFX");
	BaseAllocator* profilerAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(UnityDefaultAllocator<LowLevelAllocator>)("ALLOC_PROFILER");
	BaseAllocator* vertexDataAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(PSP2DelayedReleaseAllocator("PSP2_LPDDR2_UC_DELAYED_RELEASE_ALLOCATOR_PROXY", PLATFORM_VERTEXALLOCATOR_UNCACHED_MEMORYTYPE));
	BaseAllocator* dynamicVertexDataAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(PSP2DelayedReleaseAllocator("PSP2_LPDDR2_DELAYED_RELEASE_ALLOCATOR_PROXY", PLATFORM_VERTEXALLOCATOR_CACHED_MEMORYTYPE));
	BaseAllocator* vertexDataLocalVidAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(PSP2DelayedReleaseAllocator("PSP2_CDRAM_DELAYED_RELEASE_ALLOCATOR_PROXY", PLATFORM_VERTEXALLOCATOR_VRAM_MEMORYTYPE));

	m_AllocatorMap[kMemDynamicGeometryId].alloc
		= m_AllocatorMap[kMemImmediateGeometryId].alloc
		= m_AllocatorMap[kMemGeometryId].alloc
		= m_AllocatorMap[kMemBatchedGeometryId].alloc
		//		= m_AllocatorMap[kMemSkinningTempId].alloc
		= m_AllocatorMap[kMemSkinningId].alloc
		= m_AllocatorMap[kMemTextureId].alloc = gfxAllocator;

	m_AllocatorMap[kMemBaseObjectId].alloc = gameObjectAllocator;

	m_AllocatorMap[kMemProfilerId].alloc
		= m_AllocatorMap[kMemMemoryProfilerId].alloc
		= m_AllocatorMap[kMemMemoryProfilerStringId].alloc = profilerAllocator;

	// CPU accessible mesh data goes in mapped LPPDR2 cached memory.
	m_AllocatorMap[kMemVertexDataId].alloc = dynamicVertexDataAllocator;

	// GXM data all goes to mapped VRAM if possible, or mapped LPDDR2 un-cached memory if VRAM is full.
	m_AllocatorMap[kMemPSP2GXMBuffersId].alloc = vertexDataLocalVidAllocator;
	m_AllocatorMap[kMemPSP2GXMVertexDataId].alloc = vertexDataLocalVidAllocator;

	// Set fall-back allocator, if VRAM is full redirects to LPDDR2 un-cached memory.
	((PSP2DelayedReleaseAllocator*) vertexDataLocalVidAllocator)->SetFallback((PSP2DelayedReleaseAllocator*) vertexDataAllocator);
	((PSP2DelayedReleaseAllocator*) vertexDataLocalVidAllocator)->SetMaxKernelMem(0);

#else

	BaseAllocator* gameObjectAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(UnityDefaultAllocator<LowLevelAllocator>)("ALLOC_GAMEOBJECT");
	BaseAllocator* gfxAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(UnityDefaultAllocator<LowLevelAllocator>)("ALLOC_GFX");
	BaseAllocator* profilerAllocator = m_Allocators[m_NumAllocators++] = HEAP_NEW(UnityDefaultAllocator<LowLevelAllocator>)("ALLOC_PROFILER");

	m_AllocatorMap[kMemDynamicGeometryId].alloc
		= m_AllocatorMap[kMemImmediateGeometryId].alloc
		= m_AllocatorMap[kMemGeometryId].alloc
		= m_AllocatorMap[kMemVertexDataId].alloc
		= m_AllocatorMap[kMemBatchedGeometryId].alloc
		= m_AllocatorMap[kMemTextureId].alloc = gfxAllocator;

	m_AllocatorMap[kMemBaseObjectId].alloc = gameObjectAllocator;

	m_AllocatorMap[kMemProfilerId].alloc
		= m_AllocatorMap[kMemMemoryProfilerId].alloc
		= m_AllocatorMap[kMemMemoryProfilerStringId].alloc = profilerAllocator;

#endif
}
bool MemoryManager::InitializeDebugAllocator()
{
#if !PLATFORM_SUPPORTS_DEBUG_ALLOCATOR
	return false;
#else
	// Override all allocators with DebugAllocator, even temp ones!
	DebugAllocator* debugAllocator = HEAP_NEW(DebugAllocator)("ALLOC_DEBUG");
	DebugAllocator::ProtectionMode protectionMode = DEBUG_ALLOCATOR_BUFFER_OVERRUN_PROTECTION_MODE;
	std::string protectionModeStr = GetFirstValueForARGV("debugallocator");
	if (!protectionModeStr.empty())
	{
		switch (protectionModeStr[0])
		{
		case '0': protectionMode = DebugAllocator::kNoProtection; break;
		case '1': protectionMode = DebugAllocator::kWriteProtection; break;
		case '2': protectionMode = DebugAllocator::kReadWriteProtection; break;
		default: break;
		}
	}
	debugAllocator->SetBufferOverrunProtectionMode(protectionMode);
	
	m_Allocators[m_NumAllocators++] = debugAllocator;
	for (int i = kMemTempLabels; i < kMemLabelCount; i++)
		m_AllocatorMap[i].alloc = debugAllocator;

	return true;
#endif
}

MemoryManager::~MemoryManager()
{
	for(int i = 0; i < m_NumAllocators; i++)
	{
		Assert(m_Allocators[i]->GetAllocatedMemorySize() == 0);
	}

	ThreadCleanup();

	for (int i = 0; i < m_NumAllocators; i++)
	{
		HEAP_DELETE (m_Allocators[i], BaseAllocator);
	}
	HEAP_DELETE (m_BucketAllocator, BaseAllocator);

#if UNITY_WIN
#if ENABLE_MEM_PROFILER
//	_CrtSetAllocHook(pfnOldCrtAllocHook);
#endif
#endif
}

class MemoryManagerAutoDestructor
{
public:
	MemoryManagerAutoDestructor(){}
	~MemoryManagerAutoDestructor()
	{
		//HEAP_DELETE(g_MemoryManager, MemoryManager);
	}
};

MemoryManagerAutoDestructor g_MemoryManagerAutoDestructor;


#else


MemoryManager::MemoryManager()
: m_NumAllocators(0)
, m_FrameTempAllocator(NULL)
{
}

MemoryManager::~MemoryManager()
{
}

#endif

#if UNITY_WIN
#include <winnt.h>
#define ON_WIN(x) x
#else
#define ON_WIN(x)
#endif

#if ENABLE_MEMORY_MANAGER

void* MemoryManager::LowLevelAllocate(size_t size)
{
#if UNITY_PS3
	int num1MBPages = PS3Memory::GetNum1MBOSPages(size);
	if (num1MBPages > 0)
		return PS3Memory::AllocateAndMap1MBPages(num1MBPages);
#endif

	void* ptr = UNITY_LL_ALLOC(kMemDefault, size, kDefaultMemoryAlignment);
	if (ptr != NULL)
		ON_WIN(AtomicAdd(&m_LowLevelAllocated, size));

	return ptr;
}

void* MemoryManager::LowLevelCAllocate(size_t count, size_t size)
{
	void* ptr = NULL;

	if (DoesMultiplicationOverflow(size, count))
	{
		FatalErrorMsg(OverflowInMemoryAllocatorString);
		return NULL;
	}

	const size_t allocSize = count*size;

#if UNITY_PS3
	int num1MBPages = PS3Memory::GetNum1MBOSPages(allocSize);
	if (num1MBPages > 0)
		ptr = PS3Memory::AllocateAndMap1MBPages(num1MBPages);
	else
#endif
		ptr = UNITY_LL_ALLOC(kMemDefault, allocSize, kDefaultMemoryAlignment);

	if (ptr != NULL)
	{
		ON_WIN(AtomicAdd(&m_LowLevelAllocated, allocSize));
		memset(ptr, 0, allocSize);
	}

	return ptr;
}

void* MemoryManager::LowLevelReallocate(void* p, size_t size, size_t oldSize)
{
	void* newPtr;
#if UNITY_PS3
	int oldNum1MBPages = PS3Memory::GetNum1MBOSPages(oldSize);
	if (oldNum1MBPages > 0)
	{
		newPtr = LowLevelAllocate(size);
		if (newPtr)
			UNITY_MEMCPY(newPtr, p, std::min<size_t>(size, oldSize));
		PS3Memory::FreeAndUnmap1MBPages(p);
	}
	else
#endif
	newPtr = UNITY_LL_REALLOC(kMemDefault, p, size, kDefaultMemoryAlignment);
	if (newPtr != NULL)
	{
		ON_WIN(AtomicSub(&m_LowLevelAllocated, oldSize));
		ON_WIN(AtomicAdd(&m_LowLevelAllocated, size));
	}

	return newPtr;
}

void MemoryManager::LowLevelFree(void* p, size_t oldSize)
{
	if (p == NULL)
		return;

#if UNITY_PS3
	int oldNum1MBPages = PS3Memory::GetNum1MBOSPages(oldSize);
	if (oldNum1MBPages > 0)
		PS3Memory::FreeAndUnmap1MBPages(p);
	else
#endif
		UNITY_LL_FREE(kMemDefault, p);

	ON_WIN(AtomicSub(&m_LowLevelAllocated, oldSize));
}

void MemoryManager::ThreadInitialize(size_t tempSize)
{
	const bool isMainThread = Thread::CurrentThreadIsMainThread();
	if (isMainThread && !m_IsInitialized)
		InitializeMainThreadAllocators();
	
	if (!m_IsInitializedDebugAllocator)
	{
		// On a main thread we allow kMemTempJobAlloc allocations as they must be short-lived.
		// Other threads might live for longer and steal space from a renderer.
		const MemLabelId fallbackLabel = isMainThread ? kMemTempJobAlloc : kMemTempOverflow;
		StackAllocator* tempAllocator = UNITY_NEW(StackAllocator(tempSize, fallbackLabel, "ALLOC_TEMP_THREAD"), kMemManager);
		m_FrameTempAllocator->ThreadInitialize(tempAllocator);
	}
}

void MemoryManager::ThreadCleanup()
{
	for(int i = 0; i < m_NumAllocators; i++)
		m_Allocators[i]->ThreadCleanup();

	if (Thread::CurrentThreadIsMainThread())
	{
		m_FrameTempAllocator = NULL;

		m_IsActive = false;

#if !UNITY_EDITOR
		// Destroy the allocators in reverse order in case any depend on the default allocator internally.
		for(int i = m_NumAllocators-1; i >= 0; i--)
		{
			HEAP_DELETE(m_Allocators[i], BaseAllocator);
			if(m_MainAllocators[i])
				HEAP_DELETE(m_MainAllocators[i], BaseAllocator);
			if(m_ThreadAllocators[i])
				HEAP_DELETE(m_ThreadAllocators[i], BaseAllocator);
			m_Allocators[i] = 0;
			m_MainAllocators[i] = 0;
			m_ThreadAllocators[i] = 0;
		}
		m_NumAllocators = 0;
		if (m_BucketAllocator != NULL)
		{
			HEAP_DELETE (m_BucketAllocator, BaseAllocator);
			m_BucketAllocator = NULL;
		}
#else
		for(int i = 0; i < m_NumAllocators; i++)
		{
			if(m_MainAllocators[i])
				m_Allocators[i] = m_MainAllocators[i];
		}
#endif
		for (int i = 0; i < kMemLabelCount; i++)
			m_AllocatorMap[i].alloc = m_InitialFallbackAllocator;

		return;
	}
#if ENABLE_MEM_PROFILER
	GetMemoryProfiler()->ThreadCleanup();
#endif
	if (m_BucketAllocator != NULL)
		m_BucketAllocator->ThreadCleanup ();
}


void MemoryManager::SetTempAllocatorSize( size_t tempAllocatorSize )
{
	((TempTLSAllocator*) m_FrameTempAllocator)->GetCurrentAllocator()->SetBlockSize(tempAllocatorSize);
}


void MemoryManager::FrameMaintenance(bool cleanup)
{
	for(int i = 0; i < m_NumAllocators; i++)
		m_Allocators[i]->FrameMaintenance(cleanup);
}

void MemoryManager::ThreadTempAllocFrameMaintenance()
{
	m_FrameTempAllocator->FrameMaintenance(false);
}

#else

void* MemoryManager::LowLevelAllocate(int size)
{
	return UNITY_LL_ALLOC(kMemDefault, size, 4);
}

void* MemoryManager::LowLevelCAllocate(int count, int size)
{
	void* ptr = UNITY_LL_ALLOC(kMemDefault, count*size, 4);
	memset(ptr,0,count*size);
	return ptr;
}

void* MemoryManager::LowLevelReallocate(void* p, int size, size_t oldSize)
{
	return UNITY_LL_REALLOC(kMemDefault, p, size, 4);
}

void  MemoryManager::LowLevelFree(void* p, size_t oldSize)
{
	UNITY_LL_FREE(kMemDefault, p);
}

void MemoryManager::ThreadInitialize(size_t tempSize)
{}

void MemoryManager::ThreadCleanup()
{}

#endif

void OutOfMemoryError( size_t size, int align, MemLabelRef label, int line, const char* file )
{
	core::string str(kMemTempAlloc);
	str.reserve(30*1024);
	str += FormatString("Could not allocate memory: System out of memory!\n");
	str += FormatString("Trying to allocate: %" PRINTF_SIZET_FORMAT "B with %d alignment. MemoryLabel: %s\n", size, align, GetMemoryManager().GetMemcatName(label));
	str += FormatString("Allocation happend at: Line:%d in %s\n", line, file);
	PrintShortMemoryStats(str, label);
	// first call the plain printf_console, to make a printout that doesn't do callstack and other allocations
	printf_console("%s", str.c_str());
	// Then do a FatalErroString, that brings up a dialog and launches the bugreporter.
	FatalErrorString(str.c_str());
}

inline void* CheckAllocation(void* ptr, size_t size, int align, MemLabelRef label, const char* file, int line)
{
	if(ptr == NULL)
		OutOfMemoryError(size, align, label, line, file);
	return ptr;
}

void MemoryManager::DisallowAllocationsOnThisThread()
{
#if ENABLE_DISALLOW_ALLOCATIONS_ON_THREAD_CHECK
	s_DisallowAllocationsOnThread = true;
#endif
}
void MemoryManager::ReallowAllocationsOnThisThread()
{
#if ENABLE_DISALLOW_ALLOCATIONS_ON_THREAD_CHECK
	s_DisallowAllocationsOnThread = false;
#endif
}

void* MemoryManager::Allocate(size_t size, int align, MemLabelRef label, int allocateOptions/* = kNone*/, const char* file /* = NULL */, int line /* = 0 */)
{
	DebugAssert(IsPowerOfTwo(align));
	DebugAssert(align != 0);
	DebugAssert(GetLabelIdentifier(label) != GetLabelIdentifier(kMemInvalidAlloc));
	DebugAssert(GetLabelIdentifier(label) != kMemTempLabels);

	// Return unique pointer even for 0 size allocations. That pointer should be deallocated later.
	if (size == 0)
		size = 1;

	align = MaxAlignment(align, kDefaultMemoryAlignment);

	if (DoesAdditionOverflow(size, align + kMaxAllocatorOverhead))
	{
		if ((allocateOptions & kAllocateOptionReturnNullIfOutOfMemory) != kAllocateOptionReturnNullIfOutOfMemory)
		{
			// FatalErrorMsg effectively aborts the program execution. Skip this statement if the allocate option is
			// kAllocateOptionReturnNullIfOutOfMemory, as this means that the caller can recover from a failed allocation.
			FatalErrorMsg(OverflowInMemoryAllocatorString);
		}
		else
		{
			WarningStringMsg(OverflowInMemoryAllocatorString);
		}
		return NULL;
	}

	// Fallback to backup allocator if we have not yet initialized the MemoryManager
	if (!IsActive())
	{
		//TODO: Ideally we should not allocate any "temp" memory before MemoryManager is initialized
		//AssertMsg(!IsTempAllocatorLabel(label), "No allocation should happen on kMemTempAlloc label before MemoryManager is initialized!");
		if (IsTempLabel(label))
			m_InitialFallbackTempAllocationsCount++;

		void* ptr = m_InitialFallbackAllocator->Allocate(size, align);
		return ptr;
	}

	if (IsTempAllocatorLabel(label))
	{
		void* ptr = ((TempTLSAllocator*) m_FrameTempAllocator)->TempTLSAllocator::Allocate(size, align);
		if (ptr != NULL)
			return ptr;

		// If tempallocator thread has not been initialized fallback to kMemTempOverflow label
		return Allocate(size, align, kMemTempOverflow, allocateOptions, file, line);
	}

	BaseAllocator* alloc = GetAllocator(label);
	CheckDisalowAllocation();

	void* ptr = alloc->Allocate(size, align);
	if (ptr == NULL && (allocateOptions & kAllocateOptionReturnNullIfOutOfMemory))
		return NULL;

	CheckAllocation( ptr, size, align, label, file, line );
	DebugAssert(((UIntPtr)ptr & (align-1)) == 0);

	// Register allocation in the profiler
	if (!IsTempLabel(label))
	{
#if ENABLE_MEM_PROFILER
		RegisterAllocation(ptr, size, label, "Allocate", file, line);
#endif
#if STOMP_MEMORY_ON_ALLOC
		memset(ptr, 0xcd, size);
#endif
	}

	return ptr;
}

void* MemoryManager::Reallocate(void* ptr, size_t size, int align, MemLabelRef label, int allocateOptions/* = kNone*/, const char* file /* = NULL */, int line /* = 0 */)
{
	DebugAssert(IsPowerOfTwo(align));
	DebugAssert(align != 0);
	DebugAssert(GetLabelIdentifier(label) != GetLabelIdentifier(kMemInvalidAlloc));

	if (ptr == NULL)
		return Allocate(size,align,label,allocateOptions,file,line);

	if (size == 0)
	{
		Deallocate(ptr, label);
		return NULL;
	}

	align = MaxAlignment(align, kDefaultMemoryAlignment);

	if (DoesAdditionOverflow(size, align + kMaxAllocatorOverhead))
	{
		if ((allocateOptions & kAllocateOptionReturnNullIfOutOfMemory) != kAllocateOptionReturnNullIfOutOfMemory)
		{
			// FatalErrorMsg effectively aborts the program execution. Skip this statement if the allocate option is
			// kAllocateOptionReturnNullIfOutOfMemory, as this means that the caller can recover from a failed allocation.
			FatalErrorMsg(OverflowInMemoryAllocatorString);
		}
		else
		{
			WarningStringMsg(OverflowInMemoryAllocatorString);
		}
		return NULL;
	}

	// Fallback to backup allocator if we have not yet initialized the MemoryManager
	if (!IsActive())
	{
		//TODO: Ideally we should not allocate any "temp" memory before MemoryManager is initialized
		//AssertMsg(!IsTempAllocatorLabel(label), "No allocation should happen on kMemTempAlloc label before MemoryManager is initialized!");
		return m_InitialFallbackAllocator->Reallocate(ptr, size, align);
	}

	if (IsTempLabel(label))
	{
		void* newptr = NULL;
		if (IsTempAllocatorLabel(label))
			newptr = ((TempTLSAllocator*) m_FrameTempAllocator)->TempTLSAllocator::Reallocate(ptr, size, align);
		else
			newptr = GetAllocator(label)->Reallocate(ptr, size, align);

		if (newptr)
			return newptr;

		// if tempallocator thread has not been initialized fallback to kMemDefault
		return Reallocate(ptr, size, align, kMemTempOverflow, allocateOptions, file, line);
	}

	BaseAllocator* alloc = GetAllocator(label);
	CheckDisalowAllocation();

	if (!alloc->Contains(ptr))
	{
		// It wasn't the expected allocator that contained the pointer.
		// allocate on the expected allocator and move the memory there
		void* newptr = Allocate(size, align, label, allocateOptions, file, line);
		if ((allocateOptions & kAllocateOptionReturnNullIfOutOfMemory) && !newptr)
			return NULL;

		size_t oldSize = GetAllocatorContainingPtr(ptr)->GetPtrSize(ptr);
		UNITY_MEMCPY(newptr, ptr, std::min<size_t>(size, oldSize));

		Deallocate(ptr);
		return newptr;
	}

	VerifyPtrIntegrity(alloc, ptr);

#if ENABLE_MEM_PROFILER
	// register the deletion of the old allocation and extract the old root owner
	AllocationRootReference* root = NULL;
	if (GetMemoryManager().GetAllocator(label)->GetProfilerHeader(ptr))	// make sure the allocator supports profiler
	{
		root = GET_ROOT_REFERENCE(ptr, label);
		if (root)
			root->Retain();
	}
	RegisterDeallocation(ptr, label, "Reallocate");
#endif

	void* newptr = alloc->Reallocate(ptr, size, align);
	if ((allocateOptions & kAllocateOptionReturnNullIfOutOfMemory) && !newptr)
		return NULL;

	CheckAllocation(newptr, size, align, label, file, line);
	DebugAssert(((UIntPtr) newptr & (align - 1)) == 0);

#if ENABLE_MEM_PROFILER
	RegisterAllocation(newptr, size, CreateMemLabel(GetLabelIdentifier(label), root), "Reallocate", file, line);
	if(root)
		root->Release();
#endif

	return newptr;
}

void MemoryManager::Deallocate(void* ptr, MemLabelRef label)
{
	DebugAssert(GetLabelIdentifier(label) != GetLabelIdentifier(kMemInvalidAlloc));

	if (ptr == NULL)
		return;

	if (!IsActive())
	{
		// if we are outside the scope of Initialize/Destroy - fallback
		if (IsTempLabel(label))
			m_InitialFallbackTempAllocationsCount--;

		return Deallocate(ptr);
	}

	if (IsTempLabel(label))
	{
#if STOMP_TEMP_MEMORY_ON_DEALLOC
		BaseAllocator* tempAlloc = GetAllocator(label);
		if (tempAlloc->Contains(ptr))
			memset32(ptr, 0xdeadbeef, tempAlloc->GetRequestedPtrSize(ptr));
#endif

		if (IsTempAllocatorLabel(label))
		{
			// If not found, Fallback to kMemDefault allocator has the pointer
			if (!((TempTLSAllocator*) m_FrameTempAllocator)->TempTLSAllocator::TryDeallocate(ptr))
				Deallocate(ptr, kMemTempOverflow);
		}
		else
		{
#if !STOMP_TEMP_MEMORY_ON_DEALLOC
			BaseAllocator* tempAlloc = GetAllocator(label);
#endif
			tempAlloc->Deallocate(ptr);
		}

		return;
	}

	BaseAllocator* alloc = GetAllocator(label);
	CheckDisalowAllocation();

#if ENABLE_MEM_PROFILER

	// We need RegisterDeallocation happens before deallocation.
	// That means we can't use TryDeallocate because ptr might be from low-level malloc (OSX)
	if (!alloc->Contains(ptr))
	{
		Deallocate(ptr);
		return;
	}

	VerifyPtrIntegrity(alloc, ptr);

	RegisterDeallocation(ptr, label, "Deallocate");

#if STOMP_MEMORY_ON_DEALLOC
	if(!alloc->IsDelayedRelease())
	{
		memset32(ptr, 0xdeadbeef, alloc->GetRequestedPtrSize(ptr));
	}
#endif
	alloc->Deallocate(ptr);

#else // ENABLE_MEM_PROFILER

	// Without profiler we can go fast path by default
	if (!alloc->TryDeallocate(ptr))
		Deallocate(ptr);

#endif // ENABLE_MEM_PROFILER
}

void MemoryManager::Deallocate(void* ptr)
{
	if (ptr == NULL)
		return;

	BaseAllocator* alloc = GetAllocatorContainingPtr(ptr);
	if (alloc)
	{
		Assert(alloc != m_FrameTempAllocator);
		VerifyPtrIntegrity(alloc, ptr);
#if ENABLE_MEM_PROFILER
		if(GetMemoryProfiler() && alloc != m_InitialFallbackAllocator)
		{
			size_t oldsize = alloc->GetRequestedPtrSize(ptr);
			GetMemoryProfiler()->UnregisterAllocation(ptr, oldsize, kMemDefault);
			//RegisterDeallocation(MemLabelId(labelid), oldsize); // since we don't have the label, we will not register this deallocation
			if(m_LogAllocations)
				printf_console("Deallocate (%p): %11d\tTotal: %.2fMB (%" PRINTF_SIZET_FORMAT ")\n", ptr, -(int)oldsize, GetTotalAllocatedMemory() / (1024.0f * 1024.0f), GetTotalAllocatedMemory());
		}
#endif
#if STOMP_MEMORY_ON_DEALLOC
		if(!alloc->IsDelayedRelease())
		{
			memset32(ptr, 0xdeadbeef, alloc->GetRequestedPtrSize(ptr));
		}
#endif
		alloc->Deallocate(ptr);
	}
	else
	{
		if(IsActive())
			UNITY_LL_FREE (kMemDefault, ptr);
		//else ignore the deallocation, because the allocator is no longer around
	}
}

struct ExternalAllocInfo
{
	size_t size;
	size_t relatedID;
	const char* file;
	int line;
};

#if ENABLE_MEM_PROFILER

typedef UNITY_MAP(kMemMemoryProfiler,void*,ExternalAllocInfo ) ExternalAllocationMap;
static ExternalAllocationMap* g_ExternalAllocations = NULL;
Mutex g_ExternalAllocationLock;

void register_external_gfx_allocation(void* ptr, size_t size, size_t related, const char* file, int line)
{
	Mutex::AutoLock autolock(g_ExternalAllocationLock);
	if (g_ExternalAllocations == NULL)
	{
		SET_ALLOC_OWNER(NULL);
		g_ExternalAllocations = new ExternalAllocationMap();
	}
	ExternalAllocationMap::iterator it = g_ExternalAllocations->find(ptr);
	if (it != g_ExternalAllocations->end())
	{
		ErrorStringMsg("allocation 0x%p already registered @ %s:l%d size %d; now calling from %s:l%d size %d?",
			ptr,
			it->second.file, it->second.line, (int)it->second.size,
			file, line, (int)size);
	}

	if (related == 0)
		related = (size_t)ptr;

	ExternalAllocInfo info;
	info.size = size;
	info.relatedID = related;
	info.file = file;
	info.line = line;
	g_ExternalAllocations->insert(std::make_pair(ptr, info));
	MemoryManager::m_RegisteredGfxDriverMemory += size;

	GetMemoryProfiler()->RegisterMemoryToID(related, size);
}

void register_external_gfx_deallocation(void* ptr, const char* file, int line)
{
	if(ptr == NULL)
		return;
	Mutex::AutoLock autolock(g_ExternalAllocationLock);
	if (g_ExternalAllocations == NULL)
		g_ExternalAllocations = new ExternalAllocationMap();

	ExternalAllocationMap::iterator it = g_ExternalAllocations->find(ptr);
	if (it == g_ExternalAllocations->end())
	{
		// not registered
		return;
	}
	size_t size = it->second.size;
	size_t related = it->second.relatedID;
	MemoryManager::m_RegisteredGfxDriverMemory -= size;
	g_ExternalAllocations->erase(it);
	GetMemoryProfiler()->UnregisterMemoryToID(related,size);

	if (g_ExternalAllocations->empty())
	{
		delete g_ExternalAllocations;
		g_ExternalAllocations = NULL;
	}
}

#endif


BaseAllocator* MemoryManager::GetAllocatorContainingPtr(const void* ptr)
{
	for(int i = 0; i < m_NumAllocators ; i++)
	{
#if (UNITY_PS3 && ONLY_DLMALLOC)
		if (i == m_PS3DelayedReleaseIndex)
		{
			continue;
		}
#endif
		if(m_Allocators[i])	// Can be NULL when shutting down.
		{
			if(m_Allocators[i]->IsAssigned() && m_Allocators[i]->Contains(ptr))
				return m_Allocators[i];
		}
	}

	if(m_InitialFallbackAllocator->Contains(ptr))
		return m_InitialFallbackAllocator;

#if (UNITY_PS3 && ONLY_DLMALLOC)
	if( m_IsActive )
	{
		BaseAllocator* delayedReleaseAlloc  = (m_PS3DelayedReleaseIndex != -1) ? m_Allocators[ m_PS3DelayedReleaseIndex ] : NULL;
		if( delayedReleaseAlloc  && delayedReleaseAlloc ->IsAssigned() && delayedReleaseAlloc ->Contains( ptr ) )
		{
			return delayedReleaseAlloc;
		}
	}
#endif

	{
		Mutex::AutoLock lock(m_CustomAllocatorMutex);
		for (int i = 0; i < kMaxCustomAllocators; i++)
		{
			if (m_CustomAllocators[i] >(void*)(intptr_t)kMaxCustomAllocators)
			{
				if (m_CustomAllocators[i]->Contains(ptr))
					return m_CustomAllocators[i];
			}
		}
	}

	if (m_FrameTempAllocator != NULL && m_FrameTempAllocator->Contains(ptr))
		return m_FrameTempAllocator;

	return NULL;
}

const char* MemoryManager::GetAllocatorName( int i )
{
	return i < m_NumAllocators ? m_Allocators[i]->GetName() : "Custom";
}

const char* MemoryManager::GetMemcatName(MemLabelRef label)
{
	return GetLabelIdentifier(label) < kMemLabelCount ? MemLabelName[GetLabelIdentifier(label)] : "Custom";
}

MemLabelId MemoryManager::AddCustomAllocator(BaseAllocator* allocator)
{
	Mutex::AutoLock lock(m_CustomAllocatorMutex);

	Assert (allocator->Contains(NULL) == false); // to assure that Contains does not just return true
	size_t freeIndex = m_NextFreeCustomAllocatorIndex;
	Assert(freeIndex != kMaxCustomAllocators);
	m_NextFreeCustomAllocatorIndex = reinterpret_cast<size_t>(m_CustomAllocators[freeIndex]);
	
	MemLabelIdentifier identifier = (MemLabelIdentifier)(kMemLabelCount + freeIndex);
	m_CustomAllocators[freeIndex] = allocator;
	return CreateMemLabel(identifier);
}

BaseAllocator* MemoryManager::GetCustomAllocator(MemLabelRef label)
{
	Assert(GetLabelIdentifier(label) >= kMemLabelCount);
	Mutex::AutoLock lock(m_CustomAllocatorMutex);
	size_t index = GetLabelIdentifier(label) - kMemLabelCount;
	return m_CustomAllocators[index];
}

void MemoryManager::RemoveCustomAllocator(MemLabelRef label)
{
	Mutex::AutoLock lock(m_CustomAllocatorMutex);
	Assert(GetLabelIdentifier(label) >= kMemLabelCount);

	size_t index = GetLabelIdentifier(label) - kMemLabelCount;
	m_CustomAllocators[index] = reinterpret_cast<BaseAllocator*>(m_NextFreeCustomAllocatorIndex);
	m_NextFreeCustomAllocatorIndex = index;
}

BaseAllocator* MemoryManager::GetAllocatorAtIndex(int index)
{
	return m_Allocators[index];
}

BaseAllocator* MemoryManager::GetAllocator(MemLabelRef label)
{
	if (GetLabelIdentifier(label) < kMemLabelCount)
	{
		BaseAllocator* alloc = m_AllocatorMap[GetLabelIdentifier(label)].alloc;
		return alloc;
	}
	else
	{
		int index = GetLabelIdentifier(label) - kMemLabelCount;
		DebugAssert(index < kMaxCustomAllocators);
		return m_CustomAllocators[index] <= (void*)kMaxCustomAllocators ? NULL : m_CustomAllocators[index];
	}
}

int MemoryManager::GetAllocatorIndex(BaseAllocator* alloc)
{
	for (int i = 0; i < m_NumAllocators; i++)
		if (alloc == m_Allocators[i])
			return i;

	return m_NumAllocators;
}

size_t MemoryManager::GetTotalAllocatedMemory()
{
	size_t total = 0;
	if (m_BucketAllocator != NULL)
		total += m_BucketAllocator->GetAllocatedMemorySize ();
	for(int i = 0; i < m_NumAllocators ; i++)
		total += m_Allocators[i]->GetAllocatedMemorySize();

	{
		Mutex::AutoLock lock(m_CustomAllocatorMutex);
		for (int i = 0; i < kMaxCustomAllocators; i++)
		{
			if (m_CustomAllocators[i] >(void*)(intptr_t)kMaxCustomAllocators)
				total += m_CustomAllocators[i]->GetAllocatedMemorySize();
		}
	}

	return total;
}

size_t MemoryManager::GetTotalReservedMemory()
{
	size_t total = 0;
	if (m_BucketAllocator != NULL)
		total += m_BucketAllocator->GetReservedMemorySize();
	for (int i = 0; i < m_NumAllocators; i++)
		total += m_Allocators[i]->GetReservedMemorySize();

	{
		Mutex::AutoLock lock(m_CustomAllocatorMutex);
		for (int i = 0; i < kMaxCustomAllocators; i++)
		{
			if (m_CustomAllocators[i] >(void*)(intptr_t)kMaxCustomAllocators)
				total += m_CustomAllocators[i]->GetReservedMemorySize();
		}
	}

	return total;
}

size_t MemoryManager::GetTotalUnusedReservedMemory()
{
	return GetTotalReservedMemory() - GetTotalAllocatedMemory();;
}

int MemoryManager::GetAllocatorCount( )
{
	return m_NumAllocators;
}

size_t MemoryManager::GetAllocatedMemory( MemLabelRef label )
{
#if ENABLE_MEM_PROFILER
	if (IsTempAllocatorLabel(label))
	{
		return m_FrameTempAllocator->GetAllocatedMemorySize();
	}
	return AtomicAdd(&m_AllocatorMap[GetLabelIdentifier(label)].allocatedMemory, 0);
#else
	return 0;
#endif
}

size_t MemoryManager::GetTotalProfilerMemory()
{
	return  GetAllocator(kMemProfiler)->GetAllocatedMemorySize();
}

int MemoryManager::GetAllocCount( MemLabelRef label )
{
#if ENABLE_MEM_PROFILER
	return AtomicAdd(&m_AllocatorMap[GetLabelIdentifier(label)].numAllocs, 0);
#else
	return 0;
#endif
}

size_t MemoryManager::GetLargestAlloc( MemLabelRef label )
{
#if ENABLE_MEM_PROFILER
	return AtomicAdd(&m_AllocatorMap[GetLabelIdentifier(label)].largestAlloc, 0);
#else
	return 0;
#endif
}

void MemoryManager::StartLoggingAllocations(size_t logAllocationsThreshold)
{
	m_LogAllocations = true;
	m_LogAllocationsThreshold = logAllocationsThreshold;
};

void MemoryManager::StopLoggingAllocations()
{
	m_LogAllocations = false;
};

#if ENABLE_MEM_PROFILER
void MemoryManager::RegisterAllocation(void* ptr, size_t size, MemLabelRef label, const char* function, const char* file, int line)
{
	if (GetMemoryProfiler())
	{
		DebugAssert(!IsTempAllocatorLabel(label));
		BaseAllocator* alloc = GetAllocator(label);
		size_t actualSize = alloc->GetRequestedPtrSize(ptr);
		if(GetLabelIdentifier(label) < kMemLabelCount)
		{
			AtomicAdd(&m_AllocatorMap[GetLabelIdentifier(label)].allocatedMemory, actualSize);
			AtomicIncrement(&m_AllocatorMap[GetLabelIdentifier(label)].numAllocs);
			size_t oldLargestAlloc;
			do
			{
				oldLargestAlloc = AtomicAdd(&m_AllocatorMap[GetLabelIdentifier(label)].largestAlloc, 0);
			} while (oldLargestAlloc < size && !AtomicCompareExchange(&m_AllocatorMap[GetLabelIdentifier(label)].largestAlloc, actualSize, oldLargestAlloc));
		}
		GetMemoryProfiler()->RegisterAllocation(ptr, label, file, line, actualSize);
		if (m_LogAllocations && size >= m_LogAllocationsThreshold)
		{
			size_t totalAllocatedMemoryAfterAllocation = GetTotalAllocatedMemory();
			printf_console( "%s (%p): %11" PRINTF_SIZET_FORMAT "\tTotal: %.2fMB (%" PRINTF_SIZET_FORMAT ") in %s:%d\n",
				function, ptr, size, totalAllocatedMemoryAfterAllocation / (1024.0f * 1024.0f), totalAllocatedMemoryAfterAllocation, file, line
				);
		}
	}
}

void MemoryManager::RegisterDeallocation(void* ptr, MemLabelRef label, const char* function)
{
	if (ptr != NULL && GetMemoryProfiler())
	{
		DebugAssert(!IsTempAllocatorLabel(label));
		BaseAllocator* alloc = GetAllocator(label);
		size_t oldsize = alloc->GetRequestedPtrSize(ptr);
		GetMemoryProfiler()->UnregisterAllocation(ptr, oldsize, label);
		if(GetLabelIdentifier(label) < kMemLabelCount)
		{
			AtomicSub(&m_AllocatorMap[GetLabelIdentifier(label)].allocatedMemory, oldsize);
			AtomicDecrement(&m_AllocatorMap[GetLabelIdentifier(label)].numAllocs);
		}

		if (m_LogAllocations && oldsize >= m_LogAllocationsThreshold)
		{
			size_t totalAllocatedMemoryAfterDeallocation = GetTotalAllocatedMemory() - oldsize;
			printf_console( "%s (%p): %11d\tTotal: %.2fMB (%" PRINTF_SIZET_FORMAT ")\n",
				function, ptr, -(int)oldsize, totalAllocatedMemoryAfterDeallocation / (1024.0f * 1024.0f), totalAllocatedMemoryAfterDeallocation);
		}
	}
}
#endif

void PrintShortMemoryStats(core::string& str, MemLabelRef label)
{
#if ENABLE_MEMORY_MANAGER
	MemoryManager& mm = GetMemoryManager();
	str += "Memory overview\n\n";
	for(int i = 0; i < mm.GetAllocatorCount() ; i++)
	{
		BaseAllocator* alloc = mm.GetAllocatorAtIndex(i);
		if(alloc == NULL)
			continue;
		str += FormatString( "[ %s ] used: %" PRINTF_SIZET_FORMAT "B | peak: %" PRINTF_SIZET_FORMAT "B | reserved: %" PRINTF_SIZET_FORMAT "B \n",
									alloc->GetName(), alloc->GetAllocatedMemorySize(), alloc->GetPeakAllocatedMemorySize(), alloc->GetReservedMemorySize());
	}
#endif
}

#if UNITY_WIN && !UNITY_UAP && ENABLE_MEM_PROFILER && !defined(UNITY_WIN_API_SUBSET)

// this is only here to catch stray mallocs - UNITY_MALLOC should be used instead
// use tls to mark if we are in our own malloc or not. register stray mallocs.
int catchMemoryAllocHook(int allocType, void *userData, size_t size, int blockType, long requestNumber, const unsigned char *filename, int lineNumber)
{
	if(!GetMemoryProfiler())
		return TRUE;

	if (allocType == _HOOK_FREE)
	{
#ifdef _DEBUG
		int headersize = sizeof(_CrtMemBlockHeader);
		_CrtMemBlockHeader* header = (_CrtMemBlockHeader*)((size_t) userData - headersize);
		GetMemoryProfiler()->UnregisterAllocation(NULL, header->nDataSize, kMemDefault);
#endif
	}
	else
		GetMemoryProfiler()->RegisterAllocation(NULL, CreateMemLabel(kMemLabelCount), NULL, 0, size);
	return TRUE;
}

#endif


#else // !ENABLE_MEMORY_MANAGER

void* malloc_internal(size_t size, size_t align, MemLabelRef label, int allocateOptions, const char* file, int line)
{
	return UNITY_LL_ALLOC(label, size, align);
}

void* calloc_internal(size_t count, size_t size, int align, MemLabelRef label, int allocateOptions, const char* file, int line)
{
	void* ptr = UNITY_LL_ALLOC(label, size * count, align);
	memset (ptr, 0, size * count);
	return ptr;
}

void* realloc_internal(void* ptr, size_t size, int align, MemLabelRef label, int allocateOptions, const char* file, int line)
{
	return UNITY_LL_REALLOC(label, ptr, size, align);
}

void free_alloc_internal(void* ptr, MemLabelRef label)
{
	UNITY_LL_FREE(label, ptr);
}

#if !((UNITY_OSX || UNITY_LINUX) && UNITY_EDITOR)
void* operator new (size_t size) THROWING_NEW_THROW { return UNITY_LL_ALLOC (kMemNewDelete, size, kDefaultMemoryAlignment); }
void* operator new [] (size_t size) THROWING_NEW_THROW { return UNITY_LL_ALLOC (kMemNewDelete, size, kDefaultMemoryAlignment); }
void operator delete (void* p) throw() { UNITY_LL_FREE (kMemNewDelete, p); } // can't make allocator assumption, since ptr can be newed by other operator new
void operator delete [] (void* p) throw() {UNITY_LL_FREE (kMemNewDelete, p); }

void* operator new (size_t size, const std::nothrow_t&) throw() { return UNITY_LL_ALLOC (kMemNewDelete, size, kDefaultMemoryAlignment); }
void* operator new [] (size_t size, const std::nothrow_t&) throw() { return UNITY_LL_ALLOC (kMemNewDelete, size, kDefaultMemoryAlignment); };
void operator delete (void* p, const std::nothrow_t&) throw() { UNITY_LL_FREE (kMemNewDelete, p); }
void operator delete [] (void* p, const std::nothrow_t&) throw() { UNITY_LL_FREE (kMemNewDelete, p); }
#endif

#endif
