#ifndef VERTEX_DATA_H_
#define VERTEX_DATA_H_

#include "GfxDevice/GfxDeviceTypes.h"
#include "Allocator/MemoryMacros.h"

struct ChannelInfo;

class VertexData
{
public:
	VertexData() { }
	~VertexData();
	void Deallocate();

private:
	UInt8* m_Data;
};

struct ALIGN_TYPE(4) VertexStreamsLayout
{
	UInt32 channelMasks[kMaxVertexStreams];
};

typedef struct ALIGN_TYPE(4) ChannelInfo
{
	UInt8 stream;
	UInt8 offset;
	UInt8 format;
	UInt8 dimension;

	enum { kInvalidDimension = 0 };

	ChannelInfo() : stream(0), offset(0), format(0), dimension(kInvalidDimension) {}

	bool IsValid() const { return kInvalidDimension != dimension; }

	bool operator ==(const ChannelInfo& rhs) const { return (stream == rhs.stream) && (offset == rhs.offset) && (format == rhs.format) && (dimension == rhs.dimension); }
	bool operator !=(const ChannelInfo& rhs) const { return !(*this == rhs); }

} ChannelInfoArray[kShaderChannelCount];

struct VertexChannelsInfo
{
	VertexChannelsInfo() {}
	VertexChannelsInfo(const ChannelInfoArray& src);
	bool operator < (const VertexChannelsInfo& rhs) const;
	ChannelInfoArray channels;
};

struct ChannelFormatDimension
{
	ChannelFormatDimension(const ChannelInfo& src) : format(src.format), dimension(src.dimension) {}
	ChannelFormatDimension(VertexChannelFormat fmt, int dim) : format(UInt8(fmt)), dimension(UInt8(dim)) {}
	ChannelFormatDimension() : format(0), dimension(0) {}

	UInt8 format;
	UInt8 dimension;
};

struct VertexChannelsLayout
{
	ChannelFormatDimension channels[kShaderChannelCount];
};

namespace VertexLayouts
{
	extern VertexChannelsLayout kVertexChannelsDefault;
};

inline int GetPrimitiveCount(int indexCount, GfxPrimitiveType topology, bool nativeQuads)
{
	switch (topology) {
	case kPrimitiveTriangles: return indexCount / 3;
	case kPrimitiveTriangleStrip: return indexCount - 2;
	case kPrimitiveQuads: return nativeQuads ? indexCount / 4 : indexCount / 4 * 2;
	case kPrimitiveLines: return indexCount / 2;
	case kPrimitiveLineStrip: return indexCount - 1;
	case kPrimitivePoints: return indexCount;
	default:  return 0;
	};
}

#endif