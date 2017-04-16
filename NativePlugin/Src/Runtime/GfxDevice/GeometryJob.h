#pragma once

#include "Graphics/Mesh/DynamicVBO.h"
#include "Utilities/UniqueIDGenerator.h"
#include "Jobs/JobTypes.h"
#include "Jobs/Jobs.h"

class GfxDevice;
class GfxBuffer;

/*
// GeometryJobs need to inherit from GeometryJobData
struct MyGeometryJob : GeometryJobData
{
	float vertexValue;
	UInt32 bufferSize;
};

void ModifyVerticesJob (MyGeometryJob* job)
{
	// When mapping the vertex buffer fails, the job will still be run in order to deallocate the job memory.
	// Thus this check is necessary in every job.
	if (job.mappedVertexData != NULL)
	{
		memset (job.mappedVertexData, job->vertexValue, job->bufferSize);
	}

	// Just like ScheduleJob, it is the responsibility of the job to deallocate user data
	UNITY_DELETE(job, kMemTempJobAlloc);
}

// Sometime... the earlier in the frame the better
// Skinning will remember what characters were visible last frame and will start skinning after LateUpdate
// SkinnedMeshes that entered the viewport (OnBecameVisible) are skinned "on the fly" after culling.
void MyObject::Update ()
{
	// You must allocate as many fences as you will schedule jobs.
	m_GeometryJobFence = device.AllocateGeometryJob ();
	
	MyGeometryJob* job = UNITY_NEW (MyGeometryJob, kMemTempJobAlloc);
	job->vertexValue = 5.0F;
	job->bufferSize = m_VertexBuffer.GetBufferSize();

	GeometryJobInstruction instruction (m_GeometryJobFence, job, m_VertexBuffer, 0, m_VertexBuffer.GetBufferSize());

	// Note: For performance reasons,
	// you should scheddule geometry jobs all at once instead of one by one
	// So usually you have one loop that colelcts all instructions and then call ScheduleGeometryJobs once.
	device.ScheduleGeometryJobs (ModifyVerticesJob, &GeometryJobInstruction (myVertexBuffer), 1);
}

// Before rendering any object with the VertexBuffer or destroying the VertexBuffer.
void MyObject::Render ()
{
	// Ensure that the Wait for geometry job has completed on the render thread.
	// It does NOT wait on main thread.
	device.PutGeometryJobFence (m_GeometryJobFence);

	// Important note: All data in MyGeometryJob must be constant until the job destroys it.
	// PutGeometryJobFence does not wait on main thread,
	// thus it is not safe to modify data referenced by the geometry job after PutGeometryJobFence is called.

	// Draw with vertex buffer
	...
}
*/


typedef UniqueSmallID GeometryJobFence;
struct GeometryJobData
{
	void* mappedVertexData;
	void* mappedIndexData;
	UInt32 numVertices;
	UInt32 numIndices;
	GeometryJobData() : mappedVertexData (NULL), mappedIndexData (NULL), numVertices (0), numIndices (0) { }
};

typedef void GeometryJobFunc(GeometryJobData* userData);

struct VertexInfo
{
	GfxBuffer* buffer;
	UInt32	mappedOffset;
	UInt32	mappedSize;
	VertexInfo(GfxBuffer* buf, UInt32 ms, UInt32 mo) : buffer(buf), mappedOffset(mo), mappedSize(ms) {}
	VertexInfo() : buffer(NULL), mappedOffset(0), mappedSize(0) {}
};

struct GeometryJobInstruction
{
	GeometryJobFence		fence;
	GeometryJobData*		userData;
	JobFence				dependsOnTask;
	UInt32					dynamicVBOStride;
	UInt32					dynamicVBONumVertices;
	UInt32					dynamicVBONumIndices;

	VertexInfo				vertexInfo;
	VertexInfo				indexInfo;

	// custom VBO
	GeometryJobInstruction(GeometryJobFence i_Fence, GeometryJobData* i_UserData, GfxBuffer* i_Buffer, UInt32 i_Offset, UInt32 i_MappedSize)
		: fence(i_Fence)
		, userData(i_UserData)
		, dynamicVBOStride (0)
		, dynamicVBONumVertices (0)
		, dynamicVBONumIndices (0)
		, vertexInfo(i_Buffer, i_MappedSize, i_Offset)
	{
	}

	GeometryJobInstruction(
		GeometryJobFence i_Fence, GeometryJobData* i_UserData,
		GfxBuffer* v_Buffer, UInt32 v_Offset, UInt32 v_MappedSize,
		GfxBuffer* i_Buffer, UInt32 i_Offset, UInt32 i_MappedSize)
		: fence(i_Fence)
		, userData(i_UserData)
		, dynamicVBOStride (0)
		, dynamicVBONumVertices (0)
		, dynamicVBONumIndices (0)
		, vertexInfo(v_Buffer, v_MappedSize, v_Offset)
		, indexInfo(i_Buffer, i_MappedSize, i_Offset)
	{
	}

	// dynamic VBO
	GeometryJobInstruction (GeometryJobFence i_Fence, GeometryJobData* i_UserData, UInt32 i_Stride, UInt32 i_NumVertices, UInt32 i_NumIndices)
		: fence (i_Fence)
		, userData (i_UserData)
		, dynamicVBOStride (i_Stride)
		, dynamicVBONumVertices (i_NumVertices)
		, dynamicVBONumIndices (i_NumIndices)
	{
	}

	GeometryJobInstruction()
		: userData (NULL)
		, dynamicVBOStride (0)
		, dynamicVBONumVertices (0)
		, dynamicVBONumIndices (0)
	{
	}

	~GeometryJobInstruction()
	{
		ClearFenceWithoutSync(dependsOnTask);
	}

	void operator = (const GeometryJobInstruction& ji)
	{
		ClearFenceWithoutSync(dependsOnTask);
		fence = ji.fence;
		userData = ji.userData;
		dependsOnTask = ji.dependsOnTask;
		vertexInfo = ji.vertexInfo;
		indexInfo = ji.indexInfo;
		dynamicVBOStride = ji.dynamicVBOStride;
		dynamicVBONumVertices = ji.dynamicVBONumVertices;
		dynamicVBONumIndices = ji.dynamicVBONumIndices;
	}
};

class GeometryJobTasks
{
	struct GeometryJobTask
	{
		GfxBuffer*			vertexBuffer;
		GfxBuffer*			indexBuffer;
		UInt32				writtenVertexBytes;
		UInt32				writtenIndexBytes;
		JobFence			fence;
		UInt32				dynamicVBOTotalVertices;
		UInt32				dynamicVBOTotalIndices;
		GeometryJobTask() : vertexBuffer(NULL), indexBuffer(NULL), writtenVertexBytes(0), writtenIndexBytes(0), dynamicVBOTotalVertices(0), dynamicVBOTotalIndices(0) { }
	};

	dynamic_array<GeometryJobTask>	m_Tasks;
	DynamicVBOChunkHandle			m_DynamicVBOChunk;

public:

	GeometryJobTasks();

	void EndGeometryJobFrame(GfxDevice& device);
	void PutGeometryJobFence(GfxDevice& device, UInt32 index);
	void ScheduleGeometryJobs(GfxDevice& device, GeometryJobFunc* geometryJobFunc, const GeometryJobInstruction* geometryJobData, UInt32 geometryJobDataCount, bool isThreadedBuffer);
	void ScheduleDynamicVBOGeometryJobs(GfxDevice& device, GeometryJobFunc* geometryJobFunc, GeometryJobInstruction* geometryJobData, UInt32 geometryJobDataCount, GfxPrimitiveType primType, DynamicVBOChunkHandle* outChunk);

private:

	static bool CompareJobs (const GeometryJobInstruction& left, const GeometryJobInstruction& right);
};
