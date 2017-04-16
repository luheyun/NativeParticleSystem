#ifndef UNITY_DEFAULT_ALLOCATOR_H_
#define UNITY_DEFAULT_ALLOCATOR_H_

#if ENABLE_MEMORY_MANAGER

#include "BaseAllocator.h"
#include "Runtime/Threads/Mutex.h"
#include "Runtime/Allocator/LowLevelDefaultAllocator.h"
#include "Runtime/Utilities/BitUtility.h"

/***************************************************************************/
/* This is used as default because it is closest to what people are used   */
/* to. Almost the same behavior as the system malloc and free. It is       */
/* hooked up to the profiler, and has a internal structure, to be able to  */
/* tell if a pointer belongs to this allocator or not, but the allocations */
/* fall through to the system malloc (or the UNITY_LL_ALLOC)               */
/***************************************************************************/


enum RequestType
{
	kRegister,
	kUnregister,
	kTest
};

template<class LLAlloctor>
class UnityDefaultAllocator : public BaseAllocator
{
public:
	UnityDefaultAllocator(const char* name);

	virtual void*  Allocate (size_t size, int align);
	virtual void*  Reallocate (void* p, size_t size, int align);
	virtual void   Deallocate (void* p);
	virtual bool   Contains (const void* p) const;
	virtual size_t GetPtrSize(const void* ptr) const;

#if USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER
	virtual ProfilerAllocationHeader* GetProfilerHeader(const void* ptr) const;
	virtual size_t GetRequestedPtrSize(const void* ptr) const;
	virtual bool   ValidateIntegrity(const void* ptr) const;
#endif // USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER

	static int GetOverheadSize(void* ptr);

private:
	// needs 30 bit (4byte aligned allocs and packed as bitarray) ( 1 byte -> 32Bytes, 4bytes rep 128Bytes(7bit))
	enum
	{
		kTargetBitsRepresentedPerBit = StaticLog2<kDefaultMemoryAlignment>::value,
		kTargetBitsRepresentedPerByte = 5 + kTargetBitsRepresentedPerBit,
		kPage1Bits = 7, // 128*4Bytes = 512Bytes (page: 4GB/128 = 32MB per pointer)
		kPage2Bits = 7, // 128*4Bytes = 512Bytes (page: 32MB/128 = 256KB per pointer)
		kPage3Bits = 5, // 32*4Bytes = 128Bytes (page: 256K/32 = 8K per pointer)
		kPage4Bits = 32-kPage1Bits-kPage2Bits-kPage3Bits-kTargetBitsRepresentedPerByte
	};

	struct PageAllocationElement
	{
		PageAllocationElement() : m_HighBits(0), m_PageAllocations(NULL){}
		UInt32 m_HighBits;
		int**** m_PageAllocations;
	};

	static const int kNumPageAllocationBlocks = 5; //each block represents 4gb of memory;
	mutable PageAllocationElement m_PageAllocationList[kNumPageAllocationBlocks];

	template<RequestType requestType>
	bool AllocationPage(const void* p) const;

	mutable Mutex m_AllocLock;

	void RegisterAllocation(const void* p);
	void RegisterDeallocation(const void* p);

	void* AddHeaderAndFooter( void* ptr, size_t size, int align ) const;
};

#endif

#endif
