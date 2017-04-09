#pragma once

#include "ParticleSystemModule.h"
#include "ParticleSystemGradients.h"

class ColorModule : public ParticleSystemModule
{
public:
	ColorModule ();

    void Init(ParticleSystemInitState* initState);
	void Update (const ParticleSystemParticles& ps, ColorRGBA32* colorTemp, size_t fromIndex, size_t toIndex);
	void CheckConsistency() {};

	inline MinMaxGradient& GetGradient() { return m_Gradient; };
	
private:
	MinMaxGradient m_Gradient;
};
