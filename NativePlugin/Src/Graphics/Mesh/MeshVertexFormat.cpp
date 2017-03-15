#include "PluginPrefix.h"
#include "MeshVertexFormat.h"
#include "GfxDevice/GfxDevice.h"
#include "Shaders/GraphicsCaps.h"
#include "GfxDevice/GfxDeviceTypes.h"

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