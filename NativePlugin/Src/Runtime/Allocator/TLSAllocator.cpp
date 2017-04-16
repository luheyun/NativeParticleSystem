#include "UnityPrefix.h"
#include "TLSAllocator.h"
#include "Runtime/Allocator/StackAllocator.h"
#include "Runtime/Allocator/MemoryManager.h"
#include "Runtime/Threads/Mutex.h"

#if ENABLE_MEMORY_MANAGER

template <class UnderlyingAllocator>
int TLSAllocator<UnderlyingAllocator>::s_NumberOfInstances = 0;

template <class UnderlyingAllocator>
UNITY_TLS_VALUE(UnderlyingAllocator*) TLSAllocator<UnderlyingAllocator>::m_UniqueThreadAllocator;

template <class UnderlyingAllocator>
TLSAllocator<UnderlyingAllocator>::TLSAllocator(const char* name)
: BaseAllocator(name)
{
	if(s_NumberOfInstances != 0)
		ErrorString("Only one instance of the TLS allocator is allowed because of TLS implementation");
	s_NumberOfInstances++;
	memset (m_ThreadTempAllocators, 0, sizeof(m_ThreadTempAllocators));
}

template <class UnderlyingAllocator>
TLSAllocator<UnderlyingAllocator>::~TLSAllocator()
{
	s_NumberOfInstances--;
}

Mutex g_AllocatorTableLock;
template <class UnderlyingAllocator>
void TLSAllocator<UnderlyingAllocator>::ThreadInitialize(BaseAllocator *allocator)
{
	m_UniqueThreadAllocator = (UnderlyingAllocator*)allocator;

	Mutex::AutoLock lock(g_AllocatorTableLock);
	for(int i = 0; i < kMaxThreadTempAllocators; i++)
	{
		if(m_ThreadTempAllocators[i] == NULL)
		{
			m_ThreadTempAllocators[i] = (UnderlyingAllocator*) allocator;
			break;
		}
	}

}

template <class UnderlyingAllocator>
void TLSAllocator<UnderlyingAllocator>::ThreadCleanup()
{
	UnderlyingAllocator* allocator = m_UniqueThreadAllocator;
	m_UniqueThreadAllocator = NULL;
	
	Mutex::AutoLock lock(g_AllocatorTableLock);
	for(int i = 0; i < kMaxThreadTempAllocators; i++)
	{
		if(m_ThreadTempAllocators[i] == allocator)
		{
			m_ThreadTempAllocators[i] = NULL;
			break;
		}
	}
	UNITY_DELETE(allocator, kMemManager);
}

template <class UnderlyingAllocator>
void TLSAllocator<UnderlyingAllocator>::FrameMaintenance(bool cleanup)
{
	UnderlyingAllocator* alloc = GetCurrentAllocator();
	if (alloc)
	{
#if DEBUGMODE
		if (alloc->GetAllocatedMemorySize() > 0)
		{
#if USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER
			WalkAllocations(BaseAllocator::LogAllocationInfo);
#endif
			AssertFormatMsg(false, "TLS Allocator %s, underlying allocator %s has unfreed allocations", GetName(), alloc->GetName());
		}
#endif
		alloc->FrameMaintenance(cleanup);
	}
}

template <class UnderlyingAllocator>
bool TLSAllocator<UnderlyingAllocator>::IsAssigned() const
{
	return m_UniqueThreadAllocator != NULL;
}

template <class UnderlyingAllocator>
UnderlyingAllocator* TLSAllocator<UnderlyingAllocator>::GetCurrentAllocator() const
{
	return m_UniqueThreadAllocator;
}

template <class UnderlyingAllocator>
void* TLSAllocator<UnderlyingAllocator>::Allocate( size_t size, int align )
{
	UnderlyingAllocator* alloc = GetCurrentAllocator();
	if (!alloc)
		return NULL;

	return alloc->UnderlyingAllocator::Allocate(size, align);
}

template <class UnderlyingAllocator>
void* TLSAllocator<UnderlyingAllocator>::Reallocate( void* p, size_t size, int align )
{
	UnderlyingAllocator* alloc = GetCurrentAllocator();
	if(!alloc)
		return NULL;

	return alloc->UnderlyingAllocator::Reallocate(p, size, align);
}

template <class UnderlyingAllocator>
void TLSAllocator<UnderlyingAllocator>::Deallocate( void* p )
{
	UnderlyingAllocator* alloc = GetCurrentAllocator();
	DebugAssert(alloc);
	return alloc->UnderlyingAllocator::Deallocate(p);
}

template <class UnderlyingAllocator>
bool TLSAllocator<UnderlyingAllocator>::TryDeallocate(void* p)
{
	UnderlyingAllocator* alloc = GetCurrentAllocator();
	if (!alloc)
		return false;

	return alloc->UnderlyingAllocator::TryDeallocate(p);
}

template <class UnderlyingAllocator>
bool TLSAllocator<UnderlyingAllocator>::Contains( const void* p ) const
{
	UnderlyingAllocator* alloc = GetCurrentAllocator();
	if(alloc && alloc->UnderlyingAllocator::Contains(p))
		return true;
	return false;
}

template <class UnderlyingAllocator>
size_t TLSAllocator<UnderlyingAllocator>::GetAllocatedMemorySize( ) const
{
	size_t allocated = 0;
	for(int i = 0; i < kMaxThreadTempAllocators; i++)
	{
		if(m_ThreadTempAllocators[i] != NULL)
			allocated += m_ThreadTempAllocators[i]->UnderlyingAllocator::GetAllocatedMemorySize();
	}
	return allocated;
}

template <class UnderlyingAllocator>
size_t TLSAllocator<UnderlyingAllocator>::GetBookKeepingMemorySize() const
{
	size_t total = 0;
	for(int i = 0; i < kMaxThreadTempAllocators; i++)
	{
		if(m_ThreadTempAllocators[i] != NULL)
			total += m_ThreadTempAllocators[i]->UnderlyingAllocator::GetBookKeepingMemorySize();
	}
	return total;
}

template <class UnderlyingAllocator>
size_t TLSAllocator<UnderlyingAllocator>::GetReservedMemorySize() const
{
	size_t total = 0;
	for(int i = 0; i < kMaxThreadTempAllocators; i++)
	{
		if(m_ThreadTempAllocators[i] != NULL)
			total += m_ThreadTempAllocators[i]->UnderlyingAllocator::GetReservedMemorySize();
	}
	return total;
}

template <class UnderlyingAllocator>
size_t TLSAllocator<UnderlyingAllocator>::GetPtrSize( const void* ptr ) const
{
	// all allocators have the same allocation header
	return m_ThreadTempAllocators[0]->UnderlyingAllocator::GetPtrSize(ptr);
}

template <class UnderlyingAllocator>
bool TLSAllocator<UnderlyingAllocator>::CheckIntegrity()
{
	bool succes = true;
	for(int i = 0; i < kMaxThreadTempAllocators; i++)
	{
		if(m_ThreadTempAllocators[i] != NULL)
			succes &= m_ThreadTempAllocators[i]->UnderlyingAllocator::CheckIntegrity();
	}
	return succes;
}

#if USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER

template<class UnderlyingAllocator>
ProfilerAllocationHeader* TLSAllocator<UnderlyingAllocator>::GetProfilerHeader(const void* ptr) const
{
	// all allocators have the same allocation header
	if (m_ThreadTempAllocators[0] != NULL)
		return m_ThreadTempAllocators[0]->UnderlyingAllocator::GetProfilerHeader(ptr);

	// If TLSAllocator is not setup, we fallback to default one
	return GetMemoryManager().GetAllocator(kMemDefault)->GetProfilerHeader(ptr);
}

template<class UnderlyingAllocator>
size_t TLSAllocator<UnderlyingAllocator>::GetRequestedPtrSize(const void* ptr) const
{
	// all allocators have the same allocation header
	if (m_ThreadTempAllocators[0] != NULL)
		return m_ThreadTempAllocators[0]->UnderlyingAllocator::GetRequestedPtrSize(ptr);

	// If TLSAllocator is not setup, we fallback to default one
	return GetMemoryManager().GetAllocator(kMemDefault)->GetRequestedPtrSize(ptr);
}

template<class UnderlyingAllocator>
void TLSAllocator<UnderlyingAllocator>::WalkAllocations(BaseAllocator::WalkAllocationsCallback callback) const
{
	if (m_ThreadTempAllocators[0] != NULL)
		m_ThreadTempAllocators[0]->UnderlyingAllocator::WalkAllocations(callback);
}

#endif // USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER

template class TLSAllocator< StackAllocator >;

#endif // #if ENABLE_MEMORY_MANAGER
