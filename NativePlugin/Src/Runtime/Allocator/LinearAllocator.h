#ifndef LINEAR_ALLOCATOR_H_
#define LINEAR_ALLOCATOR_H_

#include <cstddef>
#include <list>
#include "Configuration/UnityConfigure.h"
#if UNITY_LINUX || UNITY_PS4
#include <stdint.h>   // uintptr_t
#endif

#include "Runtime/Allocator/MemoryMacros.h"

struct LinearAllocatorBase
{
	static const int	kMinimalAlign = 4;
	
	struct Block
	{
		char*	m_Begin;
		char*	m_Current;
		size_t	m_Size;
		MemLabelId m_Label;
		
		void initialize (size_t size, MemLabelId label)
		{
			m_Label = label;
			m_Current = m_Begin = (char*)UNITY_MALLOC(label,size);
			m_Size = size;
		}
		
		void reset ()
		{
			m_Current = m_Begin;
		}
		
		void purge ()
		{
			UNITY_FREE(m_Label, m_Begin);
		}
		
		size_t used () const
		{
			return m_Current - m_Begin;
		}
		
		void* current () const
		{
			return m_Current;
		}
		
		size_t available () const
		{
			return m_Size - used ();
		}
		
		size_t padding (size_t alignment) const
		{
			size_t pad = (((uintptr_t)m_Current - 1) | (alignment - 1)) + 1 - (uintptr_t)m_Current;
			return pad;
		}

		void* bump (size_t size)
		{
			char* p = m_Current;
			m_Current += size;
			return p;
		}
		
		void roll_back (size_t size)
		{
			m_Current -= size;
		}
		
		bool belongs (const void* p)
		{
			//if (p >= m_Begin && p <= m_Begin + m_Size)
			//	return true;
			//return false;

			//return p >= m_Begin && p <= m_Begin + m_Size;

			return (uintptr_t)p - (uintptr_t)m_Begin <= (uintptr_t)m_Size;
		}

		void set (void* p)
		{
			m_Current = (char*)p;
		}
	};
	
	typedef std::list<Block, STL_ALLOCATOR(kMemPoolAlloc, Block) >	block_container;
	
	LinearAllocatorBase (size_t blockSize, MemLabelId label)
	:	m_Blocks(), m_BlockSize (blockSize), m_AllocLabel (label)
	{
	}
	
	void add_block (size_t size)
	{
		m_Blocks.push_back (Block());
		size_t blockSize = size > m_BlockSize ? size : m_BlockSize;
		m_Blocks.back ().initialize (blockSize, m_AllocLabel);
	}
	
	void purge (bool releaseAllBlocks = false)
	{
		if (m_Blocks.empty ())
			return;
			
		block_container::iterator begin = m_Blocks.begin ();
		
		if (!releaseAllBlocks)
			begin++;
			
		for (block_container::iterator it = begin, end = m_Blocks.end (); it != end; ++it)
			it->purge ();

		m_Blocks.erase (begin, m_Blocks.end ());

		if (!releaseAllBlocks)
			m_Blocks.back ().reset ();
	}
	
	bool belongs (const void* p)
	{
		for (block_container::iterator it = m_Blocks.begin (), end = m_Blocks.end (); it != end; ++it)
		{
			if (it->belongs (p))
				return true;
		}
		
		return false;
	}

	void* current () const
	{
		return m_Blocks.empty () ? 0 : m_Blocks.back ().current ();
	}

	void rewind (void* mark)
	{
		for (block_container::iterator it = m_Blocks.end (); it != m_Blocks.begin (); --it)
		{
			block_container::iterator tit = it;
			--tit;

			if (tit->belongs (mark)) {
				tit->set (mark);

				for (block_container::iterator temp = it; temp != m_Blocks.end (); ++temp)
					temp->purge ();
				
				m_Blocks.erase (it, m_Blocks.end ());
				break;
			}
		}
	}
	
protected:
	block_container	m_Blocks;
	size_t			m_BlockSize;
	MemLabelId      m_AllocLabel;
};


struct ForwardLinearAllocator : public LinearAllocatorBase
{
	ForwardLinearAllocator (size_t blockSize, MemLabelId label)
	:	LinearAllocatorBase (blockSize, label)
	{
	}
	
	~ForwardLinearAllocator ()
	{
		purge (true);
	}

	size_t GetAllocatedBytes() const
	{
		size_t s = 0;
		for (block_container::const_iterator it = m_Blocks.begin (); it != m_Blocks.end(); ++it)
			s += it->used();
		return s;
	}
	
	void* allocate (size_t size, size_t alignment = 4)
	{
//		Assert (size == AlignUIntPtr (size, kMinimalAlign));
		
		if (m_Blocks.empty ())
			add_block (size);

		Block* block = &m_Blocks.back ();
		size_t padding = block->padding (alignment);
		
		if (size + padding > block->available ()) {
			add_block (size);
			block = &m_Blocks.back ();
		}
		
		uintptr_t p = (uintptr_t)block->bump (size + padding);
		
		return (void*)(p + padding);
	}

	void deallocate (void* dealloc)
	{
	}
	
	void deallocate_no_thread_check (void* dealloc)
	{
	}
	
	void purge (bool releaseAllBlocks = false)
	{
		LinearAllocatorBase::purge (releaseAllBlocks);
	}

	void rewind (void* mark)
	{
		LinearAllocatorBase::rewind (mark);
	}

	using LinearAllocatorBase::current;
	using LinearAllocatorBase::belongs;
};


#endif
