#pragma once

#include "ParticleSystemModule.h"
#include "Math/Random/rand.h"
#include "Utilities/LinkedList.h"

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

class Mesh;

class ShapeModule : public ParticleSystemModule
{
public:
    ShapeModule();

    enum { kSphere, kSphereShell, kHemiSphere, kHemiSphereShell, kCone, kBox, kMesh, kConeShell, kConeVolume, kConeVolumeShell, kMax };

    void Init(ParticleSystemInitState* initState);
    void ResetSeed(const ParticleSystemInitState& initState);
    //void CalculateProceduralBounds(MinMaxAABB& bounds, const Vector3f& emitterScale, Vector2f minMaxBounds) const; todo

    void Start(const ParticleSystemInitState& initState, const ParticleSystemState& state, ParticleSystemParticles& ps, const Matrix4x4f& matrix, size_t fromIndex, float t);
    void CheckConsistency();

    inline void SetShapeType(int type) { m_Type = type; };
    inline void SetRadius(float radius) { m_Radius = radius; };

private:
    Rand& GetRandom();

    int m_Type;

    // Primitive stuff
    float m_Radius;
    float m_Angle;
    float m_Length;
    float m_BoxX;
    float m_BoxY;
    float m_BoxZ;

    bool m_RandomDirection;
    Rand m_Random;
};
