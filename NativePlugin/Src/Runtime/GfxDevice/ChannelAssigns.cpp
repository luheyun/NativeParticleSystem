#include "PluginPrefix.h"
#include "ChannelAssigns.h"
#include "Utilities/Word.h"

// Channel names (strings for reading from ShaderLab)
static const char * const kShaderChannelName[] = // Expected size is kShaderChannelCount
{
	"VERTEX",
	"NORMAL",
	"COLOR",
	"TEXCOORD",
	"TEXCOORD1",
	"TEXCOORD2",
	"TEXCOORD3",
#if GFX_HAS_TWO_EXTRA_TEXCOORDS
	"TEXCOORD4",
	"TEXCOORD5",
#endif
	"TANGENT",
};


ShaderChannel GetShaderChannelFromName( const std::string& name )
{
	std::string nameUpper = ToUpper(name);
	for( int i = 0; i < kShaderChannelCount; ++i )
		if( kShaderChannelName[i] == nameUpper )
			return (ShaderChannel)i;

	return kShaderChannelNone;
}



ChannelAssigns::ChannelAssigns()
:	m_TargetMap(0)
,	m_SourceMap(0)
{
	for( int i = 0; i < kVertexCompCount; ++i )
		m_Channels[i] = kShaderChannelNone;
}

void ChannelAssigns::MergeWith( const ChannelAssigns& additional )
{
	for( int i = 0; i < kVertexCompCount; ++i )
	{
		ShaderChannel source = additional.GetSourceForTarget(VertexComponent(i));
		if( source != kShaderChannelNone )
			Bind( source, (VertexComponent)i );
	}
}

void ChannelAssigns::Bind (ShaderChannel source, VertexComponent target)
{
	// TODO: skip kShaderChannelTexCoord ones here?
	// TODO: filter duplicates somehow?

	if (target != kVertexCompNone)
	{
		m_Channels[target] = source;
		m_TargetMap |= (1<<target);
	}
	m_SourceMap |= (1<<source);
}

void ChannelAssigns::Unbind( VertexComponent target )
{
	m_TargetMap &= ~(1<<target);
	m_Channels[target] = kShaderChannelNone;
}

bool ChannelAssigns::operator== (const ChannelAssigns& other) const
{
	return m_SourceMap == other.m_SourceMap &&
		   m_TargetMap == other.m_TargetMap &&
		   memcmp(&m_Channels[0], &other.m_Channels[0], sizeof(m_Channels)) == 0;
}
