#ifndef DYNAMIC_HEAP_ALLOCATOR_H_
#define DYNAMIC_HEAP_ALLOCATOR_H_

#if ENABLE_MEMORY_MANAGER

#include "BaseAllocator.h"
#include "Runtime/Threads/Mutex.h"
#include "Runtime/Allocator/LowLevelDefaultAllocator.h"
#include "Runtime/Utilities/LinkedList.h"

/****************************************************************************/
/* This is the Allocator we want all platform owners to use.                */
/* This allocator is based on tlsf and allocates blocks of memory to        */
/* which tlsf is applied. It keeps a list of tlsf blocks, and whenever	    */
/* a block is full, it switches to another block, or allocates a new block, */
/* if none are free. This can also make a split and have two pools of tlsf  */
/* block - one for small allocations, and one for large (to reduce internal */
/* fragmentation). The tricky part of this one, is to set up the block size */
/* according to the platform. Larger blocks is more efficient and fragments */
/* less, but is less flexible for limited memory platforms.                 */
/****************************************************************************/

class BucketAllocator;

template<class LLAllocator>
class DynamicHeapAllocator : public BaseAllocator
{
public:
	DynamicHeapAllocator (UInt32 poolIncrementSize, size_t splitLimit, bool useLocking, BucketAllocator* bucketAllocator, const char* name);
	~DynamicHeapAllocator ();

	virtual void*  Allocate (size_t size, int align);
	virtual void*  Reallocate (void* p, size_t size, int align);
	virtual void   Deallocate (void* p) { TryDeallocate(p); }
	virtual bool   TryDeallocate (void* p);
	virtual bool   Contains (const void* p) const;
	virtual size_t GetPtrSize(const void* ptr) const;
	virtual bool   CheckIntegrity();
#if USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER
	virtual ProfilerAllocationHeader* GetProfilerHeader(const void* ptr) const;
	virtual size_t GetRequestedPtrSize(const void* ptr) const;
	virtual bool   ValidateIntegrity(const void* ptr) const;
#endif // USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER

private:
	/// Lockless fixed-size allocator for small memory blocks.
	BucketAllocator* m_BucketAllocator;

	struct PoolElement : public ListElement
	{
		bool Contains(const void* ptr) const
		{
			return ptr >= memoryBase && ptr < memoryBase + memorySize;
		}
		void*  tlsfPool;
		char*  memoryBase;
		UInt32 memorySize;
		UInt32 allocationCount;
	};

	typedef List<PoolElement> PoolList;

	size_t m_SplitLimit;
	PoolElement& GetActivePool(size_t size);
	PoolList& GetPoolList(size_t size);

	PoolList m_SmallTLSFPools;
	PoolList m_LargeTLSFPools;

	mutable Mutex m_DHAMutex;
	bool m_UseLocking;
	size_t m_RequestedPoolSize;

	struct LargeAllocations
	{
		LargeAllocations* next;
		char*             allocatedPtr;
		void*             returnedPtr;
		size_t            allocatedSize;
		size_t            returnedSize;
	};
	LargeAllocations* m_FirstLargeAllocation;

	PoolElement* FindPoolFromPtr(const void* ptr);
	const PoolElement* FindPoolFromPtr(const void* ptr) const;
};

#endif
#endif
