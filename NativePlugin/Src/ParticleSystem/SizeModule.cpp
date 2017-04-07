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

void SizeModule::Init(ParticleSystemInitState* initState)
{
    if (initState->sizeModuleEnable)
    {
        SetEnabled(true);
        MinMaxCurve& curve = GetCurve();

        for (int i = 0; i < initState->sizeModuleCurve->maxCurve->keyFrameCount; ++i)
        {
            AnimationCurve::Keyframe keyFrame;
            keyFrame.time = initState->sizeModuleCurve->maxCurve->pKeyFrameContainer[i]->time;
            keyFrame.inSlope = initState->sizeModuleCurve->maxCurve->pKeyFrameContainer[i]->inSlope;
            keyFrame.outSlope = initState->sizeModuleCurve->maxCurve->pKeyFrameContainer[i]->outSlope;
            keyFrame.value = initState->sizeModuleCurve->maxCurve->pKeyFrameContainer[i]->value;
            curve.editorCurves.max.AddKey(keyFrame);
        }

        curve.editorCurves.max.SetPreInfinity(initState->sizeModuleCurve->maxCurve->preInfinity);
        curve.editorCurves.max.SetPostInfinity(initState->sizeModuleCurve->maxCurve->postInfinity);

        for (int i = 0; i < initState->sizeModuleCurve->minCurve->keyFrameCount; ++i)
        {
            AnimationCurve::Keyframe keyFrame;
            keyFrame.time = initState->sizeModuleCurve->minCurve->pKeyFrameContainer[i]->time;
            keyFrame.inSlope = initState->sizeModuleCurve->minCurve->pKeyFrameContainer[i]->inSlope;
            keyFrame.outSlope = initState->sizeModuleCurve->minCurve->pKeyFrameContainer[i]->outSlope;
            keyFrame.value = initState->sizeModuleCurve->minCurve->pKeyFrameContainer[i]->value;
            curve.editorCurves.min.AddKey(keyFrame);
        }

        curve.editorCurves.min.SetPreInfinity(initState->sizeModuleCurve->minCurve->preInfinity);
        curve.editorCurves.min.SetPostInfinity(initState->sizeModuleCurve->minCurve->postInfinity);
        curve.minMaxState = initState->sizeModuleCurve->minMaxState;
        curve.SetScalar(initState->sizeModuleCurve->scalar);
    }
    else
    {
        SetEnabled(false);
    }
}

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
