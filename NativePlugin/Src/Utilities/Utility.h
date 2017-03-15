#ifndef UTILITY_H
#define UTILITY_H

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

#endif