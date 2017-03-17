#include "PluginPrefix.h"
#include "VertexData.h"

/*
On most platforms, for skinning/non-uniform-scaling of meshes you would want to split your data into
a hot data stream (position, normal and tangent) and a cold data stream (diffuse and uvs) in order to maximize CPU cache access patterns and
reduce bandwidth and computation ( you won't need to copy the cold data )
*/

namespace VertexLayouts
{
#define MAKE_CHANNEL(fmt, dim) ChannelFormatDimension(kChannelFormat##fmt, dim)
	VertexChannelsLayout kVertexChannelsDefault =
	{ {	// Array wrapped by struct requires double braces
			MAKE_CHANNEL(Float, 3),		// position
			MAKE_CHANNEL(Float, 3),		// normal
			MAKE_CHANNEL(Color, 4),		// color
			MAKE_CHANNEL(Float, 2),		// texcoord0
			MAKE_CHANNEL(Float, 2),		// texcoord1
			MAKE_CHANNEL(Float, 2),		// texcoord2
			MAKE_CHANNEL(Float, 2),		// texcoord3
#if GFX_HAS_TWO_EXTRA_TEXCOORDS
			MAKE_CHANNEL(Float, 2),		// texcoord4
			MAKE_CHANNEL(Float, 2),		// texcoord5
#endif
			MAKE_CHANNEL(Float, 4)		// tangent
		} };
#undef MAKE_CHANNE
}

VertexData::~VertexData()
{
	Deallocate();
}

void VertexData::Deallocate()
{
	if (m_Data)
		delete m_Data;

	m_Data = NULL;
}

VertexChannelsInfo::VertexChannelsInfo(const ChannelInfoArray& src)
{
	memcpy(&channels, &src, sizeof(channels));
}

bool VertexChannelsInfo::operator < (const VertexChannelsInfo& rhs) const
{
	return memcmp(channels, rhs.channels, sizeof(channels)) < 0;
}