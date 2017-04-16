#include "PluginPrefix.h"
#include "BaseAllocator.h"
#include "Threads/AtomicOps.h"

static volatile int g_IncrementIdentifier = 0x10;

BaseAllocator::BaseAllocator(const char* name)
: m_Name(name)
, m_NumAllocations(0)
, m_TotalAllocatedBytes(0)
, m_TotalReservedBytes(0)
, m_PeakAllocatedBytes(0)
, m_BookKeepingMemoryUsage(0)
{
	m_AllocatorIdentifier = static_cast<UInt32>(AtomicIncrement(&g_IncrementIdentifier));
}

bool BaseAllocator::TryDeallocate(void* p)
{
	if (!Contains(p))
		return false;

	Deallocate(p);
	return true;
}

#if USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER
void BaseAllocator::WalkAllocations(WalkAllocationsCallback) const
{ 
	DebugAssertMsg(false, "WalkAllocations not implemented by this allocator."); 
}

void BaseAllocator::LogAllocationInfo(const void* ptr, size_t size, void* const* callstack, size_t maxCallstackSize)
{
	char buf[4096];
	FormatBuffer(buf, 4096, "Allocation of %i bytes at %08x", size, ptr);
	
#if STACKTRACE_IMPLEMENTED
	if (callstack != NULL && maxCallstackSize > 0)
	{
		size_t len = strlen(buf);
		buf[len++] = '\n';
		GetReadableStackTrace(buf + len, 4096 - len, callstack, maxCallstackSize);
	}
#endif
	
	LogString(buf);
}
#endif
