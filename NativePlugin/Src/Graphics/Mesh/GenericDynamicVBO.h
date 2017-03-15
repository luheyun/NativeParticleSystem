#pragma once

#include "DynamicVBO.h"

class GfxDevice;

class GenericDynamicVBO : public DynamicVBO
{
public:
	GenericDynamicVBO(GfxDevice& device, GfxBufferMode bufferMode = kGfxBufferModeDynamic, UInt32 initialVBSize = 0, UInt32 initialIBSize = 0);
	virtual ~GenericDynamicVBO();

	virtual void SwapBuffers(UInt16 frameIndex);

	virtual DynamicVBOChunk* HandleToChunk(const DynamicVBOChunkHandle& chunkHandle, bool createIfNotExist = true);

private:
	virtual UInt8* AllocateVB(UInt32 size, DynamicVBOChunkHandle& chunkHandle);
	virtual UInt8* AllocateIB(UInt32 size, DynamicVBOChunkHandle& chunkHandle);
	virtual void DrawChunkInternal(const DynamicVBOChunkHandle& chunkHandle, const ChannelAssigns& channels, UInt32 channelsMask, VertexDeclaration* vertexDecl, DrawBuffersRange* ranges, int numDrawRanges, UInt32 stride);
	virtual void ReleaseChunkInternal(const DynamicVBOChunkHandle& chunkHandle, UInt32 actualVertices, UInt32 actualIndices);

private:
	UInt32 GetAllocateSize (const GfxBuffer* buffer, UInt32 size) const;
	bool ReserveVertexBuffer(UInt32 bufferIndex, UInt32 size);
	bool ReserveIndexBuffer(UInt32 bufferIndex, UInt32 size);

	struct BufferPositions
	{
		// Buffer offset for circular mode
		BufferPositions () : writeOffset(0) {}
		UInt32 FinishWriting (UInt32 bytes) { UInt32 drawOffset = writeOffset; writeOffset += bytes; return drawOffset; }
		UInt32 writeOffset;
	};

	GfxDevice& m_Device;
	GfxBufferMode m_BufferMode;

	std::vector<GfxBuffer*> m_VertexBuffers;
	std::vector<GfxBuffer*> m_IndexBuffers;

	BufferPositions m_VertexPos;
	BufferPositions m_IndexPos;

	size_t m_ActiveVertexBufferIndex;
	size_t m_ActiveIndexBufferIndex;

	const size_t m_VertexBufferChunkSize;
	const size_t m_IndexBufferChunkSize;
};
