#include "PluginPrefix.h"
#include "DynamicVBO.h"

UInt32 DynamicVBO::s_CurrentRenderThreadChunkId = 0;


DynamicVBO::DynamicVBO()
{

}

bool DynamicVBO::GetChunk(UInt32 stride, UInt32 maxVertices, UInt32 maxIndices, GfxPrimitiveType primType, DynamicVBOChunkHandle* outHandle)
{
	bool result = true;

	// threaded rendering may have already filled this in
	//if (IsHandleValid(*outHandle) == false)
	//{
	//	*outHandle = AllocateHandle();
	//}

	DynamicVBOChunk* chunk = HandleToChunk(*outHandle);
	chunk->stride = stride;
	chunk->primitiveType = primType;
	chunk->indexed = (maxIndices > 0);
	chunk->writtenVertices = maxVertices;
	chunk->writtenIndices = maxIndices;

	UInt32 requiredVBSize = maxVertices * stride;
	UInt32 requiredIBSize = maxIndices * sizeof(UInt16);

	//if (primType == kPrimitiveQuads && !GetGraphicsCaps().hasNativeQuad)
	//{
	//	// Use memory buffer and convert to triangles
	//	m_QuadBuffer.resize_uninitialized(maxIndices);
	//	outHandle->ibPtr = m_QuadBuffer.data();
	//	if (maxIndices)
	//		result = outHandle->ibPtr ? true : false;
	//	// Allocate IB space later
	//	requiredIBSize = 0;
	//}

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

void DynamicVBO::SwapBuffers(UInt16 frameIndex)
{
	s_CurrentRenderThreadChunkId = 0;
}