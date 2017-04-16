#ifndef SHURIKENMODULESIZE_H
#define SHURIKENMODULESIZE_H

#include "ParticleSystemModule.h"
#include "ParticleSystemCurves.h"

class SizeModule : public ParticleSystemModule
{
public:
	SizeModule();
	
    void Init(ParticleSystemInitState* initState);
	void Update (const ParticleSystemParticles& ps, float* tempSize, size_t fromIndex, size_t toIndex);

	void CheckConsistency () {};

	inline MinMaxCurve& GetCurve() { return m_Curve; }
	
private:	
	MinMaxCurve m_Curve;
};

#endif // SHURIKENMODULESIZE_H
