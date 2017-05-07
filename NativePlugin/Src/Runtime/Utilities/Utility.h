#ifndef UTILITY_H
#define UTILITY_H

#include "Allocator/MemoryMacros.h"

template <class DataType>
inline bool CompareArrays(const DataType *lhs, const DataType *rhs, size_t arraySize)
{
    for (size_t i = 0; i < arraySize; i++)
    {
        if (lhs[i] != rhs[i])
            return false;
    }
    return true;
}

template <size_t ElementSize, size_t Alignment>
inline bool CompareFixedSizeAndAlignment(const void* lhs, const void* rhs, size_t elementCount)
{
    // We check at compile time if it's safe to cast data to native integer types
    if (UNITY_64 && Alignment >= ALIGN_OF(UInt64) && (ElementSize % sizeof(UInt64)) == 0)
    {
        return CompareArrays(reinterpret_cast<const UInt64*>(lhs), reinterpret_cast<const UInt64*>(rhs), ElementSize / sizeof(UInt64) * elementCount);
    }
    else if (Alignment >= ALIGN_OF(UInt32) && (ElementSize % sizeof(UInt32)) == 0)
    {
        return CompareArrays(reinterpret_cast<const UInt32*>(lhs), reinterpret_cast<const UInt32*>(rhs), ElementSize / sizeof(UInt32) * elementCount);
    }
    else if (Alignment >= ALIGN_OF(UInt16) && (ElementSize % sizeof(UInt16)) == 0)
    {
        return CompareArrays(reinterpret_cast<const UInt16*>(lhs), reinterpret_cast<const UInt16*>(rhs), ElementSize / sizeof(UInt16) * elementCount);
    }
    else
        return !memcmp(lhs, rhs, ElementSize * elementCount);
}

template <class DataType>
inline bool CompareMemory(const DataType& lhs, const DataType& rhs)
{
#ifdef ALIGN_OF
    return CompareFixedSizeAndAlignment<sizeof(DataType), ALIGN_OF(DataType)>(&lhs, &rhs, 1);
#else
    return CompareFixedSizeAndAlignment<sizeof(DataType), 1>(&lhs, &rhs, 1);
#endif
}

template <class T>
inline T clamp(const T&t, const T& t0, const T& t1)
{
	if (t < t0)
		return t0;
	else if (t > t1)
		return t1;
	else
		return t;
}

// Rounds value up.
// Note: multiple does not need to be a power of two value.
template <class DataType>
inline DataType RoundUpMultiple(DataType value, DataType multiple)
{
	return ((value + multiple - 1) / multiple) * multiple;
}

template<class T>
inline T* Stride(T* p, size_t offset)
{
	return reinterpret_cast<T*>((char*)p + offset);
}

template <class T>
inline T clamp01(const T& t)
{
	if (t < 0)
		return 0;
	else if (t > 1)
		return 1;
	else
		return t;
}

// Euclid's algorithm:
// https://en.wikipedia.org/wiki/Euclidean_algorithm
inline UInt32 GreatestCommonDenominator(UInt32 a, UInt32 b)
{
	for (;;)
	{
		if (a == 0) return b;
		b %= a;
		if (b == 0) return a;
		a %= b;
	}

	return 0;
}

inline UInt32 LeastCommonMultiple(UInt32 a, UInt32 b)
{
	UInt32 temp = GreatestCommonDenominator(a, b);
	return temp ? (a / temp * b) : 0;
}

#endif