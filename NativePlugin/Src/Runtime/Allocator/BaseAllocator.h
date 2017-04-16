#pragma once

#include "Misc/AllocatorLabels.h"
#include "Utilities/NonCopyable.h"

#include <stddef.h> // for size_t

class BaseAllocator : public NonCopyable
{
public:
	BaseAllocator(const char* name);
	virtual ~BaseAllocator () {}

	/// Return allocator name
	const char* GetName() const { return m_Name; }
	/// Return allocator name
	UInt32 GetAllocatorIdentifier() const { return m_AllocatorIdentifier; }

	/// Allocate memory block of the specified size (minimum size) and alignment.
	virtual void*  Allocate (size_t size, int align) = 0;
	/// Reallocate memory block at the specified pointer.
	virtual void*  Reallocate (void* p, size_t size, int align) = 0;
	/// Deallocate memory block.
	virtual void   Deallocate(void* p) = 0;
	/// Combined Contains and Deallocate call to deallocate memory block if it belongs to the allocator.
	virtual bool   TryDeallocate(void* p);
	/// Return true if the pointer belongs to the allocator
	virtual bool   Contains (const void* p) const = 0;
	/// Return the allocation size for the ptr (actually allocated size, not requested one).
	virtual size_t GetPtrSize(const void* ptr) const = 0;

	/// Return true if allocator is ready. Only the TLS allocator uses this call since the TLS value for this thread might not have been initialized yet
	virtual bool   IsAssigned() const { return true; }
	/// Verify allocations
	virtual bool   CheckIntegrity() { return true; }

	/// Return the actual number of allocated bytes.
	virtual size_t GetAllocatedMemorySize() const { return m_TotalAllocatedBytes; }
	/// Get the reserved size of the allocator (including all overhead memory allocated).
	virtual size_t GetReservedMemorySize() const { return m_TotalReservedBytes; }
	/// Get the peak allocated size of the allocator
	virtual size_t GetPeakAllocatedMemorySize() const { return m_PeakAllocatedBytes; }
	/// Get profiler headers overhead size.
	virtual size_t GetBookKeepingMemorySize() const { return m_BookKeepingMemoryUsage; }

	// get the present number of allocations held
	virtual size_t GetNumberOfAllocations() const { return m_NumAllocations; }

	// return the free block count for each pow2
	virtual void GetFreeBlockCount(int* freeCount, int size) {}
	// return the used block count for each pow2
	virtual void GetUsedBlockCount(int* usedCount, int size) {}

	virtual void ThreadInitialize(BaseAllocator* allocator) {}
	virtual void ThreadCleanup() {}

	virtual void FrameMaintenance(bool cleanup) {}

#if USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER
	/// Return NULL if allocator does not allocate the memory profile header
	virtual ProfilerAllocationHeader* GetProfilerHeader(const void* ptr) const = 0;
	/// Return NULL if allocator does not allocate the memory profile header
	virtual size_t GetRequestedPtrSize(const void* ptr) const = 0;
	/// Return if ptr is valid and not corrupted allocation
	virtual bool   ValidateIntegrity(const void* ptr) const { return true; }
	/// Execute a callback for every current outstanding allocation on this allocator
	typedef void (*WalkAllocationsCallback)(const void* ptr, size_t size, void* const* callstack, size_t maxCallstackSize);
	virtual void   WalkAllocations(WalkAllocationsCallback callback) const;
	/// Log allocation information - intended to be used with WalkAllocations for leak reporting
	static void LogAllocationInfo(const void* ptr, size_t size, void* const* callstack, size_t maxCallstackSize);
#endif // USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER

	// Return true if this is a delayed release allocator, i.e. actual memory is freed sometime after deallocation, typically when in use by the GPU.
	virtual bool IsDelayedRelease() const { return false; }

protected:
	void RegisterAllocationData(size_t allocatedSize, size_t overhead);
	void RegisterDeallocationData(size_t allocatedSize, size_t overhead);

	const char* m_Name;
	UInt32 m_AllocatorIdentifier;
	UInt32 m_NumAllocations;                 ///< Allocation count
	size_t m_TotalAllocatedBytes;            ///< Memory requested by the allocator
	size_t m_TotalReservedBytes;             ///< All memory reserved by the allocator
	size_t m_PeakAllocatedBytes;             ///< Memory requested by the allocator
	mutable size_t m_BookKeepingMemoryUsage; ///< Memory used for bookkeeping (headers etc.)
};

inline void BaseAllocator::RegisterAllocationData(size_t allocatedSize, size_t overhead)
{
	// @TODO: Consider atomic operations
	m_TotalAllocatedBytes += allocatedSize;
	m_BookKeepingMemoryUsage += overhead;
	if (m_TotalAllocatedBytes > m_PeakAllocatedBytes)
		m_PeakAllocatedBytes = m_TotalAllocatedBytes;
	m_NumAllocations++;
}

inline void BaseAllocator::RegisterDeallocationData(size_t allocatedSize, size_t overhead)
{
	m_TotalAllocatedBytes -= allocatedSize;
	m_BookKeepingMemoryUsage -= overhead;
	m_NumAllocations--;
}
