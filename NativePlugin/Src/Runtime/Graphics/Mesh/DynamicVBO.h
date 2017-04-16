#pragma once

#include "GfxDevice/GfxDeviceTypes.h"

class VertexDeclaration;
class ChannelAssigns;

struct DynamicVBOChunk
{
	bool				indexed;
	UInt32				stride;
	UInt32				writtenVertices;
	UInt32				writtenIndices;
	GfxPrimitiveType	primitiveType;

	DynamicVBOChunk()
		: indexed(false)
		, stride(0)
		, writtenVertices(0)
		, writtenIndices(0)
		, primitiveType(kPrimitiveInvalid)
	{
	}
};

struct EXPORT_COREMODULE DynamicVBOChunkHandle
{
	enum { kInvalidId = 0xffffffff };

	DynamicVBOChunkHandle() : vbPtr(NULL), ibPtr(NULL), id(kInvalidId), frame(0), renderThread(0) {}
	DynamicVBOChunkHandle(UInt32 _id, UInt32 _frame, bool _renderThread) : vbPtr(NULL), ibPtr(NULL), id(_id), frame(_frame), renderThread(_renderThread ? 1 : 0) {}

	UInt8*	vbPtr;
	UInt16*	ibPtr;
	UInt32	id;
	UInt32	frame : 31;
	UInt32	renderThread : 1;
};

class EXPORT_COREMODULE DynamicVBO
{
public:
	struct DrawParams
	{
		DrawParams() : stride(0), vertexOffset(0), vertexCount(0), indexOffset(0), indexCount(0) {}
		DrawParams(UInt32 _stride, UInt32 _vertexOffset, UInt32 _vertexCount, UInt32 _indexOffset, UInt32 _indexCount) : stride(_stride), vertexOffset(_vertexOffset), vertexCount(_vertexCount), indexOffset(_indexOffset), indexCount(_indexCount) {}

		UInt32 stride;
		UInt32 vertexOffset;
		UInt32 vertexCount;
		UInt32 indexOffset;
		UInt32 indexCount;
	};

public:
	bool IsHandleValid(const DynamicVBOChunkHandle& chunkHandle) const;
	DynamicVBOChunkHandle AllocateHandle();

	DynamicVBO();
	virtual ~DynamicVBO() {}

	// End-of-frame event
	virtual void SwapBuffers(UInt16 frameIndex);

	virtual DynamicVBOChunk* HandleToChunk(const DynamicVBOChunkHandle& chunkHandle, bool createIfNotExist = true) { return NULL; }

	// Gets a chunk of vertex/index buffer to write into.
	//
	// maxVertices/maxIndices is the capacity of the returned chunk; you have to pass actually used
	// amounts in ReleaseChunk afterwards.
	//
	// Returns false if can't obtain a chunk for whatever reason.
	virtual bool GetChunk(UInt32 stride, UInt32 maxVertices, UInt32 maxIndices, GfxPrimitiveType primType, DynamicVBOChunkHandle* outHandle);

	virtual void ReleaseChunk(DynamicVBOChunkHandle& chunkHandle, UInt32 actualVertices, UInt32 actualIndices);

	virtual void DrawChunk(const DynamicVBOChunkHandle& chunkHandle, const ChannelAssigns& channels, UInt32 channelsMask, VertexDeclaration* vertexDecl, const DrawParams* params = NULL, int numDrawParams = 0);

protected:
	virtual UInt8* AllocateVB(UInt32 size, DynamicVBOChunkHandle& chunkHandle) = 0;
	virtual UInt8* AllocateIB(UInt32 size, DynamicVBOChunkHandle& chunkHandle) = 0;
	virtual void DrawChunkInternal(const DynamicVBOChunkHandle& chunkHandle, const ChannelAssigns& channels, UInt32 channelsMask, VertexDeclaration* vertexDecl, DrawBuffersRange* ranges, int numDrawRanges, UInt32 stride) = 0;
	virtual void ReleaseChunkInternal(const DynamicVBOChunkHandle& chunkHandle, UInt32 actualVertices, UInt32 actualIndices) = 0;

protected:
	void FillQuadIndexBuffer(DynamicVBOChunkHandle& chunkHandle);

private:
	static UInt32 s_CurrentRenderThreadChunkId;

	std::vector<UInt16> m_QuadBuffer;
};

// Translates index buffer with quads into an index buffer with triangles (two triangles per quad).
// source index buffer can be null, then it is assumed to be implicit index buffer with 4 separate vertices for each quad.
// sourceCount is the index count in the source buffer (in case of implicit buffer, it's the vertex count).
// Normally sourceCount should be a multiple of 4. If it is not, then the last (broken) quad is not converted.
void TranslateQuadIndexBufferToTriangleList(UInt16* dest, const UInt16* source, size_t sourceCount);