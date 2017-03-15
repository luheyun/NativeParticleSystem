#pragma once

class GfxDevice;
struct MeshBuffers;
struct DrawBuffersRange;

void AddDefaultStreamsToMeshBuffers(GfxDevice& device, MeshBuffers& buffers, const DrawBuffersRange& range, UInt32 wantedChannels, UInt32 availableChannels);
void AddDefaultStreamsToMeshBuffers(GfxDevice& device, MeshBuffers& buffers, UInt32 vertexRangeEnd, UInt32 wantedChannels, UInt32 availableChannels);