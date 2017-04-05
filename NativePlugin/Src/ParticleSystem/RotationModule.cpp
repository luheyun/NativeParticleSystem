#include "PluginPrefix.h"
#include "RotationModule.h"
#include "ParticleSystemCommon.h"
#include "ParticleSystemUtils.h"

struct DualMinMaxPolyCurves
{
	MinMaxOptimizedPolyCurves optRot;
	MinMaxPolyCurves rot;
};

template<ParticleSystemCurveEvalMode mode>
void UpdateTpl(const MinMaxCurve& curve, ParticleSystemParticles& ps, const size_t fromIndex, const size_t toIndex)
{
	if ( !ps.usesRotationalSpeed ) return;
	for (size_t q = fromIndex; q < toIndex; ++q)
	{
		const float time = NormalizedTime(ps, q);
		const float random = GenerateRandom(ps.randomSeed[q] + kParticleSystemRotationCurveId);
		ps.rotationalSpeed[q] += Evaluate<mode> (curve, time, random);
	}
}

template<bool isOptimized>
void UpdateProceduralTpl(const DualMinMaxPolyCurves& curves, ParticleSystemParticles& ps)
{
	const size_t count = ps.array_size ();
	for (size_t q=0; q<count; q++)
	{
		float time = NormalizedTime(ps, q);
		float random = GenerateRandom(ps.randomSeed[q] + kParticleSystemRotationCurveId);
		float range = ps.startLifetime[q];
		float value;
		if(isOptimized)
			value = EvaluateIntegrated (curves.optRot, time, random);
		else
			value = EvaluateIntegrated (curves.rot, time, random);
		ps.rotation[q] += value * range;
	}
}

RotationModule::RotationModule() : ParticleSystemModule(true)
{}

void RotationModule::Init(float minAngularVelocity, float maxAngularVelocity)
{
    MinMaxAnimationCurves animCurves;
    AnimationCurve::Keyframe minKeyFrame, maxKeyFrame;
    minKeyFrame.value = minAngularVelocity;
    animCurves.min.AddKey(minKeyFrame);
    maxKeyFrame.value = maxAngularVelocity;
    animCurves.max.AddKey(maxKeyFrame);
    m_Curve.minMaxState = kMMCTwoConstants;
    m_Curve.editorCurves = animCurves;
    m_Curve.SetScalar(Deg2Rad(45.0f));
}

void RotationModule::Update(const ParticleSystemInitState& initState, const ParticleSystemState& state, ParticleSystemParticles& ps, const size_t fromIndex, const size_t toIndex)
{
	if (m_Curve.minMaxState == kMMCScalar)
		UpdateTpl<kEMScalar>(m_Curve, ps, fromIndex, toIndex);
	else if(m_Curve.IsOptimized() && m_Curve.UsesMinMax())
		UpdateTpl<kEMOptimizedMinMax>(m_Curve, ps, fromIndex, toIndex);
	else if(m_Curve.IsOptimized())
		UpdateTpl<kEMOptimized>(m_Curve, ps, fromIndex, toIndex);
	else 
		UpdateTpl<kEMSlow>(m_Curve, ps, fromIndex, toIndex);

}

void RotationModule::UpdateProcedural (const ParticleSystemState& state, ParticleSystemParticles& ps)
{
	DualMinMaxPolyCurves curves;
	if(m_Curve.IsOptimized())
	{
		curves.optRot = m_Curve.polyCurves; curves.optRot.Integrate();
		UpdateProceduralTpl<true>(curves, ps);
	}
	else
	{
		BuildCurves(curves.rot, m_Curve.editorCurves, m_Curve.GetScalar(), m_Curve.minMaxState); curves.rot.Integrate();
		UpdateProceduralTpl<false>(curves, ps);
	}
}
