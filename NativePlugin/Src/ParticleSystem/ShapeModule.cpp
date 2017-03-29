#include "PluginPrefix.h"
#include "ShapeModule.h"

ShapeModule::ShapeModule() : ParticleSystemModule(true)
, m_Type(kCone)
{
}

void ShapeModule::Start(const ParticleSystemInitState& initState, const ParticleSystemState& state, ParticleSystemParticles& ps, const Matrix4x4f& matrix, size_t fromIndex, float t)
{
	// todo	
}

void ShapeModule::AcquireMeshData(const Matrix4x4f& worldToLocal)
{
	// todo
	if (!IsUsingMesh())
		return;
}

void ShapeModule::ResetSeed(const ParticleSystemInitState& initState)
{
	m_Random.SetSeed(initState.randomSeed);
}