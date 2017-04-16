#ifndef BUCKET_ALLOCATOR_H_
#define BUCKET_ALLOCATOR_H_

#include "Runtime/Allocator/BaseAllocator.h"

#if ENABLE_MEMORY_MANAGER

#include "Runtime/Allocator/AllocationHeader.h"
#include "Runtime/Threads/AtomicQueue.h"
#include "Runtime/Threads/AtomicOps.h"
#include "Runtime/Threads/ExtendedAtomicOps.h"
#include "Runtime/Threads/Mutex.h"
#include "Runtime/Utilities/dynamic_array.h"

#if !defined(PLATFORM_HAS_NO_SUPPORT_FOR_BUCKET_ALLOCATOR)
#define PLATFORM_HAS_NO_SUPPORT_FOR_BUCKET_ALLOCATOR ((UNITY_WIN && UNITY_WP_8_1) || UNITY_LINUX || UNITY_ANDROID || UNITY_WINRT || UNITY_STV || UNITY_WEBGL || UNITY_XBOXONE)
#endif // !defined(PLATFORM_HAS_NO_SUPPORT_FOR_BUCKET_ALLOCATOR)

#define USE_BUCKET_ALLOCATOR ATOMIC_HAS_QUEUE && !PLATFORM_HAS_NO_SUPPORT_FOR_BUCKET_ALLOCATOR

#if USE_BUCKET_ALLOCATOR

// Define BUCKETALLOCATOR_USES_ALLOCATION_HEADER if you want to use memory debugging checks for small allocations
#define BUCKETALLOCATOR_USES_ALLOCATION_HEADER

/// Bucket allocator is used for allocations up to 64 bytes of memory.
/// It is represented by 4 blocks of a fixed-size "buckets" (for allocations of 16/32/48/64 bytes of memory).
/// Allocation is lockless, blocks are only growable.
class BucketAllocator : public BaseAllocator
{
public:
	BucketAllocator(const char *name, size_t bucketGranularity, size_t bucketsCount, size_t largeBlockSize, size_t maxLargeBlocksCount);
	~BucketAllocator();

	//@{ BaseAllocator interface
	virtual void*  Allocate(size_t size, int align);
	virtual void*  Reallocate(void* p, size_t size, int align);
	virtual bool   Contains(const void* p) const { return ContainsPtr(p); }
	virtual void   Deallocate(void* p) { DeallocateInternal(p); }
	virtual bool   TryDeallocate(void* p);
	virtual size_t GetPtrSize(const void* p) const;
	virtual size_t GetAllocatedMemorySize() const;
	virtual size_t GetReservedMemorySize() const;
	virtual size_t GetPeakAllocatedMemorySize() const;
	virtual size_t GetBookKeepingMemorySize() const;
#if USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER
	virtual ProfilerAllocationHeader* GetProfilerHeader(const void* ptr) const;
	virtual size_t GetRequestedPtrSize(const void* ptr) const;
#endif // USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER
	//@}

	/// Return true if BucketAllocator can allocate such amount of memory
	FORCE_INLINE bool   CanAllocate(size_t size, int align) const { return size <= m_MaxBucketSize && align <= kMaxAlignment; }
	/// Return true if p belongs to large blocks
	FORCE_INLINE bool   ContainsPtr(const void* p) const;

private:
	struct LargeBlock
	{
		LargeBlock(void* p, void* e, char* b) : realPtr(p), endPtr(e), firstBlockPtr(b) {}

		void* realPtr;
		void* endPtr;
		char* firstBlockPtr;
	};

	struct Block
	{
		Block(int bs) : bucketSize(bs) {}

		int bucketSize;
	};

	struct Buckets : public NonCopyable
	{
		Buckets(int size, int realSize)
			: usedBucketsCount(0), usedBlocksCount(0), maxUsedBucketsCount(0), bucketsSize(size), realBucketsSize(realSize), canGrow(1) {}

		FORCE_INLINE void* PopBucket() { return availableBuckets.Pop(); }
		FORCE_INLINE void  PushBucket(void* p) { availableBuckets.Push(reinterpret_cast<AtomicNode*>(p)); }
		FORCE_INLINE void  UpdateUsed(int delta)
		{
			int newUsed = AtomicAdd(&usedBucketsCount, delta);
			if (delta <= 0)
				return;
			int newMaxUsedBucketsCount;
			do // Update peak allocations
			{
				newMaxUsedBucketsCount = AtomicAdd(&maxUsedBucketsCount, 0);
			} while (newMaxUsedBucketsCount < newUsed && !AtomicCompareExchange(&maxUsedBucketsCount, newUsed, newMaxUsedBucketsCount));
		}

		size_t GetUsedMemory() const { return bucketsSize * AtomicAdd(&usedBucketsCount, 0); }
		size_t GetMaxUsedMemory() const { return bucketsSize * AtomicAdd(&maxUsedBucketsCount, 0); }

		//@{ This members should be properly aligned.
		ALIGN_TYPE(16) AtomicStack         availableBuckets;                ///< Available buckets.
		ALIGN_TYPE(4) mutable volatile int usedBucketsCount;                ///< Count of used buckets.
		ALIGN_TYPE(4) volatile int         usedBlocksCount;                 ///< Count of used small blocks.
		ALIGN_TYPE(4) mutable volatile int maxUsedBucketsCount;             ///< Highest count of used buckets.
		ALIGN_TYPE(4) volatile int         canGrow;                         ///< True if we can add new blocks
		//@}
		const int                          bucketsSize;                     ///< Size of this allocations.
		const int                          realBucketsSize;                 ///< Size of this bucket including AllocationHeader overhead.
		Mutex                              growMutex;                       ///< Mutex for new small block allocation
	};

	bool        AddLargeBlock();
	bool        AddMoreBuckets(Buckets* buckets);
	static int  GetRealBucketSize(int size);
	void        AddBlockToBuckets(Buckets* buckets, void* ptr, int size);
	FORCE_INLINE Block*   GetBlockFromPtr(const void* ptr) const { return reinterpret_cast<Block*>(((size_t) ptr) & ~(kBlockSize-1)); }
	FORCE_INLINE Buckets* GetBucketsForSize(size_t size) const { return m_Buckets[size > 0 ? ((size - 1) >> m_BucketGranularityBits) : 0]; }
	FORCE_INLINE void     DeallocateInternal(void* p);

	static const int           kMaxAlignment = 16;
	static const int           kBlockSize = 16 * 1024;

	const int                  m_BucketGranularity;
	const int                  m_BucketGranularityBits;
	const int                  m_MaxBucketSize;
	const int                  m_LargeBlockSize;                 ///< Size of large memory block for all small allocations.
	LargeBlock*                m_LargeBlocks;                    ///< Large blocks of continuous memory.
	ALIGN_TYPE(4) mutable volatile int m_UsedLargeBlocks;
	ALIGN_TYPE(4) volatile int m_CurrentLargeBlockUsedSize;
	const int                  m_MaxLargeBlocks;
	dynamic_array<Buckets*>    m_Buckets;                        ///< Buckets of various size.
	Mutex                      m_NewLargeBlockMutex;
};

FORCE_INLINE bool BucketAllocator::TryDeallocate(void* p)
{
	if (!ContainsPtr(p))
		return false;

	DeallocateInternal(p);
	return true;
}

FORCE_INLINE bool BucketAllocator::ContainsPtr(const void* p) const
{
	int currentUsed = AtomicAdd(&m_UsedLargeBlocks, 0);
	for (int i = 0; i < currentUsed; ++i)
	{
		if (p >= m_LargeBlocks[i].realPtr && p < m_LargeBlocks[i].endPtr)
			return true;
	}
	return false;
}

FORCE_INLINE void BucketAllocator::DeallocateInternal(void* p)
{
#ifdef BUCKETALLOCATOR_USES_ALLOCATION_HEADER
	const AllocationHeader* allocHeader = AllocationHeader::GetAllocationHeader(p);
	void* realPtr = allocHeader->GetAllocationPtr();
#else // BUCKETALLOCATOR_USES_ALLOCATION_HEADER
	void* realPtr = p;
#endif // BUCKETALLOCATOR_USES_ALLOCATION_HEADER

	Block* oldBlock = GetBlockFromPtr(realPtr);
	Buckets* buckets = GetBucketsForSize(oldBlock->bucketSize);
	buckets->PushBucket(realPtr);
	buckets->UpdateUsed(-1);
}

#else // ATOMIC_HAS_QUEUE

class BucketAllocator : public BaseAllocator
{
public:
	BucketAllocator(const char *name) : BaseAllocator(name) {}

	//@{ BaseAllocator interface
	virtual void*  Allocate(size_t size, int align) { return NULL; }
	virtual void*  Reallocate(void* p, size_t size, int align) { return NULL; }
	virtual void   Deallocate(void* p) {}
	virtual bool   Contains(const void* p) const { return false; }
	virtual size_t GetPtrSize(const void* p) const { return 0; }
	virtual size_t GetAllocatedMemorySize() const { return 0; }
	virtual size_t GetReservedMemorySize() const { return 0; }
	virtual size_t GetPeakAllocatedMemorySize() const { return 0; }
	virtual size_t GetBookKeepingMemorySize() const { return 0; }
#if USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER
	virtual ProfilerAllocationHeader* GetProfilerHeader(const void* ptr) const { return NULL; }
	virtual size_t GetRequestedPtrSize(const void* ptr) const { return 0; }
#endif // USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER
	//@}

	/// Deallocation probe. Return true on successful deallocation, false if it is not our ptr
	bool   TryDeallocate(void* p) { return false; }
	/// Return true if BucketAllocator can allocate such amount of memory
	bool   CanAllocate(size_t size, int align) const { return false; }
};

#endif // USE_BUCKET_ALLOCATOR

#endif // ENABLE_MEMORY_MANAGER
#endif // !BUCKET_ALLOCATOR_H_
