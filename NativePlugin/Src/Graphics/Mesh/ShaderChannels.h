#pragma once

#include "GfxDevice/GfxDeviceTypes.h"
#include "Allocator/MemoryMacros.h"

struct ShaderChannelIterator
{
	ShaderChannelIterator(ShaderChannel startChan = kShaderChannelVertex) : channel(startChan), mask(1 << startChan) {}

	ShaderChannel GetChannel() { return ShaderChannel(channel); }
	UInt32 GetMask() { return mask; }

	void operator++() { channel++; mask <<= 1; }
	void operator++(int) { channel++; mask <<= 1; }

	FORCE_INLINE bool IsValid() const { return channel < kShaderChannelCount; }
	FORCE_INLINE bool IsInMask(UINT32 compareMask) const { return (mask & compareMask) != 0; }
	FORCE_INLINE bool AnyRemaining(UINT32 compareMask) const { return IsValid() && mask <= compareMask; }

private:
	int channel;
	UInt32 mask;
};