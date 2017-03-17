#include "PluginPrefix.h"
#include "MeshVertexFormat.h"
#include "GfxDevice/GfxDevice.h"
#include "Shaders/GraphicsCaps.h"
#include "GfxDevice/GfxDeviceTypes.h"
#include "Graphics/Mesh/ShaderChannels.h"
#include "Graphics/Mesh/VertexData.h"

static const UInt8 kVertexChannelFormatSizes[] = // Expected size is kChannelFormatCount
{
	4,  // kChannelFormatFloat
	2,  // kChannelFormatFloat16
	1,  // kChannelFormatColor
	1,  // kChannelFormatByte
	4,  // kChannelFormatUInt32
};

void AddDefaultStreamsToMeshBuffers(GfxDevice& device, MeshBuffers& buffers, const DrawBuffersRange& range, UInt32 wantedChannels, UInt32 availableChannels)
{
	UInt32 vertexRangeEnd = range.baseVertex + range.firstVertex + range.vertexCount;
	AddDefaultStreamsToMeshBuffers(device, buffers, vertexRangeEnd, wantedChannels, availableChannels);
}

void AddDefaultStreamsToMeshBuffers(GfxDevice& device, MeshBuffers& buffers, UInt32 vertexRangeEnd, UInt32 wantedChannels, UInt32 availableChannels)
{
	//UInt32 missingChannels = (wantedChannels & ~availableChannels) & GetGraphicsCaps().requiredShaderChannels;
	//UInt8 defaultStreamIndices[kGfxDefaultVertexBufferCount] = { 0, 0 };
	//for (ShaderChannelIterator it; it.AnyRemaining(missingChannels); ++it)
	//{
	//	if (missingChannels & it.GetMask())
	//	{
	//		GfxDefaultVertexBufferType type = GetDefaultVertexBufferTypeForChannel(it.GetChannel());
	//		UInt8& defaultStreamIndex = defaultStreamIndices[type];
	//		if (defaultStreamIndex == 0)
	//		{
	//			defaultStreamIndex = buffers.vertexStreamCount++;
	//			buffers.vertexStreams[defaultStreamIndex] = device.GetDefaultVertexBuffer(type, vertexRangeEnd);
	//		}
	//	}
	//}
}

static UInt32 CalculateAvailableChannels(const ChannelInfoArray& channels)
{
	UInt32 channelMask = 0;

	for (ShaderChannelIterator it; it.IsValid(); ++it)
	{
		if (channels[it.GetChannel()].IsValid())
			channelMask |= it.GetMask();
	}

	return channelMask;
}

size_t GetChannelFormatSize(UInt8 format)
{
	return kVertexChannelFormatSizes[format];
}

size_t BuildSingleStreamChannelInfo(UInt32 shaderChannelMask, const VertexChannelsLayout& vertexChannels, ChannelInfoArray channels)
{
	size_t offset = 0;
	for (ShaderChannelIterator it; it.AnyRemaining(shaderChannelMask); ++it)
	{
		if (!it.IsInMask(shaderChannelMask))
			continue;

		ChannelInfo& info = channels[it.GetChannel()];
		const ChannelFormatDimension& chan = vertexChannels.channels[it.GetChannel()];
		info.stream = 0;
		info.offset = offset;
		info.format = chan.format;
		info.dimension = chan.dimension;
		offset += CalculateVertexElementSize(chan);
	}

	return offset;
}

MeshVertexFormat::MeshVertexFormat(UInt32 uniqueID, const VertexChannelsInfo& sourceChannels, bool renderThread)
	: m_UniqueID(uniqueID)
	, m_SourceChannels(sourceChannels)
	, m_RenderThread(renderThread)
{
	m_AvailableChannels = CalculateAvailableChannels(sourceChannels.channels);
}

MeshVertexFormat::~MeshVertexFormat()
{
}

VertexDeclaration* MeshVertexFormat::GetVertexDeclaration(UInt32 wantedChannels, const MeshVertexFormat* secondaryStreams)
{
	// Only include channels that are in the source data or always required by the device.
	// Texcoord channels are included too since they use the previous texcoord if missing.
	UInt32 maskedChannels = wantedChannels & (m_AvailableChannels | GetGraphicsCaps().requiredShaderChannels | kTexCoordShaderChannelsMask);
	UInt64 key = maskedChannels;
	if (secondaryStreams)
		key |= UInt64(secondaryStreams->m_UniqueID) << 32;

	{
		VertexDeclVectorMap::iterator found = m_VertexDecls.find(key);
		if (found != m_VertexDecls.end())
			return found->second;
	}

	VertexDeclaration* vertexDecl = CreateVertexDeclaration(maskedChannels, secondaryStreams);
	{
		m_VertexDecls.insert(VertexDeclVectorMap::value_type(key, vertexDecl));
	}

	return vertexDecl;
}

VertexDeclaration* MeshVertexFormat::CreateVertexDeclaration(UInt32 wantedChannels, const MeshVertexFormat* secondaryStreams)
{
	const bool hasSecondary = secondaryStreams != NULL;
	VertexChannelsInfo secondaryChannels;
	UInt8 firstFreeStream = GetUsedStreamCount();
	if (hasSecondary)
	{
		// Append secondary streams after the last used stream
		secondaryChannels = secondaryStreams->m_SourceChannels;
		for (ShaderChannelIterator it; it.IsValid(); ++it)
		{
			ChannelInfo& channelInfo = secondaryChannels.channels[it.GetChannel()];
			if (channelInfo.IsValid())
			{
				channelInfo.stream += firstFreeStream;
			}
		}
		firstFreeStream += secondaryStreams->GetUsedStreamCount();
	}

	VertexChannelsInfo channelsInfo;
	ChannelInfo lastValidTexCoord;

	UInt8 defaultStreamIndices[kGfxDefaultVertexBufferCount] = { 0, 0 };

	for (ShaderChannelIterator it; it.AnyRemaining(wantedChannels); ++it)
	{
		ShaderChannel channel = it.GetChannel();
		bool isTexCoord = (it.GetMask() & kTexCoordShaderChannelsMask) != 0;
		ChannelInfo sourceChannel = m_SourceChannels.channels[channel];

		// Override with channel from secondary streams
		const ChannelInfo& secondaryChannel = secondaryChannels.channels[channel];
		if (hasSecondary && secondaryChannel.IsValid())
			sourceChannel = secondaryChannel;

		// If we have a valid texcoord channel, remember it
		if (isTexCoord && sourceChannel.IsValid())
			lastValidTexCoord = sourceChannel;

		// Do we want to include this channel?
		if (!(wantedChannels & it.GetMask()))
			continue;

		ChannelInfo& destChannel = channelsInfo.channels[channel];

		if (sourceChannel.IsValid())
		{
			// We have a valid channel
			destChannel = sourceChannel;
		}
		else if (isTexCoord && lastValidTexCoord.IsValid())
		{
			// Replicate last valid texture coord
			destChannel = lastValidTexCoord;
		}
		else if (GetGraphicsCaps().requiredShaderChannels & it.GetMask())
		{
			// Add vertex stream for default black/white values if needed
			GfxDefaultVertexBufferType type = GetDefaultVertexBufferTypeForChannel(channel);
			UInt8& defaultStreamIndex = defaultStreamIndices[type];
			if (defaultStreamIndex == 0)
				defaultStreamIndex = firstFreeStream++;

			destChannel.stream = defaultStreamIndex;
			destChannel.offset = GetDefaultVertexBufferOffsetForChannel(channel);
			destChannel.format = kChannelFormatColor;
			destChannel.dimension = 4;
		}
	}

	GfxDevice& device = GetGfxDevice();
	return device.GetVertexDeclaration(channelsInfo);
}

UInt8 MeshVertexFormat::GetUsedStreamCount() const
{
	UInt8 lastUsedStream = 0;
	for (ShaderChannelIterator it; it.AnyRemaining(m_AvailableChannels); ++it)
	{
		const ChannelInfo& channel = m_SourceChannels.channels[it.GetChannel()];
		if (channel.IsValid())
			lastUsedStream = std::max(lastUsedStream, channel.stream);
	}
	return lastUsedStream + UInt8(1);
}

MeshVertexFormatManager::MeshVertexFormatManager()
	: m_NextUniqueID(0)
{
}

MeshVertexFormatManager::~MeshVertexFormatManager()
{
}

MeshVertexFormat* MeshVertexFormatManager::GetMeshVertexFormat(const VertexChannelsInfo& sourceChannels)
{
	auto it = m_MeshVertexFormapMap.lower_bound(sourceChannels);

	if (it != m_MeshVertexFormapMap.end() && !m_MeshVertexFormapMap.key_comp()(sourceChannels, it->first))
		return &it->second;

	// Insert a new source format into map
	UInt32 uniqueID = ++m_NextUniqueID; // Zero is invalid
	MeshVertexFormat& result = m_MeshVertexFormapMap.insert(it, 
		MeshVertexFormatMap::value_type(sourceChannels, MeshVertexFormat(uniqueID, sourceChannels, false)))->second;
	return &result;
}

MeshVertexFormat* MeshVertexFormatManager::GetDefault(UInt32 shaderChannels)
{
	VertexChannelsInfo sourceChannels;
	BuildSingleStreamChannelInfo(shaderChannels, VertexLayouts::kVertexChannelsDefault, sourceChannels.channels);
	return GetMeshVertexFormat(sourceChannels);
}

const size_t               kMaxStaticDefaultMeshVertexFormats = 20;
static DefaultMeshVertexFormat* 	gStaticInitializedMeshVertexFormats[kMaxStaticDefaultMeshVertexFormats];
static size_t              			gStaticInitializedMeshVertexFormatCount = 0;

DefaultMeshVertexFormat::DefaultMeshVertexFormat(UInt32 channels)
	: channels(channels), vertexFormat(NULL)
{
	gStaticInitializedMeshVertexFormats[gStaticInitializedMeshVertexFormatCount++] = this;
}

static MeshVertexFormatManager* s_MeshVertexFormatManager = new MeshVertexFormatManager();

MeshVertexFormatManager& GetMeshVertexFormatManager()
{
	return *s_MeshVertexFormatManager;
}

void InitializeMeshVertexFormatManager()
{
	for (size_t i = 0; i < gStaticInitializedMeshVertexFormatCount; ++i)
	{
		DefaultMeshVertexFormat* format = gStaticInitializedMeshVertexFormats[i];
		format->vertexFormat = GetMeshVertexFormatManager().GetDefault(format->channels);
	}
}