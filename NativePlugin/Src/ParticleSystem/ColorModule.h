#ifndef SHURIKENMODULECOLOR_H
#define SHURIKENMODULECOLOR_H

#include "ParticleSystemModule.h"
#include "ParticleSystemGradients.h"

class ColorModule : public ParticleSystemModule
{
public:
	ColorModule ();

	void Update (const ParticleSystemParticles& ps, ColorRGBA32* colorTemp, size_t fromIndex, size_t toIndex);
	void CheckConsistency() {};

	inline MinMaxGradient& GetGradient() { return m_Gradient; };

	template<class TransferFunction>
	void Transfer (TransferFunction& transfer);
	
private:
	MinMaxGradient m_Gradient;
};

#endif // SHURIKENMODULECOLOR_H
