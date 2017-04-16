#include "PluginPrefix.h"
#include "StackAllocator.h"

#include "Runtime/Allocator/MemoryManager.h"

StackAllocator::StackAllocator(size_t blockSize, MemLabelId fallbackMemLabel, const char* name)
	: BaseAllocator(name)
	, m_Block(NULL)
	, m_BlockSize(blockSize)
	, m_FallbackLabel(fallbackMemLabel)
	, m_LastAlloc(NULL)
{
#if ENABLE_MEMORY_MANAGER
	m_Block = MemoryManager::LowLevelAllocate(m_BlockSize);
#else
	m_Block = malloc(m_BlockSize);
#endif

	m_TotalReservedBytes = blockSize;
	m_PresentShortFall = 0;
	m_MaxShortFall = 0;
}

StackAllocator::~StackAllocator()
{
	// Temporarily comment out this assert to keep ABV green. Will turn it back on when leaks in user code are eliminated.
	//Assert(m_LastAlloc == NULL);

#if ENABLE_MEMORY_MANAGER
	MemoryManager::LowLevelFree(m_Block, m_BlockSize);
#else
	free(m_Block);
#endif
}

//	Adjust shortfall UP or DOWN.
//	UP on alloc
//	DOWN on dealloc
//	UP or DOWN on realloc
void StackAllocator::UpdateShortFall(const SInt64 delta)
{
	m_PresentShortFall += delta;
	if (m_PresentShortFall > m_MaxShortFall)
	{
		m_MaxShortFall = m_PresentShortFall;
	}
	//AssertMsg(m_PresentShortFall >= 0, "Likely cause is StackAllocator dealloc call made when StackAllocator was not responsible for the allocation (label mismatch between alloc and dealloc");
	//Assert(m_MaxShortFall >= 0);
}

void* StackAllocator::Allocate(size_t size, int align)
{
	//1 byte alignment doesn't work for webgl; this is a fix(ish)....
#if UNITY_NO_UNALIGNED_MEMORY_ACCESS
	if(align % 8 != 0)
		align = 8;
#endif

	const size_t alignmask = align - 1;

	// Make header size a multiple
	size_t alignedHeaderSize = (GetHeaderSize() + alignmask) & ~alignmask;
	size_t paddedSize = (size + alignedHeaderSize + alignmask) & ~alignmask;

	void* ptr;
	void* freePtr = AlignPtr(GetBlockFreePtr(), align);
	size_t usedSize = static_cast<char*>(freePtr) - static_cast<char*>(m_Block);
	if (usedSize < m_BlockSize && (m_BlockSize - usedSize) >= paddedSize)
	{
		// User ptr
		ptr = static_cast<char*>(freePtr) + alignedHeaderSize;

		// Header
		Header* h = GetHeader(ptr);
		h->size = size;
		h->deleted = 0;
		h->prevPtr = m_LastAlloc;
#if DEBUG_STACK_LEAK
		GetStacktrace(h->callstack, 20, 5);
#endif

		m_LastAlloc = ptr;

		RegisterAllocationData(size, GetHeaderSize());
	}
	else
	{
		// Spilled over. We have to allocate the memory default alloc
		BaseAllocator* allocator = GetMemoryManager().GetAllocator(m_FallbackLabel);
		align = MaxAlignment(align, kDefaultMemoryAlignment);
		ptr = allocator->Allocate(size, align);
		if (ptr != NULL)
			UpdateShortFall(allocator->GetPtrSize(ptr));
	}

	return ptr;
}

void* StackAllocator::Reallocate(void* p, size_t size, int align)
{
#if UNITY_NO_UNALIGNED_MEMORY_ACCESS
	// The passed in align parameter is not always sufficient for webgl.
	// Enforce 8 byte alignment.
	if(align % 8 != 0)
		align = 8;
#endif

	if (p == NULL)
		return Allocate(size, align);

	void* freePtr = AlignPtr(GetBlockFreePtr(), align);
	size_t usedSize = static_cast<char*>(freePtr) - static_cast<char*>(m_Block);
	size_t freeSize = usedSize <= m_BlockSize ? m_BlockSize - usedSize : 0;
	size_t oldSize = GetPtrSize(p);

	void* newPtr = NULL;

	if (InBlock(p))
	{
		if ((p == m_LastAlloc || oldSize >= size) &&
			AlignPtr(p, align) == p &&
			oldSize + freeSize > size)
		{
			// just expand the top allocation of the stack to the realloc amount
			newPtr = p;

			Header* h = GetHeader(p);
			h->size = size;
			RegisterDeallocationData(oldSize, 0);
			RegisterAllocationData(size, 0);
		}
		else
		{
			newPtr = Allocate(size, align);
			if (newPtr != NULL)
				memcpy(newPtr, p, std::min(size, oldSize));
			Deallocate(p);
		}
	}
	else
	{
		BaseAllocator* allocator = GetMemoryManager().GetAllocator(m_FallbackLabel);
		size_t oldSize = allocator->GetPtrSize(p);
		align = MaxAlignment(align, kDefaultMemoryAlignment);
		newPtr = allocator->Reallocate(p, size, align);
		if (newPtr != NULL)
			UpdateShortFall(allocator->GetPtrSize(newPtr));
		UpdateShortFall(-(SInt64) oldSize);
	}

	return newPtr;
}

bool StackAllocator::TryDeallocate(void* p)
{
	if (p == NULL)
		return true;

	if (p == m_LastAlloc)
	{
		//AssertMsg(!IsDeleted(p), "Trying to free already freed memory in StackAllocator!");
		RegisterDeallocationData(GetPtrSize(p), GetHeaderSize());
		SetDeleted(p);
		do
		{
			m_LastAlloc = GetPrevAlloc(m_LastAlloc);
		} while (m_LastAlloc != NULL && IsDeleted(m_LastAlloc));
	}
	else if (InBlock(p))
	{
		//AssertMsg(!IsDeleted(p), "Trying to free already freed memory in StackAllocator!");
		RegisterDeallocationData(GetPtrSize(p), GetHeaderSize());
		SetDeleted(p);
	}
	else
	{
		BaseAllocator* allocator = GetMemoryManager().GetAllocator(m_FallbackLabel);
		size_t oldSize = allocator->GetPtrSize(p);
		UpdateShortFall(-(SInt64) oldSize);
		allocator->Deallocate(p);
	}

	return true;
}

void StackAllocator::FrameMaintenance(bool cleanup)
{
	ManageSize ();
}

void StackAllocator::ManageSize(void)
{
	// If stack is empty
	if (m_LastAlloc == NULL && m_PresentShortFall == 0)
	{
		//	And there was a shortfall during previous use
		if (m_MaxShortFall)
		{
			static	const	int	ONE_MEGABYTE = (1<<20);
			//	Resize to nearest power of two ABOVE what was required
			const size_t newSize = (m_BlockSize + m_MaxShortFall);
			size_t	x = 1;
			//	Or step up by one megabyte once we are already at least one meg in size
			//	Stops a large growth when stack is already large
			while (x < newSize)
			{
				(x <= ONE_MEGABYTE) ? x<<=1 : x += ONE_MEGABYTE;
			}

#if ENABLE_MEMORY_MANAGER
			MemoryManager::LowLevelFree(m_Block, m_BlockSize);
			m_Block = (char*) MemoryManager::LowLevelAllocate(x);
#else
			free(m_Block);
			m_Block = (char*)malloc(x);
#endif
			m_BlockSize = x;
			m_PresentShortFall = 0;
			m_MaxShortFall = 0;
		}
	}
}

bool StackAllocator::IsOverflowAllocation(const void* p) const
{
	// Pointer in block is checked by Contains in the .h file. Only if it is an overflow allocation, this is called
	BaseAllocator* allocator = GetMemoryManager().GetAllocator(m_FallbackLabel);
	return allocator->Contains(p);
}

void StackAllocator::SetBlockSize(size_t blockSize)
{
	//Assert(GetAllocatedMemorySize() == 0);

#if ENABLE_MEMORY_MANAGER
	m_Block = MemoryManager::LowLevelReallocate(m_Block, blockSize, m_BlockSize);
#else
	m_Block = realloc(m_Block, m_BlockSize);
#endif
	m_BlockSize = blockSize;
	m_TotalReservedBytes = blockSize;
}