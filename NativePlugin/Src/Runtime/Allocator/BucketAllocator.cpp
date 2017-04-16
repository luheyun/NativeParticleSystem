#include "UnityPrefix.h"
#include "Runtime/Allocator/BucketAllocator.h"
#include "Runtime/Allocator/AllocationHeader.h"
#include "Runtime/Allocator/MemoryMacros.h"
#include "Runtime/Allocator/MemoryManager.h"
#include "Runtime/Profiler/MemoryProfiler.h"
#include "Runtime/Utilities/StaticAssert.h"

#if ENABLE_MEMORY_MANAGER && USE_BUCKET_ALLOCATOR

BucketAllocator::BucketAllocator(const char *name, size_t bucketGranularity, size_t bucketsCount, size_t largeBlockSize, size_t maxLargeBlocksCount)
	: BaseAllocator(name)
	, m_BucketGranularity(bucketGranularity)
	, m_BucketGranularityBits(HighestBit(bucketGranularity))
	, m_MaxBucketSize(bucketGranularity * bucketsCount)
	, m_LargeBlockSize(largeBlockSize)
	, m_UsedLargeBlocks(0)
	, m_MaxLargeBlocks(maxLargeBlocksCount)
{
	// We need m_BlockSize to be at least 256 bytes so we can have some header at the beginning
	// and can m_CurrentLargeBlockUsedSize low bits for m_LargeBlocks number.
	CompileTimeAssert((kBlockSize >= (1 << 8)) && ((kBlockSize & 0xFF) == 0), "Invalid BucketAllocator::kBlockSize value");

	// Bucket must be >= atomic_word in order to use its memory as AtomicNode without data
	Assert(m_BucketGranularity >= sizeof(atomic_word));
	Assert(IsPowerOfTwo(m_BucketGranularity));

	m_Buckets.resize_uninitialized(bucketsCount);
	for (size_t i = 0; i < bucketsCount; ++i)
	{
		size_t bucketSize = (1 + i) * m_BucketGranularity;
		size_t realBucketSize = GetRealBucketSize(bucketSize);
		m_Buckets[i] = UNITY_NEW_ALIGNED(Buckets(bucketSize, realBucketSize), kMemDefault, 16);
	}

	// Add some initial buckets
	m_LargeBlocks = reinterpret_cast<LargeBlock*>(UNITY_MALLOC(kMemDefault, sizeof(LargeBlock) * maxLargeBlocksCount));
	if (m_LargeBlocks == NULL || !AddLargeBlock())
	{
		for (size_t i = 0; i < bucketsCount; ++i)
			m_Buckets[i]->canGrow = 0;
	}
}

BucketAllocator::~BucketAllocator()
{
	for (size_t i = 0; i < m_Buckets.size(); ++i)
		UNITY_DELETE(m_Buckets[i], kMemDefault);

	for (int i = 0; i < m_UsedLargeBlocks; ++i)
		MemoryManager::LowLevelFree(m_LargeBlocks[i].realPtr, m_LargeBlockSize);

	UNITY_FREE(kMemDefault, m_LargeBlocks);
}

void* BucketAllocator::Allocate(size_t size, int alignment)
{
	if (!CanAllocate(size, alignment))
		return NULL;

	void* newRealPtr = NULL;
	Buckets* buckets = GetBucketsForSize(size);
	while (true)
	{
		newRealPtr = buckets->PopBucket();
		if (newRealPtr != NULL)
		{
			buckets->UpdateUsed(1);
			break;
		}

		// Fast check if we can allocate new block
		if (AtomicCompareExchange(&buckets->canGrow, 0, 0))
			return NULL;

		// Disallow multiple blocks creation
		int oldUsedBlocks = AtomicAdd(&buckets->usedBlocksCount, 0);
		Mutex::AutoLock lock(buckets->growMutex);
		if (oldUsedBlocks != buckets->usedBlocksCount)
			continue;

		if (!AddMoreBuckets(buckets))
			return NULL;
	}

#ifdef BUCKETALLOCATOR_USES_ALLOCATION_HEADER
	AllocationHeader* newAllocHeader = AllocationHeader::Init(newRealPtr, m_AllocatorIdentifier, size, alignment);
	return newAllocHeader->GetUserPtr();
#else // BUCKETALLOCATOR_USES_ALLOCATION_HEADER
	return newRealPtr;
#endif // BUCKETALLOCATOR_USES_ALLOCATION_HEADER
}

void* BucketAllocator::Reallocate(void* p, size_t size, int alignment)
{
	if (p == NULL)
		return Allocate(size, alignment);

	if (size == 0)
	{
		Deallocate(p);
		return NULL;
	}

	// Should it just stay in the same BlockList?
	Block* oldBlock = GetBlockFromPtr(p);
	size_t oldSize = oldBlock->bucketSize;
	if (oldSize >= size)
	{
#ifdef BUCKETALLOCATOR_USES_ALLOCATION_HEADER
		// Reinit header
		const AllocationHeader* allocHeader = AllocationHeader::GetAllocationHeader(p);
		void* realPtr = allocHeader->GetAllocationPtr();
		AllocationHeader::Init(realPtr, m_AllocatorIdentifier, size, alignment);
#endif // BUCKETALLOCATOR_USES_ALLOCATION_HEADER
		return p;
	}

	void* newPtr = Allocate(size, alignment);
	if (newPtr == NULL)
		return NULL;

	// We have constant alignment for all bucket sizes, so we can just copy old memory.
	// If we want to use different alignments for buckets we should consider repadding scheme like in DynamicHeapAllocator.
	memcpy(newPtr, p, std::min<size_t>(size, oldSize));

	DeallocateInternal(p);

	return newPtr;
}

size_t BucketAllocator::GetPtrSize(const void* p) const
{
	if (!ContainsPtr(p))
		return 0;

	return GetBlockFromPtr(p)->bucketSize;
}

size_t BucketAllocator::GetAllocatedMemorySize() const
{
	size_t size = 0;
	for (size_t i = 0; i < m_Buckets.size(); ++i)
	{
		size += m_Buckets[i]->GetUsedMemory();
	}

	return size;
}

size_t BucketAllocator::GetBookKeepingMemorySize() const
{
	return 0;
}

size_t BucketAllocator::GetReservedMemorySize() const
{
	return m_LargeBlockSize * AtomicAdd(&m_UsedLargeBlocks, 0);
}

size_t BucketAllocator::GetPeakAllocatedMemorySize() const
{
	size_t size = 0;
	for (size_t i = 0; i < m_Buckets.size(); ++i)
	{
		size += m_Buckets[i]->GetMaxUsedMemory();
	}

	return size;
}

#if USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER

ProfilerAllocationHeader* BucketAllocator::GetProfilerHeader(const void* ptr) const
{
#ifdef BUCKETALLOCATOR_USES_ALLOCATION_HEADER
	const AllocationHeader* allocHeader = AllocationHeader::GetAllocationHeader(ptr);
	return allocHeader->GetProfilerHeader();
#else // BUCKETALLOCATOR_USES_ALLOCATION_HEADER
	return NULL;
#endif // BUCKETALLOCATOR_USES_ALLOCATION_HEADER
}

size_t BucketAllocator::GetRequestedPtrSize(const void* ptr) const
{
#ifdef BUCKETALLOCATOR_USES_ALLOCATION_HEADER
	const AllocationHeader* allocHeader = AllocationHeader::GetAllocationHeader(ptr);
	return allocHeader->GetAllocationSize();
#else // BUCKETALLOCATOR_USES_ALLOCATION_HEADER
	return 0;
#endif // BUCKETALLOCATOR_USES_ALLOCATION_HEADER
}

#endif // USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER

bool BucketAllocator::AddLargeBlock()
{
	if (m_UsedLargeBlocks >= m_MaxLargeBlocks)
		return false;

	void* ptr = MemoryManager::LowLevelAllocate(m_LargeBlockSize);
	if (ptr == NULL)
		return false;

	m_LargeBlocks[m_UsedLargeBlocks].realPtr = ptr;
	m_LargeBlocks[m_UsedLargeBlocks].endPtr = static_cast<char*>(ptr) + m_LargeBlockSize;
	m_LargeBlocks[m_UsedLargeBlocks].firstBlockPtr = static_cast<char*>(AlignPtr(ptr, kBlockSize));

	AtomicExchange(&m_CurrentLargeBlockUsedSize, kBlockSize | m_UsedLargeBlocks); // One block is wasted because of alignment
	AtomicIncrement(&m_UsedLargeBlocks);

	return true;
}

bool BucketAllocator::AddMoreBuckets(Buckets* buckets)
{
	// Add new block from preallocated memory
	int newUsedSize = AtomicAdd(&m_CurrentLargeBlockUsedSize, kBlockSize);
	int largeBlockIndex = newUsedSize & 0xFF;
	if (newUsedSize < m_LargeBlockSize)
	{
		int largeBlockOffset = (newUsedSize & (~0xFF)) - kBlockSize;
		const LargeBlock& largeBlock = m_LargeBlocks[largeBlockIndex];

		AddBlockToBuckets(buckets, largeBlock.firstBlockPtr + largeBlockOffset, kBlockSize);
		return true;
	}

	// No free memory for blocks. Allocate new large block.
	Mutex::AutoLock lock(m_NewLargeBlockMutex);

	// Check if someone added a new block already
	if (largeBlockIndex == (m_CurrentLargeBlockUsedSize & 0xFF))
	{
		if (!AddLargeBlock())
		{
			AtomicCompareExchange(&buckets->canGrow, 0, 1);
			return false;
		}
	}

	return true;
}

int BucketAllocator::GetRealBucketSize(int size)
{
#ifdef BUCKETALLOCATOR_USES_ALLOCATION_HEADER
	size = AllocationHeader::CalculateNeededAllocationSize(size, kMaxAlignment);
#endif // BUCKETALLOCATOR_USES_ALLOCATION_HEADER
	return AlignSize(size, kMaxAlignment);
}

void BucketAllocator::AddBlockToBuckets(Buckets* buckets, void* ptr, int size)
{
	Assert(ptr == GetBlockFromPtr(ptr));

	AtomicAdd(&buckets->usedBlocksCount, 1);

	const int bucketsSize = buckets->bucketsSize;
	const int bucketSizeWithHeaders = GetRealBucketSize(bucketsSize);
	Block* block = new(ptr) Block(bucketsSize);

	char* p = reinterpret_cast<char*>(AlignPtr(block + 1, kMaxAlignment));
	char* endPtr = reinterpret_cast<char*>(ptr) +size - bucketSizeWithHeaders;
	while (p <= endPtr)
	{
		buckets->PushBucket(p);
		p += bucketSizeWithHeaders;
	}
}

#endif // ENABLE_MEMORY_MANAGER && USE_BUCKET_ALLOCATOR
