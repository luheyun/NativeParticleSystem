#include "PluginPrefix.h"
#include "ParticleSystem.h"
#include "ParticleSystemRenderer.h"
#include "ParticleSystemGeometryJob.h"
#include "Runtime/GfxDevice/GfxDevice.h"


void ParticleSystemGeometryJob::ScheduleJobs(ParticleSystem** particleSystems, size_t count, const Matrix4x4f& worldToCameraMatrix)
{
    int jobsCount = 0;
    int meshJobsCount = 0;
    GeometryJobInstruction* geometryJobInstructions;
    GeometryJobInstruction* geometryJobMeshInstructions;
    ParticleSystemRenderer::DrawCallData** geometryJobDrawCallData;
    ParticleSystemRenderer::DrawCallData** geometryJobMeshDrawCallData;
    ALLOC_TEMP(geometryJobInstructions, GeometryJobInstruction, count + 1);
    ALLOC_TEMP(geometryJobMeshInstructions, GeometryJobInstruction, count);
    ALLOC_TEMP(geometryJobDrawCallData, ParticleSystemRenderer::DrawCallData*, count + 1);
    ALLOC_TEMP(geometryJobMeshDrawCallData, ParticleSystemRenderer::DrawCallData*, count);

    GfxDevice& device = GetGfxDevice();
    DynamicVBO& vbo = device.GetDynamicVBO();

    GeometryJobFence geometryJobFence = device.CreateGeometryJobFence();
    GeometryJobFence geometryJobMeshFence = device.CreateGeometryJobFence();

    UInt32 billboardVertexOffsetBytes = 0;
    UInt32 meshVertexOffsetBytes = 0;
    UInt32 meshIndexOffsetBytes = 0;
    UInt32 maxBillboardParticles = 0;

    for (size_t i = 0; i < count; i++)
    {
        ParticleSystem* system = particleSystems[i];
        ParticleSystemRenderer* renderer = system->GetRenderer();
        ParticleSystemParticles* ps = system ? &system->GetParticles() : NULL;
        const UInt32 numParticles = ps ? ps->array_size() : 0;

        ParticleSystemRenderer::DrawCallData& drawCallData = renderer->m_DrawCallData;
        drawCallData.m_NumPaticles = numParticles;

        if (renderer->m_Data->renderMode == kSRMMesh)
        {
            // todo
        }

        if (numParticles)
        {
            ParticleSystemGeometryJob* geometryJob = UNITY_NEW(ParticleSystemGeometryJob, kMemTempJobAlloc)(numParticles, system->m_UVModule->GetEnabled(), ps->uses3DSize);
        }
    }
}
