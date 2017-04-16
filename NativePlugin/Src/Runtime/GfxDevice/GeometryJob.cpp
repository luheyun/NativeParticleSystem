#include "PluginPrefix.h"
#include "GeometryJob.h"
#include "GfxDevice.h"
#include "threaded/ThreadedBuffer.h"
#include "Jobs/JobBatchDispatcher.h"
#include "Utilities/Utility.h"

GeometryJobTasks::GeometryJobTasks()
	: m_Tasks (kMemGfxDevice)
{

}

void GeometryJobTasks::EndGeometryJobFrame(GfxDevice& device)
{
	for (size_t i = 0; i < m_Tasks.size(); i++)
		PutGeometryJobFence(device, i);
}

void GeometryJobTasks::PutGeometryJobFence(GfxDevice& device, UInt32 index)
{
	GeometryJobTask& task = m_Tasks[index];

	if (task.vertexBuffer || task.indexBuffer)
	{
		// Complete job and Unmap vertex buffer
		SyncFence(task.fence);

		if (task.vertexBuffer != NULL)
			device.EndBufferWrite(task.vertexBuffer, task.writtenVertexBytes);

		if (task.indexBuffer != NULL)
			device.EndBufferWrite(task.indexBuffer, task.writtenIndexBytes);

		task.vertexBuffer = NULL;
		task.indexBuffer = NULL;
	}
	else if (task.dynamicVBOTotalVertices || task.dynamicVBOTotalIndices)
	{
		// once all jobs are complete, unmap the VBO
		SyncFence (task.fence);
		
		DynamicVBO& vbo = device.GetDynamicVBO();
		if (vbo.IsHandleValid(m_DynamicVBOChunk))
		{
			vbo.ReleaseChunk (m_DynamicVBOChunk, task.dynamicVBOTotalVertices, task.dynamicVBOTotalIndices);
			m_DynamicVBOChunk = DynamicVBOChunkHandle();
		}

		task.dynamicVBOTotalVertices = 0;
		task.dynamicVBOTotalIndices = 0;
	}
}

void GeometryJobTasks::ScheduleGeometryJobs(GfxDevice& device, GeometryJobFunc* geometryJobFunc, const GeometryJobInstruction* geometryJobDatas, UInt32 geometryJobDataCount, bool isThreadedBuffer)
{
	#if ENABLE_MULTITHREADED_CODE
	JobBatchDispatcher dispatch(kNormalJobPriority, 64);
	#endif

	m_Tasks.reserve(128);

	for (UInt32 i = 0; i < geometryJobDataCount; ++i)
	{
		const GeometryJobInstruction& geometryJob = geometryJobDatas[i];
		UInt32 index = geometryJob.fence.index;

		// Ensure m_Tasks array is big enough
		UInt32 minSize = index+1;
		if (minSize > m_Tasks.size())
			m_Tasks.resize_initialized(minSize, GeometryJobTask(), true);

		GeometryJobTask& task = m_Tasks[index];

		if (geometryJob.vertexInfo.buffer != NULL)
		{
			GfxBuffer* buff = geometryJob.vertexInfo.buffer;
			
			if (isThreadedBuffer)
				buff = GetRealBuffer(geometryJob.vertexInfo.buffer);
			
			void* mappedBuf = device.BeginBufferWrite(buff, geometryJob.vertexInfo.mappedOffset, geometryJob.vertexInfo.mappedSize);
			
			if (mappedBuf != NULL)
			{
				task.vertexBuffer = buff;
				task.writtenVertexBytes = geometryJob.vertexInfo.mappedSize;
			}

			geometryJob.userData->mappedVertexData = mappedBuf;
			geometryJob.userData->numVertices = geometryJob.vertexInfo.mappedSize;
		}

		if (geometryJob.indexInfo.buffer != NULL)
		{
			GfxBuffer* buff = geometryJob.indexInfo.buffer;

			if (isThreadedBuffer)
				buff = GetRealBuffer(geometryJob.indexInfo.buffer);

			void* mappedBuf = device.BeginBufferWrite(buff, geometryJob.indexInfo.mappedOffset, geometryJob.indexInfo.mappedSize);

			if (mappedBuf != NULL)
			{
				task.indexBuffer = buff;
				task.writtenIndexBytes = geometryJob.indexInfo.mappedSize;
			}

			geometryJob.userData->mappedIndexData = mappedBuf;
			geometryJob.userData->numIndices = geometryJob.indexInfo.mappedSize;
		}
		
	#if ENABLE_MULTITHREADED_CODE
		dispatch.ScheduleJobDepends(task.fence, geometryJobFunc, geometryJob.userData, geometryJob.dependsOnTask);
	#else
		// Sync dependent job since in the next line, we will have to do the job func
		JobFence dependsOn = geometryJob.dependsOnTask;
		SyncFence(dependsOn);
		// call this only after the dependent task is synced
		geometryJobFunc(geometryJob.userData);
		// now do the sync
		PutGeometryJobFence(device, index);
	#endif
	}
}

void GeometryJobTasks::ScheduleDynamicVBOGeometryJobs(GfxDevice& device, GeometryJobFunc* geometryJobFunc, GeometryJobInstruction* geometryJobDatas, UInt32 geometryJobDataCount, GfxPrimitiveType primType, DynamicVBOChunkHandle* outChunk)
{
	#if ENABLE_MULTITHREADED_CODE
	Job* jobs;
	ALLOC_TEMP(jobs, Job, geometryJobDataCount);
	#endif

	m_Tasks.reserve(128);

	// find the lowest common multiple of all strides, to be used as our stride
	UInt32 baseStride = geometryJobDatas[0].dynamicVBOStride;
	for (size_t i = 1; i < geometryJobDataCount; ++i)
	{
		if (baseStride != geometryJobDatas[i].dynamicVBOStride)
			baseStride = LeastCommonMultiple(baseStride, geometryJobDatas[i].dynamicVBOStride);
	}

	// count data
	UInt32 numVertexBytes = 0;
	UInt32 numIndices = 0;
	for (UInt32 i = 0; i < geometryJobDataCount; ++i)
	{
		numVertexBytes = RoundUpMultiple (numVertexBytes, geometryJobDatas[i].dynamicVBOStride);
		numVertexBytes += geometryJobDatas[i].dynamicVBOStride * geometryJobDatas[i].dynamicVBONumVertices;
		numIndices += geometryJobDatas[i].dynamicVBONumIndices;
	}
	UInt32 numVertices = RoundUpMultiple (numVertexBytes, baseStride) / baseStride;

	// reserve chunk for all jobs in the dynamic VBO
	if (numVertices || numIndices)
	{
		DynamicVBO& vbo = device.GetDynamicVBO();
		vbo.GetChunk (baseStride, numVertices, numIndices, primType, outChunk);
	}
	m_DynamicVBOChunk = *outChunk;

	// Only the first job is used for synchronisation
	UInt32 index = geometryJobDatas[0].fence.index;

	// Ensure m_Tasks array is big enough
	UInt32 minSize = index+1;
	if (minSize > m_Tasks.size())
		m_Tasks.resize_initialized(minSize, GeometryJobTask(), true);

	GeometryJobTask& task = m_Tasks[index];

	// If mapping the buffer fails, we still need to invoke the job.
	// The job is responsible for potentially releasing the memory.
	if (m_DynamicVBOChunk.vbPtr || m_DynamicVBOChunk.ibPtr)
	{
		task.dynamicVBOTotalVertices = numVertices;
		task.dynamicVBOTotalIndices = numIndices;
	}

	size_t vertexDataOffset = 0;
	size_t indexDataOffset = 0;
	for (UInt32 i = 0; i < geometryJobDataCount; ++i)
	{
		const GeometryJobInstruction& geometryJob = geometryJobDatas[i];
		
		if (m_DynamicVBOChunk.vbPtr || m_DynamicVBOChunk.ibPtr)
		{
			// round up to a multiple of the stride
			vertexDataOffset = RoundUpMultiple (vertexDataOffset, (size_t)geometryJob.dynamicVBOStride);
		}

		geometryJob.userData->mappedVertexData = geometryJob.dynamicVBONumVertices ? m_DynamicVBOChunk.vbPtr + vertexDataOffset : NULL;
		geometryJob.userData->mappedIndexData = geometryJob.dynamicVBONumIndices ? m_DynamicVBOChunk.ibPtr + indexDataOffset : NULL;
		geometryJob.userData->numVertices = geometryJob.dynamicVBONumVertices;
		geometryJob.userData->numIndices = geometryJob.dynamicVBONumIndices;
		vertexDataOffset += geometryJob.dynamicVBOStride * geometryJob.dynamicVBONumVertices;
		indexDataOffset += geometryJob.dynamicVBONumIndices;

		#if ENABLE_MULTITHREADED_CODE
		jobs[i] = Job(geometryJobFunc, geometryJob.userData);
		#else
		geometryJobFunc(geometryJob.userData);
		#endif
	}

	#if ENABLE_MULTITHREADED_CODE
	ScheduleDifferentJobsConcurrent(task.fence, jobs, geometryJobDataCount);
	#endif
}

bool GeometryJobTasks::CompareJobs (const GeometryJobInstruction& left, const GeometryJobInstruction& right)
{
	return left.userData->numVertices > right.userData->numVertices;
}
