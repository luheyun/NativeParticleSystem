#include "PluginPrefix.h"
#include "DynamicHeapAllocator.h"

#if ENABLE_MEMORY_MANAGER

#include "Runtime/Allocator/AllocationHeader.h"
#include "Runtime/Allocator/BucketAllocator.h"
#include "Runtime/Utilities/BitUtility.h"
#include "Runtime/Profiler/MemoryProfiler.h"
#if UNITY_XENON
#include "PlatformDependent/Xbox360/Source/XenonMemory.h"
#endif
#include "Runtime/Threads/Thread.h"
#include "Runtime/Threads/AtomicOps.h"

#include "tlsf/tlsf.h"
#include <limits>

#if	UNITY_PS3
#include "Allocator/dlmalloc.h"
	// On PS3 we separate the DynHeap large allocs from general LowLevel allocs in the name of reducing fragmentation.
	// The DynHeap large allocs are now the ONLY client for dlmalloc
	#define LARGE_BLOCK_ALLOC(_SIZE) dlmemalign (16, _SIZE)
	#define LARGE_BLOCK_FREE(_PTR,_SIZE) dlfree (_PTR)
#else
	#define LARGE_BLOCK_ALLOC(_SIZE) LLAllocator::Malloc (_SIZE)
	#define LARGE_BLOCK_FREE(_PTR, _SIZE) LLAllocator::Free (_PTR, _SIZE)
#endif

// Helper function to get proper allocated size from tlsf allocator
static size_t GetTlsfAllocationSize(const AllocationHeader* header)
{
	size_t size = tlsf_block_size(header->GetAllocationPtr());
	return header->AdjustUserPtrSize(size);
}


template<class LLAllocator>
DynamicHeapAllocator<LLAllocator>::DynamicHeapAllocator (UInt32 poolIncrementSize, size_t splitLimit, bool useLocking, BucketAllocator* bucketAllocator, const char* name)
	: BaseAllocator(name)
	, m_BucketAllocator (bucketAllocator)
	, m_UseLocking (useLocking)
{
	m_SplitLimit = splitLimit;
	m_RequestedPoolSize = poolIncrementSize;
	m_FirstLargeAllocation = NULL;
}

template<class LLAllocator>
DynamicHeapAllocator<LLAllocator>::~DynamicHeapAllocator()
{
	Mutex::AutoLock m(m_DHAMutex);

	for(ListIterator<PoolElement> i=m_SmallTLSFPools.begin();i != m_SmallTLSFPools.end();i++)
	{
		PoolElement& pool = *i;
		tlsf_destroy(pool.tlsfPool);
		LLAllocator::Free(pool.memoryBase, pool.memorySize);
	}
	m_SmallTLSFPools.clear();
	for(ListIterator<PoolElement> i=m_LargeTLSFPools.begin();i != m_LargeTLSFPools.end();i++)
	{
		PoolElement& pool = *i;
		tlsf_destroy(pool.tlsfPool);
		LLAllocator::Free(pool.memoryBase, pool.memorySize);
	}
	m_LargeTLSFPools.clear();
}

template<class LLAllocator>
typename DynamicHeapAllocator<LLAllocator>::PoolElement& DynamicHeapAllocator<LLAllocator>::GetActivePool( size_t size )
{
	return GetPoolList( size ).front();
}

template<class LLAllocator>
typename DynamicHeapAllocator<LLAllocator>::PoolList& DynamicHeapAllocator<LLAllocator>::GetPoolList( size_t size )
{
	return size < m_SplitLimit? m_SmallTLSFPools: m_LargeTLSFPools;
}

template<class LLAllocator>
void* DynamicHeapAllocator<LLAllocator>::Allocate(size_t size, int align)
{
	DebugAssert(align > 0 && align <= 16*1024 && IsPowerOfTwo(align));

	// Use lockless BucketAllocator for small allocations up to 64 bytes.
	if (m_BucketAllocator != NULL && m_BucketAllocator->BucketAllocator::CanAllocate(size, align))
	{
		void* realPtr = m_BucketAllocator->BucketAllocator::Allocate(size, align);
		if (realPtr != NULL)
			return realPtr;
	}

	if(m_UseLocking)
		m_DHAMutex.Lock();

	size_t realSize = AllocationHeader::CalculateNeededAllocationSize(size, align);

	/// align size to tlsf block requirements
	if (realSize > 32)
	{
		size_t tlsfalign = (1 << HighestBit(realSize >> 5)) - 1;

		if (DoesAdditionOverflow(realSize, tlsfalign))
		{
			FatalErrorMsg("Size overflow in allocator.");
			return NULL;
		}

		realSize = (realSize + tlsfalign) & ~tlsfalign;
	}

	char* newRealPtr = NULL;
	if (size < m_RequestedPoolSize && !GetPoolList(realSize).empty())
		newRealPtr = (char*)tlsf_memalign(GetActivePool(realSize).tlsfPool, align, realSize);

	LargeAllocations* largeAlloc = NULL;
	if (newRealPtr == NULL)
	{
		// only try to make new tlsfBlocks if the amount is less than a 16th of the blocksize - else spill to LargeAllocations
		if(size < m_RequestedPoolSize/4)
		{
			// not enough space in the current block.
			// Iterate from the back, and find one that fits the allocation
			// put the found block at the head of the list
			PoolList& poolList = GetPoolList(realSize);
			ListIterator<PoolElement> pool = poolList.end();
			--pool;
			while(pool != poolList.end()) // List wraps around so end is the element just before the head of the list
			{
				newRealPtr = (char*)tlsf_memalign(pool->tlsfPool, align, realSize);
				if (newRealPtr != NULL)
				{
					// push_front removes the node from the list and reinserts it at the start
					Mutex::AutoLock m(m_DHAMutex);
					poolList.push_front(*pool);
					break;
				}
				--pool;
			}

			if (newRealPtr == 0)
			{
				size_t allocatePoolSize = m_RequestedPoolSize;
				void* memoryBlock = NULL;

			#if UNITY_ANDROID
				// @TODO:
				// On android we reload libunity (and keep activity around)
				// this results in leaks, as MemoryManager::m_InitialFallbackAllocator is never freed actually
				// more to it: even if we free the mem - the hole left is taken by other mallocs
				// so we cant reuse it on re-init, effectively allocing anew
				// this workaround can be removed when unity can be cleanly reloaded (sweet dreams)
				static const size_t _InitialFallbackAllocMemBlock_Size = 1024*1024;
				static bool _InitialFallbackAllocMemBlock_Taken = false;
				static char ALIGN_TYPE(16) _InitialFallbackAllocMemBlock[_InitialFallbackAllocMemBlock_Size];

				if(!_InitialFallbackAllocMemBlock_Taken)
				{
					Assert(_InitialFallbackAllocMemBlock_Size == m_RequestedPoolSize);
					_InitialFallbackAllocMemBlock_Taken = true;
					memoryBlock = _InitialFallbackAllocMemBlock;
				}
			#endif

				while(!memoryBlock && allocatePoolSize > size*2)
				{
					memoryBlock = LLAllocator::Malloc(allocatePoolSize);
					if(!memoryBlock)
						allocatePoolSize /= 2;
				}

				if(memoryBlock)
				{
					m_TotalReservedBytes += allocatePoolSize;
					PoolElement* newPoolPtr = (PoolElement*)LLAllocator::Malloc(sizeof(PoolElement));
					PoolElement& newPool = *new (newPoolPtr) PoolElement();
					newPool.memoryBase = (char*)memoryBlock;
					newPool.memorySize = allocatePoolSize;
					newPool.tlsfPool = tlsf_create(memoryBlock, allocatePoolSize);
					newPool.allocationCount = 0;

					{
						Mutex::AutoLock lock(m_DHAMutex);
						poolList.push_front(newPool);
					}

					newRealPtr = (char*)tlsf_memalign(GetActivePool(realSize).tlsfPool, align, realSize);
				}
			}
		}

		// System might be a bit slow when allocating large portions of memory
		int largeAllocRetries = 0;
		while (newRealPtr == 0)
		{
			// Large alloc uses AllocationHeaderWithSize header
			realSize = AllocationHeaderWithSize::CalculateNeededAllocationSize(size, align);
			char* largeAllocPtr = (char*) LARGE_BLOCK_ALLOC(realSize);
			if (largeAllocPtr == NULL)
			{
				// Let system a chance to free some memory for us before quit with fatal error.
				if (++largeAllocRetries < 5)
				{
					printf_console("DynamicHeapAllocator allocation probe %d failed - Could not get memory for large allocation %llu.\n", largeAllocRetries, (unsigned long long)size);
					Thread::Sleep(0.05 * largeAllocRetries);
					continue;
				}

				printf_console("DynamicHeapAllocator out of memory - Could not get memory for large allocation %llu!\n", (unsigned long long)size);
				if (m_UseLocking)
					m_DHAMutex.Unlock();
				return NULL;
			}

			// large allocation that don't fit on a clean block
			largeAlloc = (LargeAllocations*)LLAllocator::Malloc(sizeof(LargeAllocations));
			largeAlloc->allocatedPtr = largeAllocPtr;

			largeAlloc->allocatedSize = realSize;
			largeAlloc->returnedSize = size;
			m_TotalReservedBytes += size;
			{
				Mutex::AutoLock lock(m_DHAMutex);
				largeAlloc->next = m_FirstLargeAllocation;
				m_FirstLargeAllocation = largeAlloc;
			}
			newRealPtr = largeAlloc->allocatedPtr;
		}
	}

	void* ptr;
	if (!largeAlloc)
	{
		GetActivePool(realSize).allocationCount++;
		AllocationHeader* newAllocHeader = AllocationHeader::Init(newRealPtr, m_AllocatorIdentifier, size, align);
		ptr = newAllocHeader->GetUserPtr();

		RegisterAllocationData(GetTlsfAllocationSize(newAllocHeader), newAllocHeader->GetOverheadSize());
	}
	else
	{
		AllocationHeaderWithSize* newAllocHeader = AllocationHeaderWithSize::Init(newRealPtr, m_AllocatorIdentifier, size, align);
		ptr = largeAlloc->returnedPtr = newAllocHeader->GetUserPtr();

		RegisterAllocationData(size, largeAlloc->allocatedSize - size);
	}

	if(m_UseLocking)
		m_DHAMutex.Unlock();

	return ptr;
}

template<class LLAllocator>
void* DynamicHeapAllocator<LLAllocator>::Reallocate (void* p, size_t size, int align)
{
	if (p == NULL)
		return Allocate(size, align);

	if (size == 0)
	{
		Deallocate(p);
		return NULL;
	}

	size_t oldSize = m_BucketAllocator != NULL ? m_BucketAllocator->BucketAllocator::GetPtrSize(p) : 0;
	if (oldSize != 0)
	{
		// Check if we can stay in BucketAllocator
		if (m_BucketAllocator->BucketAllocator::CanAllocate(size, align))
		{
			void* realPtr = m_BucketAllocator->BucketAllocator::Reallocate(p, size, align);
			if (realPtr != NULL)
				return realPtr;
		}

		// Size is too large, use tlsf
		void* newPtr = Allocate(size, align);
		if (newPtr != NULL)
			UNITY_MEMCPY(newPtr, p, oldSize);
		m_BucketAllocator->BucketAllocator::Deallocate(p);

		return newPtr;
	}

	if(m_UseLocking)
		m_DHAMutex.Lock();

	// Check if it is tlsf allocation and we can realloc
	PoolElement* allocedPool = FindPoolFromPtr(p);
	if (allocedPool != NULL)
	{
		const AllocationHeader* allocHeader = AllocationHeader::GetAllocationHeader(p);
		void* realPtr = allocHeader->GetAllocationPtr();

		oldSize = GetTlsfAllocationSize(allocHeader);
		size_t oldOverheadSize = allocHeader->GetOverheadSize();
		size_t oldPadCount = allocHeader->GetPaddingCount();

		size_t newRealSize = AllocationHeader::CalculateNeededAllocationSize(size, align);
		char* newRealPtr = (char*) tlsf_realloc_align(allocedPool->tlsfPool, realPtr, align, newRealSize);
		if (newRealPtr != NULL)
		{
			AllocationHeader* newAllocHeader = AllocationHeader::Init(newRealPtr, m_AllocatorIdentifier, size, align);
			RegisterAllocationData(GetTlsfAllocationSize(newAllocHeader), newAllocHeader->GetOverheadSize());
			RegisterDeallocationData(oldSize, oldOverheadSize);

			// Move memory, because alignment might be different
			size_t newPadCount = AllocationHeader::GetRequiredPadding(newRealPtr, align);
			if (newPadCount != oldPadCount)
			{
				// new ptr needs different align padding. move memory and repad
				char* srcptr = newRealPtr + AllocationHeader::GetSize() + oldPadCount;
				char* dstptr = newRealPtr + AllocationHeader::GetSize() + newPadCount;
				memmove(dstptr, srcptr, std::min<size_t>(oldSize, size));
			}

			if (m_UseLocking)
				m_DHAMutex.Unlock();

			return newAllocHeader->GetUserPtr();
		}
	}

	if (oldSize == 0)
	{
		// This is large allocation
		const AllocationHeaderWithSize* largeAllocHeader = AllocationHeaderWithSize::GetAllocationHeader(p);
		oldSize = largeAllocHeader->GetAllocationSize();

		// TODO: Use LLAllocator::Realloc
	}

	// Go Allocate/Deallocate path
	void* newPtr = Allocate(size, align);
	if (newPtr != NULL)
		UNITY_MEMCPY(newPtr, p, std::min<size_t>(oldSize, size));

	Deallocate(p);

	if(m_UseLocking)
		m_DHAMutex.Unlock();

	return newPtr;
}

template<class LLAllocator>
bool DynamicHeapAllocator<LLAllocator>::TryDeallocate(void* p)
{
	if (p == NULL)
		return true;

	// Check if ptr was allocated by BucketAllocator
	if (m_BucketAllocator != NULL && m_BucketAllocator->BucketAllocator::TryDeallocate(p))
		return true;

	if(m_UseLocking)
		m_DHAMutex.Lock();

	bool deallocated = false;
	PoolElement* allocedPool = FindPoolFromPtr(p);
	if (allocedPool != NULL)
	{
		// tlsf allocation
		const AllocationHeader* allocHeader = AllocationHeader::GetAllocationHeader(p);
		void* realPtr = allocHeader->GetAllocationPtr();

		RegisterDeallocationData(GetTlsfAllocationSize(allocHeader), allocHeader->GetOverheadSize());

		allocedPool->allocationCount--;
		tlsf_free(allocedPool->tlsfPool, realPtr);
		if (allocedPool->allocationCount == 0)
		{
			{
				Mutex::AutoLock lock(m_DHAMutex);
				allocedPool->RemoveFromList();
			}
			tlsf_destroy(allocedPool->tlsfPool);
			LLAllocator::Free(allocedPool->memoryBase, allocedPool->memorySize);
			m_TotalReservedBytes -= allocedPool->memorySize;
			allocedPool->~PoolElement();
			LLAllocator::Free(allocedPool, sizeof(PoolElement));
		}

		deallocated = true;
	}
	else
	{
		// large allocation
		LargeAllocations* alloc = m_FirstLargeAllocation;
		LargeAllocations* prev = NULL;
		while (alloc != NULL)
		{
			if (alloc->returnedPtr == p)
			{
				const AllocationHeaderWithSize* largeAllocHeader = AllocationHeaderWithSize::GetAllocationHeader(p);

				RegisterDeallocationData(alloc->returnedSize, alloc->allocatedSize - alloc->returnedSize);

				{
					Mutex::AutoLock lock(m_DHAMutex);
					if(prev == NULL)
						m_FirstLargeAllocation = alloc->next;
					else
						prev->next = alloc->next;
				}

				m_TotalReservedBytes -= alloc->returnedSize;
				LARGE_BLOCK_FREE(alloc->allocatedPtr, alloc->allocatedSize);
				LLAllocator::Free(alloc, sizeof(LargeAllocations));

				deallocated = true;
				break;
			}
			prev = alloc;
			alloc = alloc->next;
		}
	}

	if(m_UseLocking)
		m_DHAMutex.Unlock();

	return deallocated;
}

void ValidateTlsfAllocation(void* memptr, size_t /*size*/, int isused, void* /*userptr*/)
{
	if (isused)
	{
		Assert(AllocationHeader::ValidateIntegrity(memptr, -1));
	}
}

template<class LLAllocator>
bool DynamicHeapAllocator<LLAllocator>::CheckIntegrity()
{
	Mutex::AutoLock m(m_DHAMutex);
	for(ListIterator<PoolElement> i=m_SmallTLSFPools.begin();i != m_SmallTLSFPools.end();i++)
		tlsf_check_heap(i->tlsfPool);
	for(ListIterator<PoolElement> i=m_LargeTLSFPools.begin();i != m_LargeTLSFPools.end();i++)
		tlsf_check_heap(i->tlsfPool);

	for(ListIterator<PoolElement> i=m_SmallTLSFPools.begin();i != m_SmallTLSFPools.end();i++)
		tlsf_walk_heap(i->tlsfPool, &ValidateTlsfAllocation, NULL);
	for(ListIterator<PoolElement> i=m_LargeTLSFPools.begin();i != m_LargeTLSFPools.end();i++)
		tlsf_walk_heap(i->tlsfPool, &ValidateTlsfAllocation, NULL);

	// TODO: Check bucket allocator
	// TODO: Check large allocations

	return true;
}

#if USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER

template<class LLAllocator>
ProfilerAllocationHeader* DynamicHeapAllocator<LLAllocator>::GetProfilerHeader(const void* ptr) const
{
#ifndef BUCKETALLOCATOR_USES_ALLOCATION_HEADER
	if (m_BucketAllocator != NULL && m_BucketAllocator->BucketAllocator::Contains(ptr))
		return NULL;
#endif // !BUCKETALLOCATOR_USES_ALLOCATION_HEADER

	// If ENABLE_MEM_PROFILER is defined, then AllocationHeader struct should be equal to AllocationHeaderWithSize
	CompileTimeAssert((IsBaseOfType<AllocationSizeHeader, AllocationHeader>::result && sizeof(AllocationHeader) == sizeof(AllocationHeaderWithSize)),
		"AllocationHeader must be equal to AllocationHeaderWithSize and contain size!");
	CompileTimeAssert((IsBaseOfType<AllocationSizeHeader, AllocationHeaderWithSize>::result && sizeof(AllocationHeader) == sizeof(AllocationHeaderWithSize)),
		"AllocationHeaderWithSize must be equal to AllocationHeader and contain size!");

	// BucketAllocator and DynamicHeapAllocator have the same AllocationHeader
	const AllocationHeader* allocHeader = AllocationHeader::GetAllocationHeader(ptr);
	return allocHeader->GetProfilerHeader();
}

template<class LLAllocator>
size_t DynamicHeapAllocator<LLAllocator>::GetRequestedPtrSize(const void* ptr) const
{
#ifndef BUCKETALLOCATOR_USES_ALLOCATION_HEADER
	if (m_BucketAllocator != NULL)
	{
		size_t size = m_BucketAllocator->BucketAllocator::GetPtrSize(ptr);
		if (size != 0)
			return size;
	}
#endif // !BUCKETALLOCATOR_USES_ALLOCATION_HEADER

	// If ENABLE_MEM_PROFILER is defined, then AllocationHeader struct should be equal to AllocationHeaderWithSize

	// BucketAllocator and DynamicHeapAllocator have the same AllocationHeader
	const AllocationHeader* allocHeader = AllocationHeader::GetAllocationHeader(ptr);
	return allocHeader->GetAllocationSize();
}

template<class LLAllocator>
bool DynamicHeapAllocator<LLAllocator>::ValidateIntegrity(const void* ptr) const
{
#ifndef BUCKETALLOCATOR_USES_ALLOCATION_HEADER
	if (m_BucketAllocator != NULL && m_BucketAllocator->BucketAllocator::Contains(ptr))
		return true;
#endif // !BUCKETALLOCATOR_USES_ALLOCATION_HEADER

	// DualThreadAllocator uses BucketAllocator and DynamicHeapAllocator that have the same AllocationHeader
	const AllocationHeader* allocHeader = AllocationHeader::GetAllocationHeader(ptr);
#if USE_MEMORY_DEBUGGING
	if ((allocHeader->GetAllocatorIdentifier() != GetAllocatorIdentifier()) &&
		(m_BucketAllocator != NULL && allocHeader->GetAllocatorIdentifier() != m_BucketAllocator->GetAllocatorIdentifier()))
		return false;
#endif
	return AllocationHeader::ValidateIntegrity(allocHeader->GetAllocationPtr(), -1);
}

#endif // USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER

struct BlockCounter
{
	int* blockCount;
	int size;
};

void FreeBlockCount (void* /*memptr*/, size_t size, int isused, void* userptr)
{
	BlockCounter* counter = (BlockCounter*)userptr;
	if (!isused )
	{
		int index = HighestBit(size);
		index = index >= counter->size ? counter->size-1 : index;
		counter->blockCount[index]++;
	}
}

void UsedBlockCount (void* /*memptr*/, size_t size, int isused, void* userptr)
{
	BlockCounter* counter = (BlockCounter*)userptr;
	if (isused )
	{
		int index = HighestBit(size);
		index = index >= counter->size ? counter->size-1 : index;
		counter->blockCount[index]++;
	}
}

template<class LLAllocator>
typename DynamicHeapAllocator<LLAllocator>::PoolElement* DynamicHeapAllocator<LLAllocator>::FindPoolFromPtr(const void* ptr)
{
	for (ListIterator<PoolElement> i = m_SmallTLSFPools.begin(); i != m_SmallTLSFPools.end(); i++)
	{
		if (i->Contains(ptr))
			return &*i;
	}
	for (ListIterator<PoolElement> i = m_LargeTLSFPools.begin(); i != m_LargeTLSFPools.end(); i++)
	{
		if (i->Contains(ptr))
			return &*i;
	}
	return NULL;
}

template<class LLAllocator>
const typename DynamicHeapAllocator<LLAllocator>::PoolElement* DynamicHeapAllocator<LLAllocator>::FindPoolFromPtr(const void* ptr) const
{
	for (ListConstIterator<PoolElement> i = m_SmallTLSFPools.begin(); i != m_SmallTLSFPools.end(); i++)
	{
		if (i->Contains(ptr))
			return &*i;
	}
	for (ListConstIterator<PoolElement> i = m_LargeTLSFPools.begin(); i != m_LargeTLSFPools.end(); i++)
	{
		if (i->Contains(ptr))
			return &*i;
	}
	return NULL;
}


template<class LLAlloctor>
bool DynamicHeapAllocator<LLAlloctor>::Contains (const void* p) const
{
	if (m_BucketAllocator != NULL && m_BucketAllocator->BucketAllocator::Contains(p))
		return true;

	bool useLocking = m_UseLocking || !Thread::CurrentThreadIsMainThread();
	if(useLocking)
		m_DHAMutex.Lock();

	if(FindPoolFromPtr(p) != NULL)
	{
		if(useLocking)
			m_DHAMutex.Unlock();
		return true;
	}

	// is this a largeAllocation
	LargeAllocations* alloc = m_FirstLargeAllocation;
	while (alloc != NULL)
	{
		if (alloc->returnedPtr == p)
		{
			if(useLocking)
				m_DHAMutex.Unlock();
			return true;
		}
		alloc = alloc->next;
	}
	if(useLocking)
		m_DHAMutex.Unlock();
	return false;

}

template<class LLAlloctor>
size_t DynamicHeapAllocator<LLAlloctor>::GetPtrSize( const void* ptr ) const
{
	size_t size = 0;
	if (m_BucketAllocator != NULL)
	{
		size = m_BucketAllocator->BucketAllocator::GetPtrSize(ptr);
		if (size != 0)
			return size;
	}

	bool useLocking = m_UseLocking || !Thread::CurrentThreadIsMainThread();
	if (useLocking)
		m_DHAMutex.Lock();

	if (FindPoolFromPtr(ptr) != NULL)
	{
		// tlsf alocation
		const AllocationHeader* allocHeader = AllocationHeader::GetAllocationHeader(ptr);
		size = GetTlsfAllocationSize(allocHeader);

		if (useLocking)
			m_DHAMutex.Unlock();

		return size;
	}

	// large allocation
	const AllocationHeaderWithSize* largeAllocHeader = AllocationHeaderWithSize::GetAllocationHeader(ptr);
	size = largeAllocHeader->GetAllocationSize();

	if (useLocking)
		m_DHAMutex.Unlock();

	return size;
}

template class DynamicHeapAllocator<LowLevelAllocator>;

#if UNITY_XENON && XBOX_USE_DEBUG_MEMORY
template class DynamicHeapAllocator<LowLevelAllocatorDebugMem>;
#endif
#if UNITY_XBOXONE
#include "PlatformDependent/XboxOne/Source/Allocator/XboxOneGPUDefaultAllocatorDefinition.h"
#endif // UNITY_XBOXONE
#endif

#undef	LARGE_BLOCK_ALLOC
#undef	LARGE_BLOCK_FREE
