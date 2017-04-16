#include "PluginPrefix.h"
#include "ColorModule.h"
#include "ParticleSystemUtils.h"

template<MinMaxGradientEvalMode mode>
void UpdateTpl(const ParticleSystemParticles& ps, ColorRGBA32* colorTemp, const MinMaxGradient& gradient, const OptimizedMinMaxGradient& optGradient, size_t fromIndex, size_t toIndex)
{
	for (size_t q = fromIndex; q < toIndex; ++q)
	{
		const float time = NormalizedTime(ps, q);
		const int random = GenerateRandomByte(ps.randomSeed[q] + kParticleSystemColorGradientId);

		ColorRGBA32 value;
		if(mode == kGEMGradient)
			value = EvaluateGradient (optGradient, time);
		else if(mode == kGEMGradientMinMax)
			value = EvaluateRandomGradient (optGradient, time, random);
		else
			value = Evaluate (gradient, time, random);

		colorTemp[q] *= value;
	}
}

ColorModule::ColorModule () : ParticleSystemModule(false)
{}

void ColorModule::Init(ParticleSystemInitState* initState)
{
	SetEnabled(initState->colorModuleEnable);

	if (GetEnabled())
	{
		m_Gradient.maxColor = initState->colorModuleMinMaxGradient->maxColor;
		m_Gradient.minColor = initState->colorModuleMinMaxGradient->minColor;
		m_Gradient.minMaxState = initState->colorModuleMinMaxGradient->minMaxState;
		MonoGradient* maxGradient = initState->colorModuleMinMaxGradient->maxGradient;
		m_Gradient.maxGradient.SetKeys(maxGradient->colorKeys, maxGradient->colorKeyCount, maxGradient->alphaKeys, maxGradient->alphaKeyCount);
		MonoGradient* minGradient = initState->colorModuleMinMaxGradient->minGradient;
		m_Gradient.minGradient.SetKeys(minGradient->colorKeys, minGradient->colorKeyCount, minGradient->alphaKeys, minGradient->alphaKeyCount);
	}
}

void ColorModule::Update (const ParticleSystemParticles& ps, ColorRGBA32* colorTemp, size_t fromIndex, size_t toIndex)
{
	OptimizedMinMaxGradient gradient;
	m_Gradient.InitializeOptimized(gradient);
	if(m_Gradient.minMaxState == kMMGGradient)
		UpdateTpl<kGEMGradient>(ps, colorTemp, m_Gradient, gradient, fromIndex, toIndex);
	else if(m_Gradient.minMaxState == kMMGRandomBetweenTwoGradients)
		UpdateTpl<kGEMGradientMinMax>(ps, colorTemp, m_Gradient, gradient, fromIndex, toIndex);
	else
		UpdateTpl<kGEMSlow>(ps, colorTemp, m_Gradient, gradient, fromIndex, toIndex);
}
