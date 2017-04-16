#include "PluginPrefix.h"
#include "Runtime/GfxDevice/threaded/GfxDeviceClient.h"
#include "Runtime/GfxDevice/threaded/GfxDeviceWorker.h"
#include "Runtime/GfxDevice/threaded/GfxCommands.h"
#include "Runtime/GfxDevice/threaded/ThreadedBuffer.h"
#include "Runtime/Threads/ThreadedStreamBuffer.h"
#include "ThreadedDynamicVBO.h"

ThreadedDynamicVBO::ThreadedDynamicVBO (GfxDeviceClient& device)
: m_ClientDevice(device)
, m_ChunkVertices(kMemTempAlloc)
, m_ChunkIndices(kMemTempAlloc)
, m_ActualVertices(0)
, m_ActualIndices(0)
, m_LastStride(0)
, m_LastPrimitiveType(kPrimitiveTriangles)
, m_MappedChunk(false)
{
	//	Main thread's owning ID is fixed here and jobs reset it in ExecuteAsyncSetup when we map a worker thread to a GfxDeviceClient
	m_OwningThreadId = Thread::GetCurrentThreadID();
}

ThreadedDynamicVBO::~ThreadedDynamicVBO ()
{
}

bool ThreadedDynamicVBO::GetChunk (UInt32 stride, UInt32 maxVertices, UInt32 maxIndices, GfxPrimitiveType primType, DynamicVBOChunkHandle* outHandle)
{
	bool result = true;
	if (!m_ClientDevice.IsSerializing())
	{
		// Single-threaded mode
		DynamicVBO& vbo = GetRealGfxDevice().GetDynamicVBO();
		return vbo.GetChunk(stride, maxVertices, maxIndices, primType, outHandle);
	}
	
	m_LastStride = stride;
	m_LastPrimitiveType = primType;
	
	*outHandle = AllocateHandle();

	if (maxVertices > 0)
	{
		m_ChunkVertices.resize_uninitialized(stride * maxVertices);
		outHandle->vbPtr = m_ChunkVertices.data();
		result = result && (outHandle->vbPtr ? true : false);
	}
	if (maxIndices > 0)
	{
		m_ChunkIndices.resize_uninitialized(maxIndices);
		outHandle->ibPtr = m_ChunkIndices.data();
		result = result && (outHandle->ibPtr ? true : false);
	}

	//DebugAssert(!m_MappedChunk);
	m_MappedChunk = result;
	return result;
}

void ThreadedDynamicVBO::ReleaseChunk (DynamicVBOChunkHandle& chunkHandle, UInt32 actualVertices, UInt32 actualIndices)
{
	if (!m_ClientDevice.IsSerializing())
	{
		// Single-threaded mode
		DynamicVBO& vbo = GetRealGfxDevice().GetDynamicVBO();
		vbo.ReleaseChunk(chunkHandle, actualVertices, actualIndices);
		return;
	}
	//DebugAssert(m_MappedChunk);
	m_ActualVertices = actualVertices;
	m_ActualIndices = actualIndices;
	m_MappedChunk = false;
	const bool validChunk = (actualVertices > 0) && (m_ChunkIndices.empty() || actualIndices > 0);
	if (!validChunk)
	{
		m_ChunkVertices.clear();
		m_ChunkIndices.clear();
		return;
	}
	//Assert(actualVertices * m_LastStride <= m_ChunkVertices.size());
	//Assert(actualIndices <= m_ChunkIndices.size());
	ThreadedStreamBuffer& commandQueue = *m_ClientDevice.GetCommandQueue();
	commandQueue.WriteValueType<GfxCommand>(kGfxCmd_DynVBO_Chunk);
	GfxCmdDynVboChunk chunk = { m_LastStride, actualVertices, actualIndices, m_LastPrimitiveType, chunkHandle };
	commandQueue.WriteValueType<GfxCmdDynVboChunk>(chunk);
	commandQueue.WriteStreamingData(m_ChunkVertices.data(), actualVertices * m_LastStride);
	if (actualIndices > 0)
		commandQueue.WriteStreamingData(m_ChunkIndices.data(), actualIndices * kVBOIndexSize);
	commandQueue.WriteSubmitData();
	// Using temp allocator so we have to clear
	m_ChunkVertices.clear();
	m_ChunkIndices.clear();
	GFXDEVICE_LOCKSTEP_CLIENT();
}

void ThreadedDynamicVBO::DrawChunk (const DynamicVBOChunkHandle& chunkHandle, const ChannelAssigns& channels, UInt32 channelsMask, VertexDeclaration* vertexDecl, const DrawParams* params, int numDrawParams)
{
	if (!m_ClientDevice.IsSerializing())
	{
		// Single-threaded mode
		DynamicVBO& vbo = GetRealGfxDevice().GetDynamicVBO();
		vbo.DrawChunk(chunkHandle, channels, channelsMask, GetRealDecl(&GetRealGfxDevice(), vertexDecl), params, numDrawParams);
		return;
	}
	//DebugAssert(!m_MappedChunk);

	m_ClientDevice.BeforeDrawCall();

	// override counts, for drawing a subset of the chunk
	DrawParams defaultParams;
	if (!params || numDrawParams == 0)
	{
		defaultParams.vertexCount = m_ActualVertices;
		defaultParams.indexCount = m_ActualIndices;
		defaultParams.stride = m_LastStride;
		params = &defaultParams;
		numDrawParams = 1;
	}

	if (params->vertexCount == 0)
		return;

	// restore default values of NULL/0 before sending the commands through
	if (params == &defaultParams)
	{
		params = NULL;
		numDrawParams = 0;
	}

	ThreadedStreamBuffer& commandQueue = *m_ClientDevice.GetCommandQueue();
	commandQueue.WriteValueType<GfxCommand>(kGfxCmd_DynVBO_DrawChunk);
	GfxCmdDynVboDrawChunk chunk = { channels, channelsMask, vertexDecl, chunkHandle, numDrawParams };
	commandQueue.WriteValueType<GfxCmdDynVboDrawChunk>(chunk);
	commandQueue.WriteArrayType<DynamicVBO::DrawParams>(params, numDrawParams);
	commandQueue.WriteSubmitData();
	GFXDEVICE_LOCKSTEP_CLIENT();
}

void ThreadedDynamicVBO::DoLockstep()
{
	m_ClientDevice.DoLockstep();
}

