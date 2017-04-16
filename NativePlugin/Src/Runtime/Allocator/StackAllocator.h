#ifndef STACK_ALLOCATOR_H_
#define STACK_ALLOCATOR_H_

#include "Runtime/Allocator/BaseAllocator.h"
#include "Runtime/Utilities/dense_hash_map.h"

/***************************************************************************/
/* The StackAllocator is the allocator we use for temp allocations.        */
/* It has a fixed buffer, and whenever we want some memory, it increments  */
/* the pointer. When an allocation is deleted, it checks if it is on the   */
/* top of the stack, and then counts the stack down, if not it will set a  */
/* bit in the header of the allocation and do nothing more. When an        */
/* allocation from the top is deleted, it will unwind until it gets to an  */
/* allocation without the bit set. If this allocator overflows, it falls   */
/* back to the default main allocator and maintains a linked list of these */
/* allocations. Still uses the same scheme for unwinding.                  */
/***************************************************************************/
#define DEBUG_STACK_LEAK 0

class StackAllocator : public BaseAllocator
{
public:
	// Constructs new instance of StackAllocator
	// blocksize: Initial size of the memory block we use for linear allocations.
	// fallbackMemLabel: Memory label for allocation after we used preallocated block.
	// name: Allocator name.
	StackAllocator(size_t blocksize, MemLabelId fallbackMemLabel, const char* name);
	virtual ~StackAllocator();

	virtual void*  Allocate(size_t size, int align);
	virtual void*  Reallocate(void* p, size_t size, int align);
	virtual void   Deallocate(void* p) { TryDeallocate(p); }
	virtual bool   TryDeallocate(void* p);
	virtual bool   Contains(const void* p) const;
	virtual size_t GetPtrSize(const void* ptr) const;
	virtual void   FrameMaintenance(bool cleanup);

#if USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER
	virtual ProfilerAllocationHeader* GetProfilerHeader(const void* ptr) const { return NULL; }
	virtual size_t GetRequestedPtrSize(const void* ptr) const { return GetPtrSize(ptr); }
	virtual void WalkAllocations(BaseAllocator::WalkAllocationsCallback callback) const;
#endif // USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER

	void SetBlockSize(size_t blockSize);

private:
	/// Header for the allocation on stack block
	struct Header
	{
		void*   prevPtr;
		UInt32  deleted : 1;
		UInt32  size : 31;
#if DEBUG_STACK_LEAK
		void*   callstack[20];
#endif
	};

	/// Continuous memory block for the StackAllocator
	void*  m_Block;
	size_t m_BlockSize;
	MemLabelId m_FallbackLabel;

	/// Stack head of allocations that belongs to the m_Block
	void*  m_LastAlloc;

	SInt64 m_PresentShortFall;
	SInt64 m_MaxShortFall;

	bool   InBlock(const void* ptr) const;
	void*  GetBlockFreePtr() const;
	bool   IsOverflowAllocation(const void* p) const;
	void   UpdateShortFall(const SInt64 delta);
	void   ManageSize(void);

	static inline bool          IsDeleted(const void* ptr);
	static inline void          SetDeleted(const void* ptr);
	static inline void*         GetPrevAlloc(const void* ptr);
	static inline Header*       GetHeader(void* ptr);
	static inline const Header* GetHeader(const void* ptr);
	static inline size_t        GetHeaderSize();
};

inline bool StackAllocator::Contains (const void* p) const
{
	// most common case. pointer being queried is the one about to be destroyed
	if (p != NULL && p == m_LastAlloc)
		return true;

	// Check if temp allocation
	if (m_LastAlloc != NULL && InBlock(p))
		return true;

	// Check large allocation
	return IsOverflowAllocation(p);
}

inline void* StackAllocator::GetBlockFreePtr() const
{
	if (m_LastAlloc == NULL)
		return m_Block;

	return static_cast<char*>(m_LastAlloc) + GetHeader(m_LastAlloc)->size;
}

inline size_t StackAllocator::GetPtrSize(const void* ptr) const
{
	return GetHeader(ptr)->size;
}

inline void* StackAllocator::GetPrevAlloc(const void* ptr)
{
	return GetHeader(ptr)->prevPtr;
}

inline StackAllocator::Header* StackAllocator::GetHeader(void* ptr)
{
	return static_cast<Header*>(ptr) -1;
}

inline const StackAllocator::Header* StackAllocator::GetHeader(const void* ptr)
{
	return static_cast<const Header*>(ptr) -1;
}

inline size_t StackAllocator::GetHeaderSize()
{
	return sizeof(Header);
}

inline bool StackAllocator::InBlock(const void* ptr) const
{
	return ptr >= m_Block && ptr < (static_cast<char*>(m_Block) + m_BlockSize);
}

inline bool StackAllocator::IsDeleted(const void* ptr)
{
	return GetHeader(ptr)->deleted != 0;
}

inline void StackAllocator::SetDeleted(const void* ptr)
{
	Header* h = const_cast<Header*>(GetHeader(ptr));
	h->deleted = 1;
}


#endif
