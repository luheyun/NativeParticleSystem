#pragma once

#include "Allocator/MemoryMacros.h"

class ALIGN_TYPE(4) ColorRGBA32
{
public:
	UInt8 r, g, b, a;

	ColorRGBA32() {}
	ColorRGBA32(UInt8 r, UInt8 g, UInt8 b, UInt8 a) { this->r = r; this->g = g; this->b = b; this->a = a; }
	ColorRGBA32(UInt32 c) { *(UInt32*)this = c; }
};
