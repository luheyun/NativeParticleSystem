#pragma once

#include "Math/Matrix4x4.h"
#include "Utilities/dynamic_array.h"
#include "ParticleSystemCurves.h"

struct ParticleSystemEmissionState
{
	ParticleSystemEmissionState() { Clear(); }
	inline void Clear()
	{
		m_ToEmitAccumulator = 0.0f;
		m_ParticleSpacing = 0.0f;
	}
	float m_ParticleSpacing;
	float m_ToEmitAccumulator;
};

struct ParticleSystemEmissionData
{
	enum { kMaxNumBursts = 4 };

	int type;
	MinMaxCurve rate;
	float burstTime[kMaxNumBursts];
	UInt16 burstParticleCount[kMaxNumBursts];
	UInt8 burstCount;
};

class ParticleSystemInitState
{
public:
	ParticleSystemInitState();

	bool looping;
	bool prewarm;
	int randomSeed;
	bool playOnAwake;
	float startDelay;
	float speed;
	float lengthInSec;
	bool useLocalSpace;
	int maxNumParticles;
    float rotationMin;
    float rotationMax;
};

// @TODO: Find "pretty" place for shared structs and enums?
struct ParticleSystemEmitReplay
{
	float  t;
	float  aliveTime;
	float emissionOffset;
	float emissionGap;
	int    particlesToEmit;
	size_t numContinuous;
	UInt32 randomSeed;

	ParticleSystemEmitReplay(float inT, int inParticlesToEmit, float inEmissionOffset, float inEmissionGap, size_t inNumContinuous, UInt32 inRandomSeed)
		: t(inT), aliveTime(0.0F), emissionOffset(inEmissionOffset), emissionGap(inEmissionGap), particlesToEmit(inParticlesToEmit), numContinuous(inNumContinuous), randomSeed(inRandomSeed)
	{}
};

struct  ParticleSystemState
{
	ParticleSystemState()
		: isSubEmitter(false)
		, culled(false)
	{

	}

	// state
	float accumulatedDt;
	bool playing;
	bool needRestart;
	bool stopEmitting;
	float delayT;

	bool GetIsSubEmitter() const { return isSubEmitter; }
	void Tick(const ParticleSystemInitState& initState, float dt);
private:
	// When setting this we need to ensure some other things happen as well
	bool isSubEmitter;

public:
	bool firstUpdate;			// Is update being called for the first time on this system?
	double cullTime;			// Absolute time, so we need as double in case it runs for ages
	double stopTime;			// Time particle system was last stopped
	float t;
	int numLoops;				// Number of loops executed
	bool invalidateProcedural;  // This is set if anything changes from script at some point when running a system
	bool culled;				// Is particle system currently culled?

	// per-frame
	Matrix4x4f localToWorld;
	Matrix4x4f WorldToLocal;
	Vector3f emitterVelocity;
	Vector3f oldPosition;

	dynamic_array<ParticleSystemEmitReplay> emitReplay;
	ParticleSystemEmissionState emissionState;
};

class ParticleSystemModule
{
public:
	ParticleSystemModule(bool enabled) : m_Enabled(enabled) {}
	virtual ~ParticleSystemModule() {}

	inline bool GetEnabled() const { return m_Enabled; }
	inline void SetEnabled(bool enabled) { m_Enabled = enabled; }

private:
	bool m_Enabled;
};