#include "Runtime/Allocator/ThreadsafeLinearAllocator.h"
#include "Runtime/Allocator/MemoryManager.h"
#include "Runtime/Threads/AtomicOps.h"
#include "Runtime/Profiler/Profiler.h"
#include "Runtime/Utilities/Stacktrace.h"

PROFILER_INFORMATION(gTempJobAllocGrow, "JobAlloc.Grow", kProfilerOther);
PROFILER_INFORMATION(gTempJobAllocOverflow, "JobAlloc.Overflow", kProfilerOther);

#define TLA_DEBUG_STACK_LEAK 0

struct ThreadsafeLinearAllocatorBlock
{
	UInt8*                     ptr;
	ALIGN_TYPE(4) volatile int usedSize;
	ALIGN_TYPE(4) volatile int allocationCount;
};

struct ThreadsafeLinearAllocatorHeader
{
	static size_t  CalculateNeededAllocationSize(size_t size, int align);
	static ThreadsafeLinearAllocatorHeader* SetupHeader(void* realPtr, size_t size, int align, int frameIndex)
	{
		void* startPtr = realPtr;
#if TLA_DEBUG_STACK_LEAK
		startPtr = static_cast<UInt8*>(realPtr) + sizeof(int);
#endif
		void* userPtr = AlignPtr(static_cast<UInt8*>(startPtr) + sizeof(ThreadsafeLinearAllocatorHeader), align);
		ThreadsafeLinearAllocatorHeader* header = GetHeader(userPtr);
		header->size = size;
		header->isOverflowAlloc = 0;
		header->blockIndex = 0;
		header->frameIndex = frameIndex;
		size_t overhead = static_cast<UInt8*>(userPtr) - static_cast<UInt8*>(realPtr);
		Assert(overhead < StaticPow2<23>::value);
		header->overheadSize = overhead;
		header->magic = kMagic;
#if TLA_DEBUG_STACK_LEAK
		UInt32 uSize = reinterpret_cast<UInt8*>(header) - static_cast<UInt8*>(realPtr);
		*(static_cast<UInt32*>(realPtr)) = uSize;
#endif
		return header;
	}
	inline static ThreadsafeLinearAllocatorHeader* GetHeader(void* ptr) { return static_cast<ThreadsafeLinearAllocatorHeader*>(ptr) -1; }
	inline void*          GetUserPtr() { return this + 1; }
	inline void*          GetRealPtr() { return static_cast<UInt8*>(GetUserPtr()) - overheadSize; }

#if TLA_DEBUG_STACK_LEAK
	static ThreadsafeLinearAllocatorHeader* GetHeaderFromRealPtr(void* ptr) 
	{ 
		UInt8* realPtr = static_cast<UInt8*>(ptr);
		UInt32* ptrU32 = static_cast<UInt32*>(ptr);

		return reinterpret_cast<ThreadsafeLinearAllocatorHeader*>(realPtr + *ptrU32); 	
	}
#endif

	static const UInt32 kMagic = 0xD06F00D;

	size_t size;
	UInt32 blockIndex      : 8;                            // So we can have 256 blocks at max.
	UInt32 isOverflowAlloc : 1;
	UInt32 overheadSize    : 23;                           // Difference between user pointer and real pointer
	UInt32 magic           : 28;                           // Magic value for the header check
	UInt32 frameIndex      : 4;                            // Index for allocation in m_FrameAllocationCount 
#if TLA_DEBUG_STACK_LEAK
	void*  nextPtr;
	void*  callstack[20];
#endif
};

size_t ThreadsafeLinearAllocatorHeader::CalculateNeededAllocationSize(size_t size, int align)
{
	size_t headersize = sizeof(ThreadsafeLinearAllocatorHeader);
#if TLA_DEBUG_STACK_LEAK
	headersize += sizeof(int);
#endif
	return size + headersize + align - 1;
}


ThreadsafeLinearAllocator::ThreadsafeLinearAllocator(int blockSize, int maxBlocksCount, const char* name)
	: BaseAllocator(name)
	, m_CurrentBlock(-1)
	, m_UsedBlocks(0)
	, m_OverflowAllocationsCount(0)
	, m_BlockSize(blockSize)
	, m_MaxBlocksCount(maxBlocksCount)
	, m_CurrentFrameIndex(0)
{
	Assert(blockSize > 0);
	Assert(maxBlocksCount > 0 && maxBlocksCount < 256);
	memset((void*)m_FrameAllocationCount, 0, m_MaxAllocationFramespan*sizeof(int));

	m_Blocks = static_cast<ThreadsafeLinearAllocatorBlock*>(GetMemoryManager().LowLevelAllocate(sizeof(ThreadsafeLinearAllocatorBlock) * m_MaxBlocksCount));
	SelectFreeBlock();
}

ThreadsafeLinearAllocator::~ThreadsafeLinearAllocator()
{
	Mutex::AutoLock lock(m_NewBlockMutex);

	Assert(m_OverflowAllocationsCount == 0);
	for (int i = 0; i < m_UsedBlocks; ++i)
	{
		Assert(m_Blocks[i].allocationCount == 0);
		GetMemoryManager().LowLevelFree(m_Blocks[i].ptr, m_BlockSize);
	}
	m_UsedBlocks = 0;

	GetMemoryManager().LowLevelFree(m_Blocks, sizeof(ThreadsafeLinearAllocatorBlock) * m_MaxBlocksCount);
}

void* ThreadsafeLinearAllocator::Allocate(size_t size, int align)
{
	size_t allocSize = ThreadsafeLinearAllocatorHeader::CalculateNeededAllocationSize(size, align);

	void* realPtr = NULL;
	int realBlockIndex = -1;
	if (allocSize < static_cast<size_t>(m_BlockSize))
	{
		for (;;)
		{
			const int blockIndex = AtomicAdd(&m_CurrentBlock, 0);

			// Early out in case we used all of our memory
			if (blockIndex == -1)
				break;

			// Here we mark block used to prevent it from being grabbed during used memory increment
			AtomicAdd(&m_Blocks[blockIndex].allocationCount, 1);

			// Grab required memory from the current block
			const int newUsedSize = AtomicAdd(&m_Blocks[blockIndex].usedSize, static_cast<int>(allocSize));
			if (newUsedSize <= m_BlockSize)
			{
				realPtr = m_Blocks[blockIndex].ptr + newUsedSize - static_cast<int>(allocSize);
				realBlockIndex = blockIndex;

				break;
			}
			else
			{
				// Yes, if there is not enough memory, block is wasted
				PROFILER_AUTO(gTempJobAllocGrow, NULL);

#if TLA_DEBUG_STACK_LEAK
				// Already added on above. Need to subtract to run PrintAllocations() correctly. 
				AtomicSub(&m_Blocks[blockIndex].usedSize, static_cast<int>(allocSize));
#endif
				// "Release" block
				AtomicSub(&m_Blocks[blockIndex].allocationCount, 1);

				// Lock to prevent greedy blocks allocations and having multiple blocks with 1 allocation.
				Mutex::AutoLock lock(m_NewBlockMutex);
				// Double check if some thread has already switched to the new block
				if (blockIndex != AtomicAdd(&m_CurrentBlock, 0))
					continue;

				// Switch to the next free block.
				if (!SelectFreeBlock())
				{
					// Set overflow mark
					if (AtomicCompareExchange(&m_CurrentBlock, -1, blockIndex))
						break;
				}
			}
		}
	}

	// Overflow. Fallback to kMemTempOverflow allocator
	if (realPtr == NULL)
	{
		PROFILER_AUTO(gTempJobAllocOverflow, NULL);

		AtomicAdd(&m_OverflowAllocationsCount, 1);
		realPtr = UNITY_MALLOC(kMemTempOverflow, allocSize);

		// No luck
		if (realPtr == NULL)
			return NULL;
	}

	// Setup our own microheader to be able to distinguish overflow allocations
	int frameIndex = m_CurrentFrameIndex;
	ThreadsafeLinearAllocatorHeader* header = ThreadsafeLinearAllocatorHeader::SetupHeader(realPtr, size, align, frameIndex);
	AtomicAdd(&m_FrameAllocationCount[frameIndex], 1);
#if TLA_DEBUG_STACK_LEAK
	header->nextPtr = (char*)realPtr + allocSize;
	GetStacktrace(header->callstack, 20, 5);
#endif
	if (realBlockIndex != -1)
		header->blockIndex = realBlockIndex;
	else
		header->isOverflowAlloc = 1;

	return header->GetUserPtr();
}

void* ThreadsafeLinearAllocator::Reallocate(void* p, size_t size, int align)
{
	// p MUST be allocated by ThreadsafeLinearAllocator!
	ThreadsafeLinearAllocatorHeader* header = ThreadsafeLinearAllocatorHeader::GetHeader(p);
	if (header->size >= size && AlignPtr(p, align) == p)
		return p;

	void* newPtr = Allocate(size, align);
	if (newPtr == NULL)
		return NULL;

	UNITY_MEMCPY(newPtr, p, std::min<size_t>(header->size, size));
	Deallocate(p);

	return newPtr;
}

void ThreadsafeLinearAllocator::Deallocate(void* p)
{
	// p MUST be allocated by ThreadsafeLinearAllocator!
	ThreadsafeLinearAllocatorHeader* header = ThreadsafeLinearAllocatorHeader::GetHeader(p);
	if(header->magic != ThreadsafeLinearAllocatorHeader::kMagic)
	{
		ErrorStringMsg("Invalid memory pointer was detected in ThreadsafeLinearAllocator::Deallocate!");
		return;
	}
	header->magic = 0xDCDCDCD;
	AtomicSub(&m_FrameAllocationCount[header->frameIndex], 1);

	if (header->isOverflowAlloc)
	{
		UNITY_FREE(kMemTempOverflow, header->GetRealPtr());
		AtomicSub(&m_OverflowAllocationsCount, 1);
		return;
	}

	const size_t blockIndex = header->blockIndex;
	Assert(p >= m_Blocks[blockIndex].ptr && p < m_Blocks[blockIndex].ptr + m_BlockSize);

	// Just decrease the counter...
	int allocCount = AtomicSub(&m_Blocks[blockIndex].allocationCount, 1);
	Assert(allocCount >= 0);

	// Check if we can make block current in case of overflow
	if (allocCount == 0)
	{
		const int currentBlockIndex = AtomicAdd(&m_CurrentBlock, 0);
		if (currentBlockIndex == -1)
		{
			Mutex::AutoLock lock(m_NewBlockMutex);
			if (currentBlockIndex == AtomicAdd(&m_CurrentBlock, 0))
			{
				// Make block current
				m_Blocks[blockIndex].usedSize = 0;
				AtomicExchange(&m_CurrentBlock, blockIndex);
			}
		}
	}
}

bool ThreadsafeLinearAllocator::Contains (const void* p) const
{
	int usedBlocks = AtomicAdd(&m_UsedBlocks, 0);
	for (int i = 0; i < usedBlocks; ++i)
	{
		if (p >= m_Blocks[i].ptr && p < m_Blocks[i].ptr + m_BlockSize)
			return true;
	}

	if (AtomicAdd(&m_OverflowAllocationsCount, 0) > 0)
		return GetMemoryManager().GetAllocator(kMemTempOverflow)->Contains(p);

	return false;
}

size_t ThreadsafeLinearAllocator::GetPtrSize(const void* p) const
{
	// p MUST be allocated by ThreadsafeLinearAllocator!
	ThreadsafeLinearAllocatorHeader* header = ThreadsafeLinearAllocatorHeader::GetHeader(const_cast<void*>(p));
	return header->size;
}

size_t ThreadsafeLinearAllocator::GetAllocatedMemorySize() const
{
	size_t usedBytes = 0;
	int usedBlocks = AtomicAdd(&m_UsedBlocks, 0);
	for (int i = 0; i < usedBlocks; ++i)
	{
		if (AtomicAdd(&m_Blocks[i].allocationCount, 0) > 0)
		{
			// Approximate value
			usedBytes += AtomicAdd(&m_Blocks[i].usedSize, 0);
		}
	}

	return usedBytes;
}

size_t ThreadsafeLinearAllocator::GetReservedMemorySize() const
{
	int usedBlocks = AtomicAdd(&m_UsedBlocks, 0);
	return usedBlocks * m_BlockSize;
}

bool ThreadsafeLinearAllocator::SelectFreeBlock()
{
	// Do we have some free blocks?
	int usedBlocks = m_UsedBlocks;
	for (int i = 0; i < usedBlocks; ++i)
	{
		if (i != m_CurrentBlock && AtomicAdd(&m_Blocks[i].allocationCount, 0) == 0)
		{
			m_Blocks[i].usedSize = 0;
			AtomicExchange(&m_CurrentBlock, i);
			return true;
		}
	}

	// Allocate new one
	if (usedBlocks >= m_MaxBlocksCount)
		return false;

	void* ptr = GetMemoryManager().LowLevelAllocate(m_BlockSize);
	if (ptr == NULL)
		return false;

	m_Blocks[usedBlocks].ptr = static_cast<UInt8*>(ptr);
	m_Blocks[usedBlocks].allocationCount = 0;
	m_Blocks[usedBlocks].usedSize = 0;

	AtomicAdd(&m_UsedBlocks, 1);
	AtomicExchange(&m_CurrentBlock, usedBlocks);

	return true;
}

void ThreadsafeLinearAllocator::PrintAllocations(int frameIndex)
{
#if TLA_DEBUG_STACK_LEAK
	for(int i = 0; i < m_UsedBlocks; i++)
	{
		if(m_Blocks[i].allocationCount == 0)
			continue;
		void* ptr = m_Blocks[i].ptr;
		while(ptr && ptr < m_Blocks[i].ptr + m_Blocks[i].usedSize - sizeof(ThreadsafeLinearAllocatorHeader))
		{
			ThreadsafeLinearAllocatorHeader* header = ThreadsafeLinearAllocatorHeader::GetHeaderFromRealPtr(ptr);
			ptr = header->nextPtr;
			if(header->magic == ThreadsafeLinearAllocatorHeader::kMagic && (frameIndex == -1 || header->frameIndex == frameIndex))
			{
				char buf[4096];
				FormatBuffer(buf, 4096, "Allocation of %i bytes at %08x", header->size, ptr);
				size_t len = strlen(buf);
				buf[len++] = '\n';
				GetReadableStackTrace(buf + len, 4096 - len, header->callstack, 20);
				LogString(buf);
			}
		}
	}
#endif
}

void ThreadsafeLinearAllocator::FrameMaintenance(bool cleanup)
{
	int oldFrameIndex = m_CurrentFrameIndex == 0 ? m_MaxAllocationFramespan-1 : m_CurrentFrameIndex - 1;
	int allocationCountForOldFrame = m_FrameAllocationCount[oldFrameIndex];
	m_FrameAllocationCount[oldFrameIndex] = 0;
	// negative values can happen, if a leak has been reported earlier
	//AssertMsg(allocationCountForOldFrame <= 0, "Threadsafe Linear allocator (Job temp alloc) has allocations that are more that 2 frames old - this is not allowed and likely a leak");
	if(allocationCountForOldFrame > 0)
		PrintAllocations(oldFrameIndex);

	m_CurrentFrameIndex = (m_CurrentFrameIndex +1) % m_MaxAllocationFramespan;

	if(cleanup)
	{
		for(int i = 0; i < m_UsedBlocks; i++)
		{
			if(m_Blocks[i].allocationCount == 0)
				continue;
			ErrorStringMsg("There are remaining Allocations on the JobTempAlloc. This is a leak, and will impact performance");
			PrintAllocations(-1);
			return;
		}
	}
}
