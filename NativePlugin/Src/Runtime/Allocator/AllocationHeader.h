#ifndef ALLOCATION_HEADER_H_
#define ALLOCATION_HEADER_H_

#if ENABLE_MEMORY_MANAGER

#include "Utilities/BitUtility.h"
#include "Utilities/TypeUtilities.h"

/*
Allocation Header:
(12345hhhhhhhhmmmm____________) (1-n:paddingcount, h: header, m:memoryprofiler)
hasPadding: 1 bit
size: 31 or 63 bits depending on size of size_t
*********USE_MEMORY_DEBUGGING*********
allocator: 16 bit
magicValue: 12 bit
*********USE_MEMORY_DEBUGGING*********
---------------
Followed by x bytes requested by MemoryProfiler
----------------------------------------
Actual Allocated Data
----------------------------------------
*********USE_MEMORY_DEBUGGING*********
Followed by a footer if memory guarding is enabled
*********USE_MEMORY_DEBUGGING*********
*/

/// Allocation size info
class AllocationSizeHeader
{
public:
	void   InitAllocationSizeHeader(void* allocPtr, size_t padCount, size_t size)
	{
		// We assume, that allocPtr MUST be at least 4 bytes aligned to fit possible padding length
		//DebugAssert(allocPtr == AlignPtr(allocPtr, 4));
		if (padCount > 0) // set leading bytes
			memset(allocPtr, kPadValue, padCount);
		SetPaddingCount(padCount);
		m_AllocationSize = size;
	}

	bool   HasPadding() const { return m_HasPadding; }
	size_t GetAllocationSize() const { return m_AllocationSize; }

	void   SetPaddingCount(size_t padding)
	{
		m_HasPadding = padding > 0;
		if (padding > 0)
			*((UInt32*)this - 1) = padding;
	}
	size_t GetPaddingCount() const { return m_HasPadding ? *((const UInt32*)this - 1) : 0; }

protected:
	static const UInt8 kPadValue = 0xAA;

	size_t m_HasPadding     : 1;
	size_t m_AllocationSize : ((sizeof(size_t) << 3) - 1);
};

class NullAllocationSizeHeader
{
public:
	void   InitAllocationSizeHeader(void* allocPtr, size_t padCount, size_t size) {}
	bool   HasPadding() const { return false; }
	void   SetPaddingCount(size_t padding) {}
	size_t GetPaddingCount() const { return 0; }
};


/// Declare memory debugging header
#pragma pack(push, 1)
class MemoryDebuggingHeader
#if USE_MEMORY_DEBUGGING || ENABLE_MEM_PROFILER
	: public AllocationSizeHeader
#endif
{
#if USE_MEMORY_DEBUGGING
public:
	void InitMemoryDebuggingHeader(void* footerPtr, int allocatorId)
	{
		m_AllocatorIdentifier = allocatorId & 0xFFFF;
		m_Magic = kMagicValue;
		// set footer
		memset(footerPtr, kFooterValue, kFooterSize);
	}
	UInt16 GetAllocatorIdentifier() const { return m_AllocatorIdentifier; }
protected:
	static const UInt32 kMagicValue = 0xDFA;
	static const UInt32 kFooterSize = 4;
	static const UInt8  kFooterValue = 0xFD;

	UInt16 m_AllocatorIdentifier;
	UInt16 m_Magic : 12;
#else
public:
	void InitMemoryDebuggingHeader(void* footerPtr, int allocatorId) {}
protected:
	static const UInt32 kFooterSize = 0;
#endif
};
#pragma pack(pop)

template<typename T>
class HasAllocationSize
{
	typedef struct { char m; } yes;
	typedef struct { char m[2]; } no;
	struct BaseMixin
	{
		size_t GetAllocationSize() const { return 0; };
	};
	struct Base : public T, public BaseMixin {};
	template <typename U, U> class Helper {};
	template <typename U> static no deduce(U*, Helper<size_t(BaseMixin::*)()const, &U::GetAllocationSize>* = 0);
	static yes deduce(...);
public:
	static const bool result = (sizeof(yes) == sizeof(deduce((Base*)(0))));
};

template<bool HasAllocationSize>
struct RealSizeHelper
{
	template<class T>
	static size_t GetRealAllocationSize(const T* header);
};

template<class T>
class AllocationHeaderBase :
	public MemoryDebuggingHeader,
	public Conditional<HasAllocationSize<MemoryDebuggingHeader>::result, NullType, T>::type
{
public:
	static AllocationHeaderBase<T>*       Init(void* allocPtr, int allocatorId, size_t size, size_t align); ///< Init header data
	static const AllocationHeaderBase<T>* GetAllocationHeader(const void* userPtr);                     ///< Return AllocationHeader ptr from user ptr

	static size_t CalculateNeededAllocationSize(size_t size, int align);                                ///< Return total size for allocation with all attached headers
	static size_t GetRequiredPadding(void* allocPtr, size_t userAlign);
	static bool   ValidateIntegrity(void* allocPtr, int allocatorId);
	static size_t GetSelfSize();                                                                        ///< Return AllocationHeader size without profiler data size
	static size_t GetSize();                                                                            ///< Return AllocationHeader data size

	void*  GetAllocationPtr() const;                                                                    ///< Return real ptr allocated by allocator
	void*  GetUserPtr() const;                                                                          ///< Return ptr that can be used
	ProfilerAllocationHeader* GetProfilerHeader() const;
	size_t AdjustUserPtrSize(size_t size) const;                                                        ///< Return size without header size

	size_t GetRealAllocationSize() const;                                                               ///< Return real allocation size including padding and header itself
	size_t GetOverheadSize() const;

protected:
	// Required for correct GetPaddingCount and InitAllocationSizeHeader functions resolving when using PSM compiler.
	typedef typename Conditional<HasAllocationSize<MemoryDebuggingHeader>::result, MemoryDebuggingHeader, T>::type BaseTypeWithPadding;

	friend struct RealSizeHelper<HasAllocationSize<BaseTypeWithPadding>::result>;

	// AllocationSizeHeader
	// MemoryDebuggingHeader
	// ProfilerAllocationHeader
	// [data]
	// Footer
};

//////////////////////////////////////////////////////////////////////////

template<class T>
inline AllocationHeaderBase<T>* AllocationHeaderBase<T>::Init(void* allocPtr, int allocatorId, size_t size, size_t align)
{
	DebugAssert(align > 0 && align <= 16 * 1024 && IsPowerOfTwo(align));

	size_t padCount = GetRequiredPadding(allocPtr, align);
	AllocationHeaderBase<T>* header = (AllocationHeaderBase<T>*)((char*)allocPtr + padCount);
	header->BaseTypeWithPadding::InitAllocationSizeHeader(allocPtr, padCount, size);
	header->InitMemoryDebuggingHeader(((char*)header->GetUserPtr()) + size, allocatorId);

	return header;
}

template<class T>
inline const AllocationHeaderBase<T>* AllocationHeaderBase<T>::GetAllocationHeader(const void* userPtr)
{
	return (const AllocationHeaderBase<T>*)((const char*)userPtr - GetSize());
}

template<class T>
inline size_t AllocationHeaderBase<T>::CalculateNeededAllocationSize(size_t size, int align)
{
	if (HasAllocationSize<AllocationHeaderBase<T> >::result)
	{
		// If size info is included we need extra space for internal alignment
		int alignMask = align - 1;
		return alignMask + GetSize() + size + kFooterSize;
	}
	else
		return AlignSize(size, align);
}

template<class T>
inline size_t AllocationHeaderBase<T>::GetRequiredPadding(void* allocPtr, size_t userAlign)
{
	if (HasAllocationSize<AllocationHeaderBase<T> >::result)
		return userAlign - ((((size_t)allocPtr + GetSize() - 1) & (userAlign - 1)) + 1);
	else
		return 0;
}

template<class T>
inline size_t AllocationHeaderBase<T>::GetSelfSize()
{
	if (HasAllocationSize<AllocationHeaderBase<T> >::result)
		return sizeof(AllocationHeaderBase<T>);
	else
		return 0;
}

template<class T>
inline size_t AllocationHeaderBase<T>::GetSize()
{
	return
		GetSelfSize()                            // Self size
		+ MemoryProfiler::GetHeaderSize();       // ProfilerAllocationHeader size
}

template<class T>
inline void* AllocationHeaderBase<T>::GetAllocationPtr() const
{
	return (void*)((const char*)this - BaseTypeWithPadding::GetPaddingCount());
}

template<class T>
inline void* AllocationHeaderBase<T>::GetUserPtr() const
{
	return (void*)((const char*)this + GetSize());
}

template<class T>
inline ProfilerAllocationHeader* AllocationHeaderBase<T>::GetProfilerHeader() const
{
	return (ProfilerAllocationHeader*) ((const char*)this + GetSelfSize());
}

template<class T>
inline size_t AllocationHeaderBase<T>::AdjustUserPtrSize(size_t size) const
{
	return size - GetSize() - BaseTypeWithPadding::GetPaddingCount();
}

template<class T>
inline size_t AllocationHeaderBase<T>::GetRealAllocationSize() const
{
	return RealSizeHelper<HasAllocationSize<AllocationHeaderBase<T> >::result>::GetRealAllocationSize(this);
}

template<>
template<class T>
inline size_t RealSizeHelper<true>::GetRealAllocationSize(const T* header)
{
	return header->GetOverheadSize() + header->GetSize() + header->GetAllocationSize() + T::kFooterSize;
}

//template<>
//template<class T>
//inline size_t RealSizeHelper<false>::GetRealAllocationSize(const T* header)
//{
//	CompileTimeAssert(false, "GetRealAllocationSize() can be used only if header has GetAllocationSize()");
//}

template<class T>
inline size_t AllocationHeaderBase<T>::GetOverheadSize() const
{
	if (HasAllocationSize<AllocationHeaderBase<T> >::result)
		return GetSize() + kFooterSize + kDefaultMemoryAlignment - 1;  // Estimate
	else
		return GetSize();                                              // Must be 0
}

template<class T>
inline bool AllocationHeaderBase<T>::ValidateIntegrity(void* allocPtr, int allocatorId)
{
#if USE_MEMORY_DEBUGGING
	AllocationHeaderBase<T>* header = (AllocationHeaderBase<T>*)allocPtr;
	if (header->m_Magic != kMagicValue)
	{
		UInt8* ptr = (UInt8*)allocPtr;
		while (*ptr == kPadValue)
			ptr++;

		size_t padCount = *(UInt32*)ptr;
		ptr += sizeof(UInt32);               // Skip pad count
		Assert(ptr - (UInt8*)allocPtr == padCount);
		header = (AllocationHeaderBase<T>*)(ptr);
	}

	if (header->m_Magic != kMagicValue)
	{
		ErrorString("Invalid memory pointer was detected. Header is corrupted!");
		return false;
	}

	if (allocatorId != -1 && (allocatorId & 0xFFFF) != header->m_AllocatorIdentifier)
	{
		WarningString("Invalid allocation was detected. Mismatching Allocate/Deallocate memory labels!");
		return false;
	}

	size_t size = header->m_AllocationSize;
	UInt8* footer = (UInt8*)header->GetUserPtr() + size;
	for (size_t i = 0; i < kFooterSize; i++, footer++)
	{
		if (*footer != kFooterValue)
		{
			ErrorString("Invalid memory pointer was detected. Footer is corrupted!");
			return false;
		}
}
#endif
	return true;
}

typedef AllocationHeaderBase<NullAllocationSizeHeader> AllocationHeader;
typedef AllocationHeaderBase<AllocationSizeHeader>     AllocationHeaderWithSize;

#endif
#endif
