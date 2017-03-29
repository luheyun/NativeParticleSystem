#include "PluginPrefix.h"
#include "SizeModule.h"
#include "ParticleSystemUtils.h"

template<ParticleSystemCurveEvalMode mode>
void UpdateTpl(const MinMaxCurve& curve, const ParticleSystemParticles& ps, float* tempSize, size_t fromIndex, size_t toIndex)
{
	for (size_t q = fromIndex; q < toIndex; ++q)
	{
		const float time = NormalizedTime(ps, q);
		const float random = GenerateRandom(ps.randomSeed[q] + kParticleSystemSizeCurveId);
		tempSize[q] *= std::max<float>(0.0f, Evaluate<mode> (curve, time, random));
	}
}

SizeModule::SizeModule() : ParticleSystemModule(false)
{}

void SizeModule::Update (const ParticleSystemParticles& ps, float* tempSize, size_t fromIndex, size_t toIndex)
{
	if(m_Curve.minMaxState == kMMCScalar)
		UpdateTpl<kEMScalar>(m_Curve, ps, tempSize, fromIndex, toIndex);
	else if (m_Curve.IsOptimized() && m_Curve.UsesMinMax ())
		UpdateTpl<kEMOptimizedMinMax>(m_Curve, ps, tempSize, fromIndex, toIndex);
	else if(m_Curve.IsOptimized())
		UpdateTpl<kEMOptimized>(m_Curve, ps, tempSize, fromIndex, toIndex);
	else
		UpdateTpl<kEMSlow>(m_Curve, ps, tempSize, fromIndex, toIndex);
}
