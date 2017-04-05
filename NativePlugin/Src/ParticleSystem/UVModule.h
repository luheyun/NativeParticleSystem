#pragma once

#include "ParticleSystemModule.h"
#include "PolynomialCurve.h"

struct ParticleSystemParticles;

class UVModule : public ParticleSystemModule
{
public:
	UVModule ();

	void Update (const ParticleSystemParticles& ps, float* tempSheetIndex, size_t fromIndex, size_t toIndex);
	void CheckConsistency ();

	inline MinMaxCurve& GetCurve() { return m_Curve; }

	void GetNumTiles(int& uvTilesX, int& uvTilesY) const;
	
private:
	enum { kWholeSheet, kSingleRow, kNumAnimationTypes };
	
	MinMaxCurve m_Curve;
	int m_TilesX, m_TilesY;
	int m_AnimationType;
	int m_RowIndex;
	float m_Cycles;
	bool m_RandomRow;
};
