#ifndef _MEMORY_MACROS_H_
#define _MEMORY_MACROS_H_

#include <limits>
#include <new>
#include "Runtime/Utilities/FileStripped.h"
#include "Misc/AllocatorLabels.h"

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

EXPORT_COREMODULE void* operator new (size_t size, MemLabelRef label, int align, const char* areaName, const char* objectName, const char* file, int line);
EXPORT_COREMODULE void* operator new (size_t size, MemLabelRef label, int align, const char* file, int line);

EXPORT_COREMODULE void operator delete (void* p, MemLabelRef label, int align, const char* areaName, const char* objectName, const char* file, int line);
EXPORT_COREMODULE void operator delete (void* p, MemLabelRef label, int align, const char* file, int line);

EXPORT_COREMODULE void* malloc_internal(size_t size, size_t align, MemLabelRef label, int allocateOptions, const char* file, int line);
EXPORT_COREMODULE void* calloc_internal(size_t count, size_t size, int align, MemLabelRef label, int allocateOptions, const char* file, int line);
EXPORT_COREMODULE void* realloc_internal(void* ptr, size_t size, int align, MemLabelRef label, int allocateOptions, const char* file, int line);
void free_internal(void* ptr);
EXPORT_COREMODULE void  free_alloc_internal(void* ptr, MemLabelRef label);

#define GET_CURRENT_ALLOC_ROOT_REFERENCE() (AllocationRootReference*)NULL
#define SET_ALLOC_OWNER(root) {}
#define UNITY_TRANSFER_OWNERSHIP(source, label, newRootRef) {}
#define SET_ROOT_REFERENCE(ptr, size, label, rootRef) {}

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

template<typename T>
inline void delete_internal(T* ptr, MemLabelRef label) { if (ptr) ptr->~T(); UNITY_FREE(label, (void*)ptr); }

#define UNITY_NEW(type, label) new (label, kDefaultMemoryAlignment, __FILE_STRIPPED__, __LINE__) type
#define UNITY_NEW_ALIGNED(type, label, align) new (label, align, __FILE_STRIPPED__, __LINE__) type
#define UNITY_DELETE(ptr, label) { delete_internal(ptr, label); ptr = NULL; }

	#define UNITY_NEW_AS_ROOT(type, label, areaName, objectName) new (label, kDefaultMemoryAlignment, __FILE_STRIPPED__, __LINE__) type
	#define UNITY_NEW_AS_ROOT_ALIGNED(type, label, align, areaName, objectName) new (label, align, __FILE_STRIPPED__, __LINE__) type
	#define SET_PTR_AS_ROOT(ptr, size, label, areaName, objectName) {}
	#define REGISTER_EXTERNAL_GFX_ALLOCATION_REF(ptr, size, related) {}
	#define REGISTER_EXTERNAL_GFX_DEALLOCATION(ptr) {}

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

#include "Runtime/Allocator/STLAllocator.h"

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

struct FreeTempMemory
{
	FreeTempMemory() : m_Memory(NULL) { }
	~FreeTempMemory() { if (m_Memory) UNITY_FREE(kMemTempAlloc, m_Memory); }
	void* m_Memory;
};

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


#ifndef ALLOC_TEMP_ALIGNED_GFX
#define ALLOC_TEMP_ALIGNED_GFX ALLOC_TEMP_ALIGNED
#endif

#define ALLOC_TEMP(ptr, type, count) \
	ALLOC_TEMP_ALIGNED(ptr,type,count,kDefaultMemoryAlignment)

#define MALLOC_TEMP(ptr, size) \
	ALLOC_TEMP_ALIGNED(ptr,char,size,kDefaultMemoryAlignment)

#define ALLOC_TEMP_MANUAL(type,count) \
	(type*)UNITY_MALLOC_ALIGNED(kMemTempAlloc, (count) * sizeof (type), kDefaultMemoryAlignment)

#define FREE_TEMP_MANUAL(ptr) \
	UNITY_FREE(kMemTempAlloc, ptr)



#define UNITY_MEMCPY(dest, src, count) memcpy(dest, src, count)

#endif
