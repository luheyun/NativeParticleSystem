#include "PluginPrefix.h"
#include "UVModule.h"
#include "ParticleSystemUtils.h"

template<ParticleSystemCurveEvalMode mode>
void UpdateWholeSheetTpl(float cycles, const MinMaxCurve& curve, const ParticleSystemParticles& ps, float* tempSheetIndex, size_t fromIndex, size_t toIndex)
{
	for (size_t q = fromIndex; q < toIndex; ++q)
		tempSheetIndex[q] = Repeat (cycles * Evaluate(curve, NormalizedTime(ps, q), GenerateRandom(ps.randomSeed[q] + kParticleSystemUVCurveId)), 1.0f);
}

UVModule::UVModule () : ParticleSystemModule(false)
,	m_TilesX (1), m_TilesY (1)
,	m_AnimationType (kWholeSheet)
,	m_RowIndex (0)
,	m_Cycles (1.0f)
,	m_RandomRow (true)
{}

void UVModule::Update (const ParticleSystemParticles& ps, float* tempSheetIndex, size_t fromIndex, size_t toIndex)
{
	const float cycles = m_Cycles;

	if (m_AnimationType == kSingleRow) // row
	{
		int rows = m_TilesY;
		float animRange = (1.0f / (m_TilesX * rows)) * m_TilesX;
		if(m_RandomRow)
		{
			for (size_t q = fromIndex; q < toIndex; ++q)
			{
				const float t = cycles * Evaluate(m_Curve, NormalizedTime(ps, q), GenerateRandom(ps.randomSeed[q] + kParticleSystemUVCurveId));
				const float x = Repeat (t, 1.0f);
				const float randomValue = GenerateRandom(ps.randomSeed[q] + kParticleSystemUVRowSelectionId);
				const float startRow = Floorf (randomValue * rows);
				float from = startRow * animRange;
				float to = from + animRange;
				tempSheetIndex[q] = Lerp (from, to, x);
			}
		}
		else
		{
			const float startRow = Floorf(m_RowIndex * animRange * rows);
			float from = startRow * animRange;
			float to = from + animRange;
			for (size_t q = fromIndex; q < toIndex; ++q)
			{
				const float t = cycles * Evaluate(m_Curve, NormalizedTime(ps, q), GenerateRandom(ps.randomSeed[q] + kParticleSystemUVCurveId));
				const float x = Repeat (t, 1.0f);
				tempSheetIndex[q] = Lerp (from, to, x);
			}
		}
	}
	else if (m_AnimationType == kWholeSheet) // grid || row
	{
		if(m_Curve.minMaxState == kMMCScalar)
			UpdateWholeSheetTpl<kEMScalar>(m_Cycles, m_Curve, ps, tempSheetIndex, fromIndex, toIndex);
		else if (m_Curve.IsOptimized() && m_Curve.UsesMinMax ())
			UpdateWholeSheetTpl<kEMOptimizedMinMax>(m_Cycles, m_Curve, ps, tempSheetIndex, fromIndex, toIndex);
		else if(m_Curve.IsOptimized())
			UpdateWholeSheetTpl<kEMOptimized>(m_Cycles, m_Curve, ps, tempSheetIndex, fromIndex, toIndex);
		else
			UpdateWholeSheetTpl<kEMSlow>(m_Cycles, m_Curve, ps, tempSheetIndex, fromIndex, toIndex);
	}
}

void UVModule::CheckConsistency ()
{
	m_AnimationType = clamp<int> (m_AnimationType, 0, kNumAnimationTypes-1);
	m_TilesX = std::max<int> (1, m_TilesX);
	m_TilesY = std::max<int> (1, m_TilesY);
	m_Cycles = std::max<int> (1, (int)m_Cycles);
	m_RowIndex = clamp<int> (m_RowIndex, 0, m_TilesY-1);
	m_Curve.SetScalar(clamp<float> (m_Curve.GetScalar(), 0.0f, 1.0f));
}

void UVModule::GetNumTiles(int& uvTilesX, int& uvTilesY) const
{
	uvTilesX = m_TilesX;
	uvTilesY = m_TilesY;
}
