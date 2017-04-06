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
	sizeModuleCurve = new MonoCurve();
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