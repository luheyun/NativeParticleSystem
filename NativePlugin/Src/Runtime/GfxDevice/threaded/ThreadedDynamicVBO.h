#ifndef THREADEDDYNAMICVBO_H
#define THREADEDDYNAMICVBO_H

#include "Runtime/GfxDevice/GfxDeviceConfigure.h"
#include "Runtime/Graphics/Mesh/DynamicVBO.h"

class GfxDeviceClient;

class ThreadedDynamicVBO : public DynamicVBO
{
public:
	ThreadedDynamicVBO (GfxDeviceClient& device);
	virtual ~ThreadedDynamicVBO ();

	virtual bool GetChunk (UInt32 stride, UInt32 maxVertices, UInt32 maxIndices, GfxPrimitiveType primType, DynamicVBOChunkHandle* outHandle);
	
	virtual void ReleaseChunk (DynamicVBOChunkHandle& vboChunk, UInt32 actualVertices, UInt32 actualIndices);

	virtual void DrawChunk (const DynamicVBOChunkHandle& chunkHandle, const ChannelAssigns& channels, UInt32 channelsMask, VertexDeclaration* vertexDecl, const DrawParams* params = NULL, int numDrawParams = 0);

	void SetOwningThread(Thread::ThreadID id) { m_OwningThreadId = id; }
private:
	virtual UInt8* AllocateVB (UInt32 size, DynamicVBOChunkHandle& chunkHandle) { return NULL; }
	virtual UInt8* AllocateIB (UInt32 size, DynamicVBOChunkHandle& chunkHandle) { return NULL; }
	virtual void DrawChunkInternal(const DynamicVBOChunkHandle& chunkHandle, const ChannelAssigns& channels, UInt32 channelsMask, VertexDeclaration* vertexDecl, DrawBuffersRange* ranges, int numDrawRanges, UInt32 stride) {}
	virtual void ReleaseChunkInternal(const DynamicVBOChunkHandle& chunkHandle, UInt32 actualVertices, UInt32 actualIndices) {}

	void DoLockstep();

	GfxDeviceClient& m_ClientDevice;
	dynamic_array<UInt8, 32> m_ChunkVertices;
	dynamic_array<UInt16, 32> m_ChunkIndices;
	UInt32 m_ActualVertices;
	UInt32 m_ActualIndices;
	UInt32 m_LastStride;
	GfxPrimitiveType m_LastPrimitiveType;
	bool m_MappedChunk;
	Thread::ThreadID m_OwningThreadId;
};

#endif
