#pragma once

#include "ParticleSystemModule.h"
#include "Math/Random/rand.h"

struct MeshTriangleData
{
	float area;
	UInt16 indices[3];
};

struct ParticleSystemEmitterMeshVertex
{
	Vector3f position;
	Vector3f normal;
	ColorRGBA32 color;
};

class ShapeModule : public ParticleSystemModule
{
public:
	ShapeModule();

	enum { kSphere, kSphereShell, kHemiSphere, kHemiSphereShell, kCone, kBox, kMesh, kConeShell, kConeVolume, kConeVolumeShell, kCircle, kCircleEdge, kSingleSidedEdge, kMeshRenderer, kSkinnedMeshRenderer, kMax };

	void Start(const ParticleSystemInitState& initState, const ParticleSystemState& state, ParticleSystemParticles& ps, const Matrix4x4f& matrix, size_t fromIndex, float t);
	bool IsUsingMesh() const { return m_Type == kMesh || m_Type == kMeshRenderer || m_Type == kSkinnedMeshRenderer; }
	void AcquireMeshData(const Matrix4x4f& worldToLocal);
	void ResetSeed(const ParticleSystemInitState& initState);

private:
	Rand& GetRandom() { return m_Random; }

	int m_Type;
	Rand m_Random;
	dynamic_array<ParticleSystemEmitterMeshVertex> m_CachedVertexData;
	dynamic_array<MeshTriangleData> m_CachedTriangleData;
};