#ifndef __CHANNELASSIGNS_H__
#define __CHANNELASSIGNS_H__

#include <string>
#include "GfxDeviceTypes.h"

namespace ShaderLab {
	struct ParserBindChannels;
}

// Tracks which vertex components should be sourced from which shader channels
// TODO: It gets serialized a lot for multithreading, can we make it more compact?
class EXPORT_COREMODULE ChannelAssigns {
public:
	ChannelAssigns();
	void FromParsedChannels(const ShaderLab::ParserBindChannels& parsed); // ShaderParser.cpp

	void Bind(ShaderChannel source, VertexComponent target);
	void Unbind(VertexComponent target);
	void MergeWith(const ChannelAssigns& additional);

	UInt32 GetTargetMap() const { return m_TargetMap; }
	UInt32 GetSourceMap() const { return m_SourceMap; }
	bool IsEmpty() const { return m_TargetMap == 0; }

	ShaderChannel GetSourceForTarget(VertexComponent target) const { return ShaderChannel(m_Channels[target]); }

	bool operator== (const ChannelAssigns& other) const;

private:
	UInt32	m_TargetMap; // bitfield of which vertex components are sourced
	UInt32	m_SourceMap; // bitfield of which source channels are used
	SInt8	m_Channels[kVertexCompCount]; // for each vertex component: from which channel it is sourced
};


ShaderChannel GetShaderChannelFromName(const std::string& name);


#endif
