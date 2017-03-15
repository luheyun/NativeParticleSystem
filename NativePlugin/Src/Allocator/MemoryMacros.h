#ifndef _MEMORY_MACROS_H_
#define _MEMORY_MACROS_H_

#include <limits>
#include <new>

#if defined(__ARMCC_VERSION)
#include <alloca.h>		// used in ALLOC_TEMP_ALIGNED
#endif

#if defined(__GNUC__) || defined(__SNC__) || defined(__clang__) || defined(__ghs__)
	#define ALIGN_OF(T) __alignof__(T)
	#define ALIGN_TYPE(val) __attribute__((aligned(val)))
	#define FORCE_INLINE inline __attribute__ ((always_inline))
#elif defined(_MSC_VER)
	#define ALIGN_OF(T) __alignof(T)
	#define ALIGN_TYPE(val) __declspec(align(val))
	#define FORCE_INLINE __forceinline
#elif defined(__ARMCC_VERSION)
	#define ALIGN_OF(T) __alignof__(T)
	#define ALIGN_TYPE(val) __attribute__((aligned(val)))  // ARMCC supports GNU extension
	#define FORCE_INLINE __forceinline
#else
	#define ALIGN_TYPE(size)
	#define FORCE_INLINE inline
#endif


#if ENABLE_MEMORY_MANAGER
//	These methods are added to be able to make some initial allocations that does not use the memory manager
extern void* GetPreallocatedMemory(int size);
#	define HEAP_NEW(cls) new (GetPreallocatedMemory(sizeof(cls))) cls
#	define HEAP_DELETE(obj, cls) obj->~cls();
#else
#	define HEAP_NEW(cls) new (UNITY_LL_ALLOC(kMemDefault,sizeof(cls),kDefaultMemoryAlignment)) cls
#	define HEAP_DELETE(obj, cls) {obj->~cls();UNITY_LL_FREE(kMemDefault,(void*)obj);}
#endif

enum
{
	kDefaultMemoryAlignment = 16
};

enum
{
	kAllocateOptionNone = 0,						// Fatal: Show message box with out of memory error and quit application
	kAllocateOptionReturnNullIfOutOfMemory = 1	// Returns null if allocation fails (doesn't show message box)
};


#if ENABLE_MEMORY_MANAGER

// Use UNITY_PLATFORM_DECLARES_NEWDELETE (in PlatformPrefixConfigure.h) for new platforms where new/delete is declared differently
#if !UNITY_PLATFORM_DECLARES_NEWDELETE

// new override does not work on mac together with pace
#if !(UNITY_OSX && UNITY_EDITOR) && !(UNITY_PLUGIN) && !UNITY_WEBGL && !UNITY_IPHONE && !UNITY_TVOS

#if UNITY_OSX
void* operator new (size_t size) throw(std::bad_alloc);
void* operator new [] (size_t size) throw(std::bad_alloc);
#else
void* operator new (size_t size) throw();
void* operator new [] (size_t size) throw();
#endif

#if UNITY_PS3
void* operator new (size_t size, size_t alignment) throw();
void* operator new [] (size_t size, size_t alignment) throw();
#endif
void operator delete (void* p) throw();
void operator delete [] (void* p) throw();

void* operator new (size_t size, const std::nothrow_t&) throw();
void* operator new [] (size_t size, const std::nothrow_t&) throw();
void operator delete (void* p, const std::nothrow_t&) throw();
void operator delete [] (void* p, const std::nothrow_t&) throw();

#elif UNITY_IPHONE || UNITY_TVOS

void* operator new (size_t size) throw(std::bad_alloc);
void* operator new [] (size_t size) throw(std::bad_alloc);
void operator delete (void* p) throw();
void operator delete [] (void* p) throw();

void* operator new (size_t size, const std::nothrow_t&) throw();
void* operator new [] (size_t size, const std::nothrow_t&) throw();
void operator delete (void* p, const std::nothrow_t&) throw();
void operator delete [] (void* p, const std::nothrow_t&) throw();

#endif //!(UNITY_OSX && UNITY_EDITOR) && !(UNITY_PLUGIN) && !UNITY_WEBGL && !UNITY_IPHONE && !UNITY_TVOS

#endif // !UNITY_PLATFORM_DECLARES_NEWDELETE

#endif

#if ENABLE_MEM_PROFILER
class BaseAllocator;

EXPORT_COREMODULE bool  push_allocation_root(void* root, BaseAllocator* alloc, bool forcePush);
EXPORT_COREMODULE void  pop_allocation_root();
EXPORT_COREMODULE AllocationRootReference* get_current_allocation_root_reference_internal();
EXPORT_COREMODULE AllocationRootReference* get_root_reference(void* ptr, MemLabelRef label);
EXPORT_COREMODULE void set_root_reference(void* ptr, size_t size, MemLabelRef label, AllocationRootReference* root);
EXPORT_COREMODULE void  assign_allocation_root(void* root, size_t size,  MemLabelRef label, const char* areaName, const char* objectName);

class AutoScopeRoot
{
public:
	AutoScopeRoot(void* root) { pushed = push_allocation_root(root, NULL, false); }
	~AutoScopeRoot() { if(pushed) pop_allocation_root(); }
	bool pushed;
};

#define GET_CURRENT_ALLOC_ROOT_REFERENCE() get_current_allocation_root_reference_internal()
#define SET_ALLOC_OWNER(root) AutoScopeRoot autoScopeRoot(root)
#define GET_ROOT_REFERENCE(ptr, label) get_root_reference(ptr, label)
#define SET_ROOT_REFERENCE(ptr, size, label, rootRef) set_root_reference(ptr, size, label, rootRef)
#define UNITY_TRANSFER_OWNERSHIP(source, label, newRootRef) transfer_ownership(source, label, newRootRef)

#else

#define GET_CURRENT_ALLOC_ROOT_REFERENCE() (AllocationRootReference*)NULL
#define SET_ALLOC_OWNER(root) {}
#define UNITY_TRANSFER_OWNERSHIP(source, label, newRootRef) {}
#define SET_ROOT_REFERENCE(ptr, size, label, rootRef) {}

#endif

#if UNITY_PS3		// ps3 new can pass in 2 parameters on the front ... size and alignment
EXPORT_COREMODULE void* operator new (size_t size, size_t alignment, MemLabelRef label, int align, const char* areaName, const char* objectName, const char* file, int line);
EXPORT_COREMODULE void* operator new (size_t size, size_t alignment, MemLabelRef label, int align, const char* file, int line);
#endif

#define UNITY_MALLOC(label, size)                      malloc_internal(size, kDefaultMemoryAlignment, label, kAllocateOptionNone, __FILE_STRIPPED__, __LINE__)
#define UNITY_MALLOC_NULL(label, size)                 malloc_internal(size, kDefaultMemoryAlignment, label, kAllocateOptionReturnNullIfOutOfMemory, __FILE_STRIPPED__, __LINE__)
#define UNITY_MALLOC_ALIGNED(label, size, align)       malloc_internal(size, align, label, kAllocateOptionNone, __FILE_STRIPPED__, __LINE__)
#define UNITY_MALLOC_ALIGNED_NULL(label, size, align)  malloc_internal(size, align, label, kAllocateOptionReturnNullIfOutOfMemory, __FILE_STRIPPED__, __LINE__)
#define UNITY_CALLOC(label, count, size)               calloc_internal(count, size, kDefaultMemoryAlignment, label, kAllocateOptionNone, __FILE_STRIPPED__, __LINE__)
#define UNITY_CALLOC_NULL(label, count, size)          calloc_internal(count, size, kDefaultMemoryAlignment, label, kAllocateOptionReturnNullIfOutOfMemory, __FILE_STRIPPED__, __LINE__)
#define UNITY_REALLOC_(label, ptr, size)               realloc_internal(ptr, size, kDefaultMemoryAlignment, label, kAllocateOptionNone, __FILE_STRIPPED__, __LINE__)
#define UNITY_REALLOC_NULL(label, ptr, size)           realloc_internal(ptr, size, kDefaultMemoryAlignment, label, kAllocateOptionReturnNullIfOutOfMemory, __FILE_STRIPPED__, __LINE__)
#define UNITY_REALLOC_ALIGNED(label, ptr, size, align) realloc_internal(ptr, size, align, label, kAllocateOptionNone, __FILE_STRIPPED__, __LINE__)
#define UNITY_REALLOC_ALIGNED_NULL(label, ptr, size, align) realloc_internal(ptr, size, align, label, kAllocateOptionReturnNullIfOutOfMemory, __FILE_STRIPPED__, __LINE__)
#define UNITY_FREE(label, ptr)                         free_alloc_internal(ptr, label)


#define UNITY_NEW(type, label) new (label, kDefaultMemoryAlignment, __FILE_STRIPPED__, __LINE__) type
#define UNITY_NEW_ALIGNED(type, label, align) new (label, align, __FILE_STRIPPED__, __LINE__) type
#define UNITY_DELETE(ptr, label) { delete_internal(ptr, label); ptr = NULL; }

#if ENABLE_MEM_PROFILER
	void register_external_gfx_allocation(void* ptr, size_t size, size_t related, const char* file, int line);
	void register_external_gfx_deallocation(void* ptr, const char* file, int line);

	template<typename T>
	inline T* pop_allocation_root_after_new(T* p)
	{
		// new is called when constructing the argument and p is set as root while in the constructor
		pop_allocation_root();
		return p;
	}

	#define UNITY_NEW_AS_ROOT(type, label, areaName, objectName) pop_allocation_root_after_new(new (label, kDefaultMemoryAlignment, areaName, objectName, __FILE_STRIPPED__, __LINE__) type)
	#define UNITY_NEW_AS_ROOT_ALIGNED(type, label, align, areaName, objectName) pop_allocation_root_after_new(new (label, align, areaName, objectName, __FILE_STRIPPED__, __LINE__) type)
	#define SET_PTR_AS_ROOT(ptr, size, label, areaName, objectName) assign_allocation_root(ptr, size, label, areaName, objectName)
	#define REGISTER_EXTERNAL_GFX_ALLOCATION_REF(ptr, size, related) register_external_gfx_allocation((void*)ptr, size, (size_t)related, __FILE_STRIPPED__, __LINE__)
	#define REGISTER_EXTERNAL_GFX_DEALLOCATION(ptr) register_external_gfx_deallocation((void*)ptr, __FILE_STRIPPED__, __LINE__)
#else
	#define UNITY_NEW_AS_ROOT(type, label, areaName, objectName) new (label, kDefaultMemoryAlignment, __FILE_STRIPPED__, __LINE__) type
	#define UNITY_NEW_AS_ROOT_ALIGNED(type, label, align, areaName, objectName) new (label, align, __FILE_STRIPPED__, __LINE__) type
	#define SET_PTR_AS_ROOT(ptr, size, label, areaName, objectName) {}
	#define REGISTER_EXTERNAL_GFX_ALLOCATION_REF(ptr, size, related) {}
	#define REGISTER_EXTERNAL_GFX_DEALLOCATION(ptr) {}
#endif

// Deprecated -> Move to new Macros
#define UNITY_ALLOC(label, size, align)                UNITY_MALLOC_ALIGNED(label, size, align)


#if UNITY_USE_PLATFORM_MEMORYMACROS			// define UNITY_USE_PLATFORM_MEMORYMACROS in PlatformPrefixConfigure.h
#include "Allocator/PlatformMemoryMacros.h"
#endif

/// ALLOC_TEMP allocates temporary memory that stays alive only inside the block it was allocated in.
/// It will automatically get freed!
/// (Watch out that you dont place ALLOC_TEMP inside an if block and use the memory after the if block.
///
/// eg.
/// float* data;
/// ALLOC_TEMP(data, float, 500);

#define kMAX_TEMP_STACK_SIZE 2000

inline void* AlignPtr (void* p, size_t alignment)
{
	size_t a = alignment - 1;
	return (void*)(((size_t)p + a) & ~a);
}

template <typename T>
inline T AlignSize (T size, size_t alignment)
{
	size_t a = alignment - 1;
	return ((size + a) & ~a);
}

/// Return maximum alignment. Both alignments must be 2^n.
inline size_t MaxAlignment(size_t alignment1, size_t alignment2)
{
	return ((alignment1 - 1) | (alignment2 - 1)) + 1;
}

// NOTE: alloca may return NULL on iOS
// see https://developer.apple.com/library/ios/documentation/System/Conceptual/ManPages_iPhoneOS/man3/alloca.3.html
#define ALLOC_TEMP_ALIGNED(ptr,type,count,alignment) \
	FreeTempMemory freeTempMemory_##ptr; \
	{ \
		size_t allocSize = (count) * sizeof(type) + (alignment)-1; \
		void* allocPtr = NULL; \
		if (count > 0) { \
			if (allocSize < kMAX_TEMP_STACK_SIZE) { \
				allocPtr = alloca(allocSize); \
			} \
			if (allocPtr == NULL) { \
				allocPtr = UNITY_MALLOC_ALIGNED(kMemTempAlloc, allocSize, kDefaultMemoryAlignment); \
				freeTempMemory_##ptr.m_Memory = allocPtr; \
			} \
		} \
		ptr = reinterpret_cast<type*> (AlignPtr(allocPtr, alignment)); \
	} \
	ANALYSIS_ASSUME(ptr)

#if UNITY_PSP2 || UNITY_PS3 || UNITY_PS4

struct FreeTempGfxMemory
{
	FreeTempGfxMemory() : m_Memory (NULL) { }
	~FreeTempGfxMemory() { if (m_Memory) UNITY_FREE(kMemVertexData, m_Memory); }
	void* m_Memory;
};

// On Sony platforms we need to ensure that temp allocations for graphics memory are
// done in mapped memory and persist while in use by the GPU so this version uses the
// delayed release allocator.
#define ALLOC_TEMP_ALIGNED_GFX(ptr,type,count,alignment) \
	FreeTempGfxMemory freeTempMemory_##ptr; \
	{ \
		size_t allocSize = (count) * sizeof(type) + (alignment)-1; \
		void* allocPtr = NULL; \
		if ((count) != 0) { \
			allocPtr = UNITY_MALLOC_ALIGNED (kMemVertexData, allocSize, alignment); \
			freeTempMemory_##ptr.m_Memory = allocPtr; \
		} \
		ptr = reinterpret_cast<type*> (AlignPtr(allocPtr, alignment)); \
	} \
	ANALYSIS_ASSUME(ptr)

#else

#ifndef ALLOC_TEMP_ALIGNED_GFX
#define ALLOC_TEMP_ALIGNED_GFX ALLOC_TEMP_ALIGNED
#endif

#endif

#define ALLOC_TEMP(ptr, type, count) \
	ALLOC_TEMP_ALIGNED(ptr,type,count,kDefaultMemoryAlignment)

#define MALLOC_TEMP(ptr, size) \
	ALLOC_TEMP_ALIGNED(ptr,char,size,kDefaultMemoryAlignment)

#define ALLOC_TEMP_MANUAL(type,count) \
	(type*)UNITY_MALLOC_ALIGNED(kMemTempAlloc, (count) * sizeof (type), kDefaultMemoryAlignment)

#define FREE_TEMP_MANUAL(ptr) \
	UNITY_FREE(kMemTempAlloc, ptr)


#if UNITY_XENON
// UNITY_MEMCPY defined here:
#include "PlatformDependent/Xbox360/Source/XenonMemory.h"
#else
#define UNITY_MEMCPY(dest, src, count) memcpy(dest, src, count)
#endif

#endif
