#pragma once

#ifndef THREADSAFE_LINEAR_ALLOCATOR_H_
#define THREADSAFE_LINEAR_ALLOCATOR_H_

#include "UnityPrefix.h"
#include "Runtime/Allocator/BaseAllocator.h"
#include "Runtime/Threads/Mutex.h"
#include "Runtime/Utilities/BitUtility.h"

struct ThreadsafeLinearAllocatorBlock;

// Lockless allocator that can be used for "sliding window" allocations.
// It uses preallocated blocks of memory and rotates them once current block is used.
// On allocation we increment used size and allocations count for the current block, and if the block
// is full we switch to the next empty block.
// On deallocation we just decrease allocations count and if it is 0 the block can be reused.
// Ideally depending on the scene and block size there should be 2-3 rotating blocks.
class ThreadsafeLinearAllocator : public BaseAllocator
{
public:
	// Construct ThreadsafeLinearAllocator.
	// @param blockSize Fixed block size.
	// @param maxBlocksCount Maximum blocks count (<= 255).
	// @param blockMemLabel Memory label for allocating blocks' memory.
	// @param name Allocator name (fore debugging purposes).
	ThreadsafeLinearAllocator(int blockSize, int maxBlocksCount, const char* name);
	virtual ~ThreadsafeLinearAllocator();

	virtual void*  Allocate (size_t size, int align);
	virtual void*  Reallocate(void* p, size_t size, int align);
	virtual bool   TryDeallocate(void* p) { Deallocate(p); return true; }
	virtual void   Deallocate(void* p);
	virtual bool   Contains(const void* p) const;
	virtual size_t GetPtrSize(const void* p) const;
	virtual size_t GetAllocatedMemorySize() const;
	virtual size_t GetReservedMemorySize() const;
	virtual void   FrameMaintenance(bool cleanup);
#if USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER
	virtual ProfilerAllocationHeader* GetProfilerHeader(const void* p) const { return NULL; }
	virtual size_t GetRequestedPtrSize(const void* p) const { return GetPtrSize(p); }
#endif // USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER

private:
	bool SelectFreeBlock();
	void PrintAllocations(int frameIndex);

	ThreadsafeLinearAllocatorBlock*    m_Blocks;
	ALIGN_TYPE(4) volatile int         m_CurrentBlock;
	ALIGN_TYPE(4) mutable volatile int m_UsedBlocks;
	ALIGN_TYPE(4) mutable volatile int m_OverflowAllocationsCount;
	const int   m_BlockSize;
	const int   m_MaxBlocksCount;
	Mutex       m_NewBlockMutex;

	static const int           m_MaxAllocationFramespan = 3;
	int                        m_CurrentFrameIndex;
	ALIGN_TYPE(4) volatile int m_FrameAllocationCount[m_MaxAllocationFramespan];
};

#endif // !THREADSAFE_LINEAR_ALLOCATOR_H_
