#include "PluginPrefix.h"
#include "ShapeModule.h"
#include "ParticleSystem/ParticleSystemUtils.h"
#include "ParticleSystem/ParticleSystem.h"
#include "Math/Vector2.h"
//#include "Graphics/TriStripper.h"
//#include "Geometry/ComputionalGeometry.h"

enum MeshDistributionMode
{
    kDistributionVertex,
    kDistributionTriangle,
};


/// This gives a random barycentric coord (on the edge of triangle)
// @TODO: Stupid: Make this in a faster way
inline Vector3f RandomBarycentricCoordEdge(Rand& rand)
{
    float u = rand.GetFloat();
    float v = rand.GetFloat();
    if (u + v > 1.0F)
    {
        u = 1.0F - u;
        v = 1.0F - v;
    }
    float w = 1.0F - u - v;

    int edge = RangedRandom(rand, 0, 2);
    if (0 == edge)
    {
        v += 0.5f * u;
        w += 0.5f * u;
        u = 0.0f;
    }
    else if (1 == edge)
    {
        u += 0.5f * v;
        w += 0.5f * v;
        v = 0.0f;
    }
    else
    {
        u += 0.5f * w;
        v += 0.5f * w;
        w = 0.0f;
    }

    return Vector3f(u, v, w);
}


// TODO: It could make sense to initialize in separated loops. i.e. separate position and velcoity vectors
inline void EmitterStoreData(const Matrix4x4f& localToWorld, const Vector3f& scale, ParticleSystemParticles& ps, size_t q, Vector3f& pos, Vector3f& n, Rand& random, bool randomDirection)
{
    if (randomDirection)
        n = RandomUnitVector(random);

    n = NormalizeSafe(n);

    pos = Scale(pos, scale);

    Vector3f vel = Magnitude(ps.velocity[q]) * n;
    vel = localToWorld.MultiplyVector3(vel);

    // @TODO: Sooo... why multiply point and then undo the result of it? Why not just MultiplyVector?
    pos = localToWorld.MultiplyPoint3(pos) - localToWorld.GetPosition();
    ps.position[q] += pos;
    ps.velocity[q] = vel;

    if (ps.usesAxisOfRotation)
    {
        Vector3f tan = Cross(-n, Vector3f::zAxis);
        if (SqrMagnitude(tan) <= 0.01)
            tan = Cross(-pos, Vector3f::zAxis);
        if (SqrMagnitude(tan) <= 0.01)
            tan = Vector3f::yAxis;
        ps.axisOfRotation[q] = Normalize(tan);
    }
}

inline void EmitterStoreData(const Matrix4x4f& localToWorld, const Vector3f& scale, ParticleSystemParticles& ps, size_t q, Vector3f& pos, Vector3f& n, ColorRGBA32& color, Rand& random, bool randomDirection)
{
    EmitterStoreData(localToWorld, scale, ps, q, pos, n, random, randomDirection);
    ps.color[q] *= color;
}


template<MeshDistributionMode distributionMode>
void GetPositionMesh(Vector3f& pos,
    Vector3f& n,
    ColorRGBA32& color,
    const ParticleSystemEmitterMeshVertex* vertexData,
    const int vertexCount,
    const MeshTriangleData* triangleData,
    const UInt32 numPrimitives,
    float totalTriangleArea,
    Rand& random,
    bool edge)
{
    // position/normal of particle is vertex/vertex normal from mesh
    if (kDistributionVertex == distributionMode)
    {
        int vertexIndex = RangedRandom(random, 0, vertexCount);
        pos = vertexData[vertexIndex].position;
        n = vertexData[vertexIndex].normal;
        color = vertexData[vertexIndex].color;
    }
    else if (kDistributionTriangle == distributionMode)
    {
        float randomArea = RangedRandom(random, 0.0f, totalTriangleArea);
        float accArea = 0.0f;
        UInt32 triangleIndex = 0;

        for (UInt32 i = 0; i < numPrimitives; i++)
        {
            const MeshTriangleData& data = triangleData[i];
            accArea += data.area;
            if (accArea >= randomArea)
            {
                triangleIndex = i;
                break;
            }
        }

        const MeshTriangleData& data = triangleData[triangleIndex];
        UInt16 a = data.indices[0];
        UInt16 b = data.indices[1];
        UInt16 c = data.indices[2];

        Vector3f barycenter;
        if (edge)
            barycenter = RandomBarycentricCoordEdge(random);
        else
            barycenter = RandomBarycentricCoord(random);

        // Interpolate vertex with barycentric coordinate
        pos = barycenter.x * vertexData[a].position + barycenter.y * vertexData[b].position + barycenter.z * vertexData[c].position;
        n = barycenter.x * vertexData[a].normal + barycenter.y * vertexData[b].normal + barycenter.z * vertexData[c].normal;

        // TODO: Don't convert to floats!!!
        ColorRGBAf color1 = vertexData[a].color;
        ColorRGBAf color2 = vertexData[b].color;
        ColorRGBAf color3 = vertexData[c].color;
        color = barycenter.x * color1 + barycenter.y * color2 + barycenter.z * color3;
    }
}

static bool CompareMeshTriangleData(const MeshTriangleData& a, const MeshTriangleData& b)
{
    return (a.area > b.area);
}

// ------------------------------------------------------------------------------------------

// todo
Vector3f* emitterScale = new Vector3f(1.0f, 1.0f, 1.0f);

ShapeModule::ShapeModule() : ParticleSystemModule(false)
, m_Type(kCone)
, m_Radius(1.0f)
, m_Angle(25.0f)
, m_Length(5.0f)
, m_BoxX(1.0f)
, m_BoxY(1.0f)
, m_BoxZ(1.0f)
, m_RandomDirection(false)
{
}

void ShapeModule::Start(const ParticleSystemInitState& initState, const ParticleSystemState& state, ParticleSystemParticles& ps, const Matrix4x4f& matrix, size_t fromIndex, float t)
{
    const float normalizedT = t / initState.lengthInSec;

    Rand& random = GetRandom();

    const float r = m_Radius;

    float a = Deg2Rad(m_Angle);
    float sinA = Sin(a);
    float cosA = Cos(a);
    float length = m_Length;

    const size_t count = ps.array_size();
    switch (m_Type)
    {
    case kSphere:
    {
        for (size_t q = fromIndex; q < count; ++q)
        {
            Vector3f pos = RandomPointInsideUnitSphere(random) * r;
            Vector3f n = pos;
            EmitterStoreData(matrix, *emitterScale, ps, q, pos, n, random, m_RandomDirection);
        }
        break;
    }
    case kSphereShell:
    {
        for (size_t q = fromIndex; q < count; ++q)
        {
            Vector3f pos = RandomUnitVector(random) * r;
            Vector3f n = pos;
            EmitterStoreData(matrix, *emitterScale, ps, q, pos, n, random, m_RandomDirection);
        }
        break;
    }
    case kHemiSphere:
    {
        for (size_t q = fromIndex; q < count; ++q)
        {
            Vector3f pos = RandomPointInsideUnitSphere(random) * r;
            if (pos.z < 0.0f)
                pos.z *= -1.0f;
            Vector3f n = pos;
            EmitterStoreData(matrix, *emitterScale, ps, q, pos, n, random, m_RandomDirection);
        }
        break;
    }
    case kHemiSphereShell:
    {
        for (size_t q = fromIndex; q < count; ++q)
        {
            Vector3f pos = RandomUnitVector(random) * r;
            if (pos.z < 0.0f)
                pos.z *= -1.0f;
            Vector3f n = pos;
            EmitterStoreData(matrix, *emitterScale, ps, q, pos, n, random, m_RandomDirection);
        }
        break;
    }
    case kCone:
    {
        for (size_t q = fromIndex; q < count; ++q)
        {
            Vector2f posXY = RandomPointInsideUnitCircle(random);
            Vector2f nXY;
            if (m_RandomDirection)
                nXY = RandomPointInsideUnitCircle(random) * sinA;
            else
                nXY = Vector2f(posXY.x, posXY.y)* sinA;
            Vector3f n(nXY.x, nXY.y, cosA);
            Vector3f pos(posXY.x * r, posXY.y * r, 0.0f);
            EmitterStoreData(matrix, *emitterScale, ps, q, pos, n, random, false);
        }
        break;
    }
    case kConeShell:
    {
        for (size_t q = fromIndex; q < count; ++q)
        {
            Vector2f posXY = NormalizeSafe(RandomUnitVector2(random));

            Vector2f nXY;
            if (m_RandomDirection)
                nXY = RandomPointInsideUnitCircle(random) * sinA;
            else
                nXY = Vector2f(posXY.x, posXY.y)* sinA;
            Vector3f n(nXY.x, nXY.y, cosA);
            Vector3f pos(posXY.x * r, posXY.y * r, 0.0f);
            EmitterStoreData(matrix, *emitterScale, ps, q, pos, n, random, false);
        }
        break;
    }
    case kConeVolume:
    {
        for (size_t q = fromIndex; q < count; ++q)
        {
            Vector2f posXY = RandomPointInsideUnitCircle(random);
            Vector2f nXY = Vector2f(posXY.x, posXY.y)* sinA;
            Vector3f n(nXY.x, nXY.y, cosA);
            Vector3f pos(posXY.x * r, posXY.y * r, 0.0f);
            pos += length * Random01(random) * NormalizeSafe(n);
            EmitterStoreData(matrix, *emitterScale, ps, q, pos, n, random, m_RandomDirection);
        }
        break;
    }
    case kConeVolumeShell:
    {
        for (size_t q = fromIndex; q < count; ++q)
        {
            Vector2f posXY = NormalizeSafe(RandomUnitVector2(random));
            Vector2f nXY = Vector2f(posXY.x, posXY.y)* sinA;
            Vector3f n(nXY.x, nXY.y, cosA);
            Vector3f pos = Vector3f(posXY.x * r, posXY.y * r, 0.0f);
            pos += length * Random01(random) * NormalizeSafe(n);
            EmitterStoreData(matrix, *emitterScale, ps, q, pos, n, random, m_RandomDirection);
        }
        break;
    }
    case kBox:
    {
        const Vector3f extents(0.5f * m_BoxX, 0.5f * m_BoxY, 0.5f * m_BoxZ);
        for (size_t q = fromIndex; q < count; ++q)
        {
            Vector3f pos = RandomPointInsideCube(random, extents);
            Vector3f n = Vector3f::zAxis;
            EmitterStoreData(matrix, *emitterScale, ps, q, pos, n, random, m_RandomDirection);
        }
    }
    break;
    default:
    {
    }
    }
}

void ShapeModule::CheckConsistency()
{
    m_Type = clamp<int>(m_Type, kSphere, kMax - 1);
    m_Angle = clamp(m_Angle, 0.0f, 90.0f);
    m_Radius = max(0.01f, m_Radius);
    m_Length = max(0.0f, m_Length);
    m_BoxX = max(0.0f, m_BoxX);
    m_BoxY = max(0.0f, m_BoxY);
    m_BoxZ = max(0.0f, m_BoxZ);
}

void ShapeModule::Init(ParticleSystemInitState* initState)
{
    SetEnabled(initState->shapeModuleEnable);

    if (GetEnabled())
    {
        m_Type = initState->shapeModuleData->type;
        m_Radius = initState->shapeModuleData->radius;
        m_Angle = initState->shapeModuleData->angle;
        m_BoxX = initState->shapeModuleData->boxX;
        m_BoxY = initState->shapeModuleData->boxY;
        m_BoxZ = initState->shapeModuleData->boxZ;
        m_RandomDirection = initState->shapeModuleData->randomDirection;
        m_Length = initState->shapeModuleData->length;
        ResetSeed(*initState);
    }
}

void ShapeModule::ResetSeed(const ParticleSystemInitState& initState)
{
    if (initState.randomSeed == 0)
        m_Random.SetSeed(GetGlobalRandomSeed());
    else
        m_Random.SetSeed(initState.randomSeed);
}

Rand& ShapeModule::GetRandom()
{
    return m_Random;
}

