#pragma once

#include "ParticleSystemModule.h"
#include "Math/Random/rand.h"
#include "ParticleSystemGradients.h"

struct ParticleSystemState;

class InitialModule : public ParticleSystemModule
{
public:
	InitialModule();

	void Start(const ParticleSystemInitState& initState, const ParticleSystemState& state, ParticleSystemParticles& ps, const Matrix4x4f& matrix, size_t fromIndex, float t);
	void Update(const ParticleSystemInitState& initState, const ParticleSystemState& state, ParticleSystemParticles& ps, const size_t fromIndex, const size_t toIndex, float dt) const;
	void GenerateProcedural(const ParticleSystemInitState& initState, const ParticleSystemState& state, ParticleSystemParticles& ps, const ParticleSystemEmitReplay& emit);
	void ResetSeed(const ParticleSystemInitState& initState);
	inline MinMaxCurve& GetSpeedCurve() { return m_Speed; }
	inline const MinMaxCurve& GetSpeedCurve() const { return m_Speed; }
	inline int GetMaxNumParticles() const { return m_MaxNumParticles; }
	inline float GetInheritVelocity() const { return m_InheritVelocity; }
	Vector3f GetGravity(const ParticleSystemInitState& initState, const ParticleSystemState& state) const;

private:
	Rand& GetRandom() { return m_Random; }

	Rand m_Random;
	int m_MaxNumParticles;
	MinMaxCurve m_Lifetime;
	MinMaxCurve m_Speed;
	MinMaxGradient m_Color;
	MinMaxCurve m_Size;
	MinMaxCurve m_Rotation;
	float m_InheritVelocity;
	float m_GravityModifier;
};