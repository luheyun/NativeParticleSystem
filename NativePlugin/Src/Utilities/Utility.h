#ifndef UTILITY_H
#define UTILITY_H

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

#endif