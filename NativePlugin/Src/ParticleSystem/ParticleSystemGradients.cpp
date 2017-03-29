#include "PluginPrefix.h"
#include "ParticleSystemGradients.h"

MinMaxGradient::MinMaxGradient()
:	minColor (255,255,255,255), maxColor (255,255,255,255), minMaxState (kMMGColor)
{
}

void MinMaxGradient::InitializeOptimized(OptimizedMinMaxGradient& g)
{
	maxGradient.InitializeOptimized(g.max);
	if (minMaxState == kMMGRandomBetweenTwoGradients)
		minGradient.InitializeOptimized(g.min);
}
