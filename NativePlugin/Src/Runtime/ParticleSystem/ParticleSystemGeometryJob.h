#pragma once

#include "Runtime/GfxDevice/GeometryJob.h"

class ParticleSystemGeometryJob : public GeometryJobData
{
public:
    friend class ParticleSystemRenderer;

    ParticleSystemGeometryJob(UInt32 numParticles, bool needsSheet, bool needs3DSize) : m_ParticlesData(numParticles, needsSheet, needs3DSize), m_Particles(NULL) {}
    static void ScheduleJobs(ParticleSystem** particleSystems, size_t count, const Matrix4x4f& worldToCameraMatrix);

private:
    ParticleSystemParticlesTempData m_ParticlesData;
    ParticleSystemParticles* m_Particles;
};