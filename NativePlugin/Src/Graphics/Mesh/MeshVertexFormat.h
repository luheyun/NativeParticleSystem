#pragma once

#include "Graphics/Mesh/VertexData.h"

class GfxDevice;
struct MeshBuffers;
struct DrawBuffersRange;

void AddDefaultStreamsToMeshBuffers(GfxDevice& device, MeshBuffers& buffers, const DrawBuffersRange& range, UInt32 wantedChannels, UInt32 availableChannels);
void AddDefaultStreamsToMeshBuffers(GfxDevice& device, MeshBuffers& buffers, UInt32 vertexRangeEnd, UInt32 wantedChannels, UInt32 availableChannels);

size_t GetChannelFormatSize(UInt8 format);

inline size_t CalculateVertexElementSize(ChannelFormatDimension elem)
{
	return GetChannelFormatSize(elem.format) * elem.dimension;
}

inline GfxDefaultVertexBufferType GetDefaultVertexBufferTypeForChannel(ShaderChannel channel)
{
	switch (channel)
	{
	case kShaderChannelNormal: // blue
	case kShaderChannelTangent: // red
		return kGfxDefaultVertexBufferRedBlue;
	default:
		return kGfxDefaultVertexBufferBlackWhite;
	}
}

inline int GetDefaultVertexBufferOffsetForChannel(ShaderChannel channel)
{
	switch (channel)
	{
	case kShaderChannelColor:
		return 0; // white
	case kShaderChannelNormal:
		return 0; // blue
	case kShaderChannelTangent:
		return 4; // red
	default:
		return 4; // black
	}
}

// Build channel info from a mask and vertex layout, assuming channels are contiguous in one stream
size_t BuildSingleStreamChannelInfo(UInt32 shaderChannelMask, const VertexChannelsLayout& vertexChannels, ChannelInfoArray channels);

// The MeshVertexFormat class represents a vertex format of a source vertex buffer (or a set of vertex buffers).
// It is shared between meshes whose vertices are in the same format, and keeps a cache of suitable vertex declarations.
// The final declaration depends on both the source format and the set of ShaderChannels a shader takes as input.
//
// When returning a VertexDeclaration, MeshVertexFormat does the following:
//
// 1. Strips any channels that are unused by the shader
//
// 2. Duplicates missing texture coordinates
//     * For example TexCoord1 will be read from TexCoord0 from a source that only has 1 texcoord
//     * This works because vertex declarations allow multiple elements to be read from the same offset
//
// 3. Sets up missing properties that are in GraphicsCaps::requiredShaderChannels
//     * The idea is that missing properties need good default values that are read from additional vertex streams
//     * These inputs are added to the vertex declaration, but the caller must provide the streams for DrawBuffers()
//     * Use AddDefaultStreamsToMeshBuffers() to add streams to a MeshBuffers struct 

class EXPORT_COREMODULE MeshVertexFormat
{
public:
	MeshVertexFormat(UInt32 uniqueID, const VertexChannelsInfo& sourceChannels, bool renderThread);
	~MeshVertexFormat();

	VertexDeclaration* GetVertexDeclaration(UInt32 wantedChannels, const MeshVertexFormat* secondaryStreams = NULL);

	UInt32 GetAvailableChannels() const { return m_AvailableChannels; }
	const ChannelInfo& GetSourceChannel(ShaderChannel index) const	{ return m_SourceChannels.channels[index]; }

private:
	friend class MeshVertexFormatManager;

	VertexDeclaration* CreateVertexDeclaration(UInt32 wantedChannels, const MeshVertexFormat* secondaryStreams);
	UInt8 GetUsedStreamCount() const;

	typedef std::map<UInt64, VertexDeclaration*> VertexDeclVectorMap;

	UInt32 m_UniqueID;
	UInt32 m_AvailableChannels;
	VertexChannelsInfo m_SourceChannels;
	VertexDeclVectorMap m_VertexDecls;
	bool m_RenderThread;
};

// Helper struct to construct VD from default channel layout
class EXPORT_COREMODULE DefaultMeshVertexFormat
{
public:
	DefaultMeshVertexFormat(UInt32 channels);

	MeshVertexFormat* GetVertexFormat()
	{
		return vertexFormat;
	}

	VertexDeclaration* GetVertexDecl(UInt32 shaderChannels)
	{
		return GetVertexFormat()->GetVertexDeclaration(shaderChannels);
	}

	const UInt32& GetChannels() const { return channels; }

private:

	friend EXPORT_COREMODULE void InitializeMeshVertexFormatManager();
	UInt32 channels;
	MeshVertexFormat* vertexFormat;
};

// MeshVertexFormatManager is a global manager that keeps a map of MeshVertexFormats
class MeshVertexFormatManager
{
public:
	MeshVertexFormatManager();
	~MeshVertexFormatManager();

	MeshVertexFormat* GetMeshVertexFormat(const VertexChannelsInfo& sourceChannels);
	MeshVertexFormat* GetDefault(UInt32 shaderChannels);

private:
	typedef std::map<VertexChannelsInfo, MeshVertexFormat> MeshVertexFormatMap;
	MeshVertexFormatMap m_MeshVertexFormapMap;
	UInt32 m_NextUniqueID;
};

MeshVertexFormatManager& GetMeshVertexFormatManager();
EXPORT_COREMODULE void InitializeMeshVertexFormatManager();
