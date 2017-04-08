#include "PluginPrefix.h"
#include "ParticleSystemModule.h"

ParticleSystemInitState::ParticleSystemInitState()
	: looping(true)
	, prewarm(false)
	, playOnAwake(true)
	, randomSeed(0)
	, startDelay(0.0f)
	, speed(1.0f)
	, lengthInSec(5.0f)
	, useLocalSpace(true)
{
}

void ParticleSystemState::Tick(const ParticleSystemInitState& initState, float dt)
{
	t += dt;

	if (!initState.looping)
		t = std::min<float>(t, initState.lengthInSec);
	else
		if (t > initState.lengthInSec)
		{
			t -= initState.lengthInSec;
			numLoops++;
		}
}

void ParticleSystemModule::InitCurveFromMono(MinMaxCurve& curve, const MonoCurve* monoCurve)
{
    for (int i = 0; i < monoCurve->maxCurve->keyFrameCount; ++i)
    {
        AnimationCurve::Keyframe keyFrame;
        keyFrame.time = monoCurve->maxCurve->pKeyFrameContainer[i]->time;
        keyFrame.inSlope = monoCurve->maxCurve->pKeyFrameContainer[i]->inSlope;
        keyFrame.outSlope = monoCurve->maxCurve->pKeyFrameContainer[i]->outSlope;
        keyFrame.value = monoCurve->maxCurve->pKeyFrameContainer[i]->value;
        curve.editorCurves.max.AddKey(keyFrame);
    }

    curve.editorCurves.max.SetPreInfinity(monoCurve->maxCurve->preInfinity);
    curve.editorCurves.max.SetPostInfinity(monoCurve->maxCurve->postInfinity);

    for (int i = 0; i < monoCurve->minCurve->keyFrameCount; ++i)
    {
        AnimationCurve::Keyframe keyFrame;
        keyFrame.time = monoCurve->minCurve->pKeyFrameContainer[i]->time;
        keyFrame.inSlope = monoCurve->minCurve->pKeyFrameContainer[i]->inSlope;
        keyFrame.outSlope = monoCurve->minCurve->pKeyFrameContainer[i]->outSlope;
        keyFrame.value = monoCurve->minCurve->pKeyFrameContainer[i]->value;
        curve.editorCurves.min.AddKey(keyFrame);
    }

    curve.editorCurves.min.SetPreInfinity(monoCurve->minCurve->preInfinity);
    curve.editorCurves.min.SetPostInfinity(monoCurve->minCurve->postInfinity);
    curve.minMaxState = monoCurve->minMaxState;
    curve.SetScalar(monoCurve->scalar);
}