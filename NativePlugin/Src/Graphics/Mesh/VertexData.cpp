#include "PluginPrefix.h"
#include "VertexData.h"

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