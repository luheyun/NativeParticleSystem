#ifndef _MEMORY_MANAGER_H_
#define _MEMORY_MANAGER_H_

#include "Misc/AllocatorLabels.h"
#include "Allocator/BaseAllocator.h"
#include "Allocator/MemoryMacros.h"
#include "Threads/Mutex.h"

#if ENABLE_MEMORY_MANAGER

class MemoryManager
{
public:
	MemoryManager();
	~MemoryManager();
	static void StaticInitialize();
	static void StaticDestroy();

	void ThreadInitialize(size_t tempSize);
	void ThreadCleanup();

	bool IsInitialized() {return m_IsInitialized;}
	bool IsActive() {return m_IsActive;}

	void* Allocate(size_t size, int align, MemLabelRef label, int allocateOptions = kAllocateOptionNone, const char* file = NULL, int line = 0);
	void* Reallocate(void* ptr, size_t size, int align, MemLabelRef label, int allocateOptions = kAllocateOptionNone, const char* file = NULL, int line = 0);
	void  Deallocate(void* ptr, MemLabelRef label);
	void  Deallocate(void* ptr);

	BaseAllocator* GetAllocator(MemLabelRef label);
	int GetAllocatorIndex(BaseAllocator* alloc);
	BaseAllocator* GetAllocatorAtIndex( int index );

	MemLabelId AddCustomAllocator(BaseAllocator* allocator);
	BaseAllocator* GetCustomAllocator(MemLabelRef label);
	void RemoveCustomAllocator(MemLabelRef label);

	void SetTempAllocatorSize(size_t tempAllocatorSize);
	
	// Performs frame maintenance for all allocators including temp allocator
	void FrameMaintenance(bool cleanup = false);

	/// Performs frame maintenance of the temp allocator for this thread.
	/// Resizes used memory
	void ThreadTempAllocFrameMaintenance();
	
	static void* LowLevelAllocate(size_t size);
	static void* LowLevelCAllocate(size_t count, size_t size);
	static void* LowLevelReallocate(void* p, size_t size, size_t oldSize);
	static void  LowLevelFree(void* p, size_t oldSize);

	const char* GetAllocatorName( int i );
	const char* GetMemcatName( MemLabelRef label );

	size_t GetTotalAllocatedMemory();
	size_t GetTotalUnusedReservedMemory();
	size_t GetTotalReservedMemory();

	size_t GetTotalProfilerMemory();

	int GetAllocatorCount( );

	size_t GetAllocatedMemory( MemLabelRef label );
	int GetAllocCount( MemLabelRef label );
	size_t GetLargestAlloc(MemLabelRef label);


	size_t GetRegisteredGFXDriverMemory(){ return m_RegisteredGfxDriverMemory;}

	void StartLoggingAllocations(size_t logAllocationsThreshold = 0);
	void StopLoggingAllocations();

	void DisallowAllocationsOnThisThread();
	void ReallowAllocationsOnThisThread();
	void CheckDisalowAllocation();

	BaseAllocator* GetAllocatorContainingPtr(const void* ptr);

	static inline bool IsTempAllocatorLabel( MemLabelRef label ) { return GetLabelIdentifier(label) == kMemTempAllocId; }

	volatile static size_t m_LowLevelAllocated;
	volatile static size_t m_RegisteredGfxDriverMemory;

private:
	void   InitializeMainThreadAllocators();
	void   InitializeDefaultAllocators();
	bool   InitializeDebugAllocator();

	static const size_t  kMaxAllocators = 16;
	static const size_t  kMaxCustomAllocators = 512;

	int              m_NumAllocators;
	bool             m_LogAllocations;
	bool             m_IsInitialized;
	bool             m_IsActive;
	bool             m_IsInitializedDebugAllocator;

	// Allocator for temporary allocations that live only inside a single frame.
	BaseAllocator*   m_FrameTempAllocator;
	// Fixed-size allocator for small allocation.
	BaseAllocator*   m_BucketAllocator;
	// Fallback allocator we use before MemoryManager is initialized on main thread.
	BaseAllocator*   m_InitialFallbackAllocator;
	int              m_InitialFallbackTempAllocationsCount;

	BaseAllocator*   m_Allocators[kMaxAllocators];
	BaseAllocator*   m_MainAllocators[kMaxAllocators];
	BaseAllocator*   m_ThreadAllocators[kMaxAllocators];

	Mutex            m_CustomAllocatorMutex;
	BaseAllocator*   m_CustomAllocators[kMaxCustomAllocators];
	size_t           m_NextFreeCustomAllocatorIndex;
	
	size_t           m_LogAllocationsThreshold;

	struct LabelInfo
	{
		BaseAllocator*                alloc;
	};
	LabelInfo        m_AllocatorMap[kMemLabelCount];
	int              m_PS3DelayedReleaseIndex;
};
MemoryManager& GetMemoryManager();

#endif
#endif
