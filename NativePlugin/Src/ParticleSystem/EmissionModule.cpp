#include "PluginPrefix.h"
#include "EmissionModule.h"
#include "ParticleSystemCurves.h"

static int AccumulateBursts(const ParticleSystemEmissionData& emissionData, float t0, float t1)
{
	int burstParticles = 0;
	const size_t count = emissionData.burstCount;
	for (size_t q = 0; q < count; ++q)
	{
		if (emissionData.burstTime[q] >= t0 && emissionData.burstTime[q] < t1)
			burstParticles += emissionData.burstParticleCount[q];
	}
	return burstParticles;
}

static float AccumulateContinuous(const ParticleSystemEmissionData& emissionData, const float length, const float toT, const float dt)
{
	float normalizedT = toT / length;
	return std::max<float>(0.0f, Evaluate(emissionData.rate, normalizedT)) * dt;
};


EmissionModule::EmissionModule() : ParticleSystemModule(true)
{
	m_EmissionData.burstCount = 0;
	m_EmissionData.type = kEmissionTypeTime;
	for (int i = 0; i < ParticleSystemEmissionData::kMaxNumBursts; i++)
	{
		m_EmissionData.burstParticleCount[i] = 30;
		m_EmissionData.burstTime[i] = 0.0f;
	}
}

void EmissionModule::Init(float rate)
{
	m_EmissionData.rate.minMaxState = kMMCScalar;
	m_EmissionData.rate.SetScalar(rate);
}

void EmissionModule::Emit(ParticleSystemEmissionState& emissionState, size_t& amountOfParticlesToEmit, size_t& numContinuous, const ParticleSystemEmissionData& emissionData, const Vector3f velocity, float fromT, float toT, float dt, float length)
{
	const float epsilon = 0.0001f;
	if (kEmissionTypeTime == emissionData.type)
	{
		float rate = 0.0f;
		float t0 = std::max<float>(0.0f, fromT);
		float t1 = std::max<float>(0.0f, toT);
		if (t1 < t0) // handle loop
		{
			rate += AccumulateContinuous(emissionData, length, t1, t1); // from start to current time
			t1 = length; // from last time to end
		}
		rate += AccumulateContinuous(emissionData, length, t1, t1 - t0); // from start to current time

		const float newParticles = rate;
		if (newParticles >= epsilon)
			emissionState.m_ParticleSpacing = 1.0f / newParticles;
		else
			emissionState.m_ParticleSpacing = 1.0f;

		emissionState.m_ToEmitAccumulator += newParticles;
		amountOfParticlesToEmit = (int)emissionState.m_ToEmitAccumulator;
		emissionState.m_ToEmitAccumulator -= (float)amountOfParticlesToEmit;

		// Continuous emits
		numContinuous = amountOfParticlesToEmit;

		// Bursts
		t0 = std::max<float>(0.0f, fromT);
		t1 = std::max<float>(0.0f, toT);
		if (t1 < t0) // handle loop
		{
			amountOfParticlesToEmit += AccumulateBursts(emissionData, 0.0f, t1); // from start to current time
			t1 = length + epsilon; // from last time to end
		}
		amountOfParticlesToEmit += AccumulateBursts(emissionData, t0, t1); // from start to current time
	}
	else
	{
		float newParticles = AccumulateContinuous(emissionData, length, toT, dt) * Magnitude(velocity); // from start to current time
		if (newParticles >= epsilon)
			emissionState.m_ParticleSpacing = 1.0f / newParticles;
		else
			emissionState.m_ParticleSpacing = 1.0f;

		emissionState.m_ToEmitAccumulator += newParticles;
		amountOfParticlesToEmit = (int)emissionState.m_ToEmitAccumulator;
		emissionState.m_ToEmitAccumulator -= (float)amountOfParticlesToEmit;

		// Continuous emits
		numContinuous = amountOfParticlesToEmit;
	}
}