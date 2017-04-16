#ifndef BITUTILITY_H
#define BITUTILITY_H

// return the next power-of-two of a 32bit number
inline UInt32 NextPowerOfTwo(UInt32 v)
{
	v -= 1;
	v |= v >> 16;
	v |= v >> 8;
	v |= v >> 4;
	v |= v >> 2;
	v |= v >> 1;
	return v + 1;
}

#endif