#ifndef TLS_ALLOCATOR_H_
#define TLS_ALLOCATOR_H_

#if ENABLE_MEMORY_MANAGER

#include "Runtime/Allocator/BaseAllocator.h"
#include "Runtime/Threads/ThreadSpecificValue.h"

/************************************************************************/
/* TLS Allocator is an indirection to a real allocator                  */
/* Has a tls value pointing to an instance of UnderlyingAllocator that  */
/* is unique per thread.                                                */
/************************************************************************/

template <class UnderlyingAllocator>
class TLSAllocator : public BaseAllocator
{
public:
	// when constructing it will be from the main thread
	TLSAllocator(const char* name);
	virtual ~TLSAllocator();

	virtual void*  Allocate(size_t size, int align);
	virtual void*  Reallocate (void* p, size_t size, int align);
	virtual void   Deallocate (void* p);
	virtual bool   TryDeallocate (void* p);
	virtual bool   Contains (const void* p) const;
	virtual size_t GetPtrSize(const void* ptr) const;
	virtual bool   IsAssigned() const;
	virtual bool   CheckIntegrity();

	virtual size_t GetAllocatedMemorySize() const;
	virtual size_t GetBookKeepingMemorySize() const;
	virtual size_t GetReservedMemorySize() const;

#if USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER
	virtual ProfilerAllocationHeader* GetProfilerHeader(const void* ptr) const;
	virtual size_t GetRequestedPtrSize(const void* ptr) const;
	virtual void WalkAllocations(WalkAllocationsCallback callback) const;
#endif // USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER

	virtual void ThreadInitialize(BaseAllocator* allocator);
	virtual void ThreadCleanup();
	virtual void FrameMaintenance(bool cleanup);

	UnderlyingAllocator* GetCurrentAllocator() const;

private:
	// because TLS values have to be static on some platforms, this is made static
	// and only one instance of the TLS is allowed
	static UNITY_TLS_VALUE(UnderlyingAllocator*) m_UniqueThreadAllocator; // the memorymanager holds the list of allocators
	static int s_NumberOfInstances;

	static const int      kMaxThreadTempAllocators = 128;
	UnderlyingAllocator*  m_ThreadTempAllocators[kMaxThreadTempAllocators];
};

#endif
#endif
