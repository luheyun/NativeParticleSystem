#ifndef ENUM_FLAGS_H
#define ENUM_FLAGS_H

// Adds ability to use bit logic operators with enum type T.
// Enum must have appropriate values (e.g. kEnumValue1 = 1 << 1, kEnumValue2 = 1 << 2)!
#define ENUM_FLAGS(T) \
	inline T operator |(const T left, const T right) { return static_cast<T>(static_cast<unsigned>(left) | static_cast<unsigned>(right)); } \
	inline T operator &(const T left, const T right) { return static_cast<T>(static_cast<unsigned>(left) & static_cast<unsigned>(right)); } \
	inline T operator ^(const T left, const T right) { return static_cast<T>(static_cast<unsigned>(left) ^ static_cast<unsigned>(right)); } \
	\
	inline T operator ~(const T value) { return static_cast<T>(~static_cast<unsigned>(value)); } \
	\
	inline T& operator |=(T& left, const T right) { return left = left | right; } \
	inline T& operator &=(T& left, const T right) { return left = left & right; } \
	inline T& operator ^=(T& left, const T right) { return left = left ^ right; } \
	\
	inline bool AnyBitsSet(const T value, const T bitsToTest) { return (value & bitsToTest) != 0; } \
	inline bool AllBitsSet(const T value, const T bitsToTest) { return (value & bitsToTest) == bitsToTest; } \
	inline bool NoBitsSet(const T value, const T bitsToTest) { return (value & ~bitsToTest) == 0; } \
	inline T SetBits(const T value, const T bitsToSet) { return (value | bitsToSet); } \
	inline T ClearBits(const T value, const T bitsToClear) { return (value & ~bitsToClear); } \
	inline T SetOrClearBits(const T value, const T bitsToSetOrClear, bool set) \
	{ return set ? SetBits(value, bitsToSetOrClear) : ClearBits(value, bitsToSetOrClear); }

// Adds ability to use increment operators with enum type T.
// Enum must have consecutive values!
#define ENUM_INCREMENT(T) \
	inline T& operator++(T &value) { value = static_cast<T>(static_cast<int>(value) + 1); return value; } \
	inline T operator++(T &value, int) { T result = value; ++value; return result; }

#endif // ENUM_FLAGS_H
