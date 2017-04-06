#pragma once

#include "ParticleSystemModule.h"

class EmissionModule : public ParticleSystemModule
{
public:
	EmissionModule();

	enum { kEmissionTypeTime, kEmissionTypeDistance };
	void Init(float rate);
	static void Emit(ParticleSystemEmissionState& emissionState, size_t& amountOfParticlesToEmit, size_t& numContinuous, const ParticleSystemEmissionData& emissionData, const Vector3f velocity, float fromT, float toT, float dt, float length);
	const ParticleSystemEmissionData& GetEmissionDataRef() { return m_EmissionData; }
	const ParticleSystemEmissionData& GetEmissionDataRef() const { return m_EmissionData; }

private:
	ParticleSystemEmissionData m_EmissionData;
};