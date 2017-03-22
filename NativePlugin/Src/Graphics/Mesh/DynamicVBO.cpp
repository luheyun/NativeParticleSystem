#include "PluginPrefix.h"
#include "DynamicVBO.h"
#include "Shaders/GraphicsCaps.h"

UInt32 DynamicVBO::s_CurrentRenderThreadChunkId = 0;


DynamicVBO::DynamicVBO()
{

}

bool DynamicVBO::IsHandleValid(const DynamicVBOChunkHandle& chunkHandle) const
{
	if (chunkHandle.id == DynamicVBOChunkHandle::kInvalidId)
		return false;

	// todo
	// render thread chunks don't use the frame index, and we can only do this test when running on the main thread, otherwise s_FrameIndex might have advanced already
	//if (!m_RenderThread && !chunkHandle.renderThread)
	//{
	//	if (chunkHandle.frame != s_FrameIndex)
	//		return false;
	//}

	return true;
}

DynamicVBOChunkHandle DynamicVBO::AllocateHandle()
{
	return DynamicVBOChunkHandle(s_CurrentRenderThreadChunkId++, 0, true);
}

bool DynamicVBO::GetChunk(UInt32 stride, UInt32 maxVertices, UInt32 maxIndices, GfxPrimitiveType primType, DynamicVBOChunkHandle* outHandle)
{
	bool result = true;

	// threaded rendering may have already filled this in
	if (IsHandleValid(*outHandle) == false)
	{
		*outHandle = AllocateHandle();
	}

	DynamicVBOChunk* chunk = HandleToChunk(*outHandle);
	chunk->stride = stride;
	chunk->primitiveType = primType;
	chunk->indexed = (maxIndices > 0);
	chunk->writtenVertices = maxVertices;
	chunk->writtenIndices = maxIndices;

	UInt32 requiredVBSize = maxVertices * stride;
	UInt32 requiredIBSize = maxIndices * sizeof(UInt16);

	if (primType == kPrimitiveQuads && !GetGraphicsCaps().hasNativeQuad)
	{
		// Use memory buffer and convert to triangles
		m_QuadBuffer.resize(maxIndices);
		outHandle->ibPtr = m_QuadBuffer.data();
		if (maxIndices)
			result = outHandle->ibPtr ? true : false;
		// Allocate IB space later
		requiredIBSize = 0;
	}

	if (result && requiredVBSize)
	{
		outHandle->vbPtr = AllocateVB(requiredVBSize, *outHandle);
		result = outHandle->vbPtr ? true : false;
	}
	if (result && requiredIBSize)
	{
		outHandle->ibPtr = (UInt16*)AllocateIB(requiredIBSize, *outHandle);
		result = outHandle->ibPtr ? true : false;
	}

	if (!result)
	{
		ReleaseChunkInternal(*outHandle, 0, 0);
		outHandle->vbPtr = NULL;
		outHandle->ibPtr = NULL;
	}

	return result;
}

void DynamicVBO::ReleaseChunk(DynamicVBOChunkHandle& chunkHandle, UInt32 actualVertices, UInt32 actualIndices)
{
	DynamicVBOChunk* chunk = HandleToChunk(chunkHandle, false);
	chunk->writtenVertices = actualVertices;
	chunk->writtenIndices = actualIndices;

	if (chunk->primitiveType == kPrimitiveQuads && !GetGraphicsCaps().hasNativeQuad)
	{
		FillQuadIndexBuffer(chunkHandle);
		m_QuadBuffer.clear();
	}

	ReleaseChunkInternal(chunkHandle, chunk->writtenVertices, chunk->writtenIndices);
}

void DynamicVBO::DrawChunk(const DynamicVBOChunkHandle& chunkHandle, const ChannelAssigns& channels, UInt32 channelsMask, VertexDeclaration* vertexDecl, const DrawParams* params, int numDrawParams)
{
	DynamicVBOChunk* chunk = HandleToChunk(chunkHandle, false);

	// Just return if nothing to render
	if ((chunk->indexed && chunk->writtenIndices == 0) || (chunk->writtenVertices == 0))
		return;

	// override counts, for drawing a subset of the chunk
	DrawParams defaultParams;
	if (!params || numDrawParams == 0)
	{
		defaultParams.vertexCount = chunk->writtenVertices;
		defaultParams.indexCount = chunk->writtenIndices;
		defaultParams.stride = chunk->stride;
		params = &defaultParams;
		numDrawParams = 1;
	}

	DrawBuffersRange* drawRanges = new DrawBuffersRange[numDrawParams];
	//ALLOC_TEMP(drawRanges, DrawBuffersRange, numDrawParams);

	for (int i = 0; i < numDrawParams; i++)
	{
		drawRanges[i] = DrawBuffersRange();
		drawRanges[i].topology = chunk->primitiveType;
		drawRanges[i].vertexCount = params[i].vertexCount;

		if (chunk->indexed)
		{
			drawRanges[i].indexCount = params[i].indexCount;
			drawRanges[i].firstIndexByte = params[i].indexOffset;
			drawRanges[i].baseVertex = params[i].vertexOffset;
		}
		else
		{
			drawRanges[i].firstVertex = params[i].vertexOffset;
		}
	}

	DrawChunkInternal(chunkHandle, channels, channelsMask, vertexDecl, drawRanges, numDrawParams, params[0].stride);
}

void DynamicVBO::FillQuadIndexBuffer(DynamicVBOChunkHandle& chunkHandle)
{
	DynamicVBOChunk* chunk = HandleToChunk(chunkHandle);

	UInt32 quadCountX4 = chunk->indexed ? chunk->writtenIndices : chunk->writtenVertices;

	if (quadCountX4 == 0)
		return;

	UInt32 quadCount = quadCountX4 / 4; // will just ignore any "broken" quads (e.g. passed 7 vertices -> will result in 1 quad)
	UInt32 reqIBSize = quadCount * 6 * sizeof(UInt16);

	if (reqIBSize)
		chunkHandle.ibPtr = (UInt16*)AllocateIB(reqIBSize, chunkHandle);

	TranslateQuadIndexBufferToTriangleList(chunkHandle.ibPtr, chunk->indexed ? m_QuadBuffer.data() : nullptr, quadCountX4);

	chunk->primitiveType = kPrimitiveTriangles;
	chunk->writtenIndices = quadCount * 6;
	chunk->indexed = true;
}

void DynamicVBO::SwapBuffers(UInt16 frameIndex)
{
	s_CurrentRenderThreadChunkId = 0;
}

void TranslateQuadIndexBufferToTriangleList(UInt16* dest, const UInt16* source, size_t sourceCount)
{
	// source count might not be multiple of 4, in that case ignore the last "broken" quad
	sourceCount &= ~3; // rounds down to multiple of 4

	if (source != nullptr)
	{
		for (int i = 0; i < sourceCount; i += 4)
		{
			*dest++ = source[0];
			*dest++ = source[1];
			*dest++ = source[2];
			*dest++ = source[0];
			*dest++ = source[2];
			*dest++ = source[3];
			source += 4;
		}
	}
	else
	{
		for (int i = 0; i < sourceCount; i += 4)
		{
			*dest++ = i + 0;
			*dest++ = i + 1;
			*dest++ = i + 2;
			*dest++ = i + 0;
			*dest++ = i + 2;
			*dest++ = i + 3;
		}
	}
}