#include "PluginPrefix.h"
#include "GenericDynamicVBO.h"
#include "GfxDevice/GfxDevice.h"
#include "GfxDevice/ChannelAssigns.h"
#include "GfxDevice/GfxBuffer.h"
#include "Utilities/Utility.h"
#include "Utilities/BitUtility.h"
#include "Graphics/Mesh/MeshVertexFormat.h"

struct GenericDynamicVBOChunk
{
	GenericDynamicVBOChunk ()
		: vb (NULL)
		, ib (NULL)
		, vertexDrawOffset (0)
		, indexDrawOffset (0)
		{}

	DynamicVBOChunk			main; // DynamicVBO is responsible for this part
	GfxBuffer*				vb;
	GfxBuffer*				ib;
	UInt32					vertexDrawOffset;
	UInt32					indexDrawOffset;
};

static std::vector<GenericDynamicVBOChunk> s_ChunkArray[2];
static std::vector<GenericDynamicVBOChunk> s_RenderThreadChunkArray;

DynamicVBOChunk* GenericDynamicVBO::HandleToChunk(const DynamicVBOChunkHandle& chunkHandle, bool createIfNotExist)
{
	std::vector<GenericDynamicVBOChunk>& chunks = chunkHandle.renderThread ? s_RenderThreadChunkArray : s_ChunkArray[chunkHandle.frame & 1];

	if (createIfNotExist && chunks.size() <= chunkHandle.id)
		chunks.resize(chunkHandle.id + 1, GenericDynamicVBOChunk());

	return &chunks[chunkHandle.id].main;
}

void GenericDynamicVBO::SwapBuffers(UInt16 frameIndex)
{
	DynamicVBO::SwapBuffers(frameIndex);

	// delete all chunks that were in use by the previous frame's VBOs
	s_ChunkArray[frameIndex & 1].resize(0);
	s_RenderThreadChunkArray.resize(0);

	// reset positions
	if (m_ActiveVertexBufferIndex != -1)
		m_ActiveVertexBufferIndex = 0;
	if (m_ActiveIndexBufferIndex != -1)
		m_ActiveIndexBufferIndex = 0;

	m_VertexPos.writeOffset = 0;
	m_IndexPos.writeOffset = 0;
}

GenericDynamicVBO::GenericDynamicVBO(GfxDevice& device, GfxBufferMode bufferMode, UInt32 initialVBSize, UInt32 initialIBSize)
:	m_Device(device)
,	m_BufferMode(bufferMode)
//,	m_VertexBuffers(kMemDefault)
//,	m_IndexBuffers(kMemDefault)
,	m_ActiveVertexBufferIndex(-1)
,	m_ActiveIndexBufferIndex(-1)
,	m_VertexBufferChunkSize(initialVBSize)
,	m_IndexBufferChunkSize(initialIBSize)
{
	m_VertexBuffers.reserve(8);
	m_IndexBuffers.reserve(8);

	//s_ChunkArray[0].set_memory_label(kMemRenderer);
	//s_ChunkArray[1].set_memory_label(kMemRenderer);
	s_ChunkArray[0].reserve(64);
	s_ChunkArray[1].reserve(64);

	//s_RenderThreadChunkArray.set_memory_label(kMemRenderer);
	s_RenderThreadChunkArray.reserve(64);
}

GenericDynamicVBO::~GenericDynamicVBO ()
{
	// delete all vb/ib
	for (size_t i = 0; i < m_VertexBuffers.size(); i++)
		m_Device.DeleteBuffer(m_VertexBuffers[i]);

	for (size_t i = 0; i < m_IndexBuffers.size(); i++)
		m_Device.DeleteBuffer (m_IndexBuffers[i]);

	// delete all chunks
	s_ChunkArray[0].clear();
	s_ChunkArray[1].clear();
	s_RenderThreadChunkArray.clear();
}


UInt8* GenericDynamicVBO::AllocateVB(UInt32 size, DynamicVBOChunkHandle& chunkHandle)
{
	GenericDynamicVBOChunk* chunk = (GenericDynamicVBOChunk*)HandleToChunk(chunkHandle, false);
	
	if (m_BufferMode == kGfxBufferModeCircular)
	{
		if (m_ActiveVertexBufferIndex == -1)
		{
			m_ActiveVertexBufferIndex = 0;
		}
		else
		{
			// Round up to a multiple of the stride
			if (chunk->main.stride > 0)
				m_VertexPos.writeOffset = RoundUpMultiple (m_VertexPos.writeOffset, chunk->main.stride);

			// Add new buffer chunks in circular mode
			if (m_VertexPos.writeOffset + size >= m_VertexBuffers[m_ActiveVertexBufferIndex]->GetBufferSize())
			{
				m_ActiveVertexBufferIndex++;
				m_VertexPos.writeOffset = 0;
			}
		}
	}
	else
	{
		// Every chunk uses a new VBO
		m_ActiveVertexBufferIndex++;
		m_VertexPos.writeOffset = 0;
	}

	if (m_VertexBuffers.size() <= m_ActiveVertexBufferIndex)
		m_VertexBuffers.push_back (m_Device.CreateVertexBuffer());

	if (!ReserveVertexBuffer (m_ActiveVertexBufferIndex, std::max (size, (UInt32)m_VertexBufferChunkSize)))
		return NULL;
	
	chunk->vb = m_VertexBuffers[m_ActiveVertexBufferIndex];
	return (UInt8*)m_Device.BeginBufferWrite(chunk->vb, m_VertexPos.writeOffset, size);
}

UInt8* GenericDynamicVBO::AllocateIB(UInt32 size, DynamicVBOChunkHandle& chunkHandle)
{
	GenericDynamicVBOChunk* chunk = (GenericDynamicVBOChunk*)HandleToChunk(chunkHandle, false);
	
	if (m_BufferMode == kGfxBufferModeCircular)
	{
		if (m_ActiveIndexBufferIndex == -1)
		{
			m_ActiveIndexBufferIndex = 0;
		}
		else
		{
			// Round up to a multiple of the stride
			if (chunk->main.stride > 0)
				m_IndexPos.writeOffset = RoundUpMultiple (m_IndexPos.writeOffset, chunk->main.stride);

			// Add new buffer chunks in circular mode
			if (m_IndexPos.writeOffset + size >= m_IndexBuffers[m_ActiveIndexBufferIndex]->GetBufferSize())
			{
				m_ActiveIndexBufferIndex++;
				m_IndexPos.writeOffset = 0;
			}
		}
	}
	else
	{
		// Every chunk uses a new VBO
		m_ActiveIndexBufferIndex++;
		m_IndexPos.writeOffset = 0;
	}

	if (m_IndexBuffers.size() <= m_ActiveIndexBufferIndex)
		m_IndexBuffers.push_back (m_Device.CreateIndexBuffer());

	if (!ReserveIndexBuffer (m_ActiveIndexBufferIndex, std::max (size, (UInt32)m_IndexBufferChunkSize)))
		return NULL;
	
	chunk->ib = m_IndexBuffers[m_ActiveIndexBufferIndex];
	return (UInt8*)m_Device.BeginBufferWrite(chunk->ib, m_IndexPos.writeOffset, size);
}

void GenericDynamicVBO::DrawChunkInternal(const DynamicVBOChunkHandle& chunkHandle, const ChannelAssigns& channels, UInt32 channelsMask, VertexDeclaration* vertexDecl, DrawBuffersRange* ranges, int numDrawRanges, UInt32 stride)
{
	GenericDynamicVBOChunk* chunk = (GenericDynamicVBOChunk*)HandleToChunk(chunkHandle, false);

	MeshBuffers buffers;
	buffers.Reset();
	buffers.vertexStreams[0].buffer = chunk->vb;
	buffers.vertexStreams[0].stride = stride;
	buffers.vertexStreamCount = 1;
	buffers.vertexDecl = vertexDecl;

	UInt32 maxVertexRangeEnd = 0;

	for (int i = 0; i < numDrawRanges; i++)
	{
		if (chunk->ib)
		{
			ranges[i].firstIndexByte += chunk->indexDrawOffset;
			ranges[i].baseVertex = (chunk->vertexDrawOffset + stride - 1 + ranges[i].baseVertex) / stride;
		}
		else
		{
			ranges[i].firstVertex = (chunk->vertexDrawOffset + stride - 1 + ranges[i].firstVertex) / stride;
		}

		maxVertexRangeEnd = std::max(maxVertexRangeEnd, ranges[i].baseVertex + ranges[i].firstVertex + ranges[i].vertexCount);
	}

	AddDefaultStreamsToMeshBuffers(m_Device, buffers, maxVertexRangeEnd, channels.GetSourceMap(), channelsMask);

	m_Device.DrawBuffers(chunk->ib, buffers.vertexStreams, buffers.vertexStreamCount, ranges, numDrawRanges, vertexDecl, channels);
}

void GenericDynamicVBO::ReleaseChunkInternal (const DynamicVBOChunkHandle& chunkHandle, UInt32 actualVertices, UInt32 actualIndices)
{
	GenericDynamicVBOChunk* chunk = (GenericDynamicVBOChunk*)HandleToChunk(chunkHandle, false);

	UInt32 vertexBytes = actualVertices * chunk->main.stride;
	UInt32 indexBytes = actualIndices * sizeof (UInt16);

	chunk->vertexDrawOffset = m_VertexPos.FinishWriting(vertexBytes);
	chunk->indexDrawOffset = m_IndexPos.FinishWriting(indexBytes);

	if (chunk->vb)
		m_Device.EndBufferWrite(chunk->vb, vertexBytes);

	if (chunk->ib)
		m_Device.EndBufferWrite(chunk->ib, indexBytes);
}

UInt32 GenericDynamicVBO::GetAllocateSize(const GfxBuffer* buffer, UInt32 size) const
{
	//@TODO: JOE. NEED PROPER CLEANUP OF THIS CODE
	//		Disable ShouldUseSysMem and never return system memory.
	//      * ON OS X mixing system memory and real VBO seems to hit an insanely slow path.
	//      * Dynamic VBO may not allocate / deallocate VBO's all the time since that will bring OS X to slow crawl.
	//        So make temporary gross workaround which makes rendering inspector reasonably fast and reallocate somewhat rarely.

	if (m_BufferMode == kGfxBufferModeDynamicForceSystemOnOSX && buffer->GetBufferSize() != size)
	{
		return size;
	}
	else if (m_BufferMode == kGfxBufferModeDynamic && buffer->GetBufferSize() != size)
	{
		return size;
	}
	else if (size > buffer->GetBufferSize())
	{
		// Allocate to next power of two if needed
		return NextPowerOfTwo(size);
	}
	

	return 0;
}

bool GenericDynamicVBO::ReserveVertexBuffer(UInt32 bufferIndex, UInt32 size)
{
	GfxBuffer* buffer = m_VertexBuffers[bufferIndex];
	UInt32 updateSize = GetAllocateSize(buffer, size);
	if (updateSize > 0)
		m_Device.UpdateBuffer(buffer, m_BufferMode, kGfxBufferLabelInternal, updateSize, NULL, 0);

	return size <= buffer->GetBufferSize();
}

bool GenericDynamicVBO::ReserveIndexBuffer(UInt32 bufferIndex, UInt32 size)
{
	GfxBuffer* buffer = m_IndexBuffers[bufferIndex];
	UInt32 updateSize = GetAllocateSize(buffer, size);
	if (updateSize > 0)
		m_Device.UpdateBuffer(buffer, m_BufferMode, kGfxBufferLabelInternal, updateSize, NULL, 0);

	return size <= buffer->GetBufferSize();
}
