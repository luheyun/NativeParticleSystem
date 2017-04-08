#pragma once

#include "Math/Matrix4x4.h"
#include "Utilities/dynamic_array.h"
#include "ParticleSystemCurves.h"
#include "Mono/ScriptingAPI.h"

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

class MonoKeyFrame
{
public:
	float time;
	float value;
	float inSlope;
	float outSlope;
};

class MonoAnimationCurve
{
public:
    ~MonoAnimationCurve()
    {
        for (int i = 0; i < keyFrameCount; ++i)
            delete pKeyFrameContainer[i];

        pKeyFrameContainer = nullptr;
    }

    void InitFromMono(MonoAnimationCurve* pMonoAnimationCurve)
    {
        pKeyFrameContainer = (MonoKeyFrame**)malloc(sizeof(MonoKeyFrame*) * keyFrameCount);
        MonoKeyFrame** container = GetScriptingArrayStart<MonoKeyFrame*>((ScriptingArrayPtr)pMonoAnimationCurve->pKeyFrameContainer);
        for (int i = 0; i < keyFrameCount; ++i)
        {
            MonoKeyFrame* srcKeyFrame = (MonoKeyFrame*)GetLogicObjectMemoryLayout((ScriptingObjectPtr)(container[i]));

            pKeyFrameContainer[i] = new MonoKeyFrame();
            memcpy(pKeyFrameContainer[i], srcKeyFrame, sizeof(MonoKeyFrame));
        }
    }

	int keyFrameCount;
	MonoKeyFrame** pKeyFrameContainer;
	int preInfinity;
	int postInfinity;
};

class MonoCurve
{
public:
	MonoCurve()
        : maxCurve(nullptr)
        , minCurve(nullptr)
	{
	}

    ~MonoCurve()
    {
        if (maxCurve != nullptr)
            delete maxCurve;

        if (minCurve != nullptr)
            delete minCurve;

        maxCurve = nullptr;
        minCurve = nullptr;
    }

	void InitFromMono(MonoCurve* pMonoCurve)
	{
		MonoAnimationCurve* srcMinCurve = (MonoAnimationCurve*)GetLogicObjectMemoryLayout((ScriptingObjectPtr)pMonoCurve->minCurve);
        minCurve = new MonoAnimationCurve();
		memcpy(minCurve, srcMinCurve, sizeof(MonoAnimationCurve));
        minCurve->InitFromMono(srcMinCurve);

		MonoAnimationCurve* srcMaxCurve = (MonoAnimationCurve*)GetLogicObjectMemoryLayout((ScriptingObjectPtr)pMonoCurve->maxCurve);
        maxCurve = new MonoAnimationCurve();
		memcpy(maxCurve, srcMaxCurve, sizeof(MonoAnimationCurve));
        maxCurve->InitFromMono(srcMaxCurve);
	}

	int minMaxState;
    float scalar;
	MonoAnimationCurve* minCurve;
	MonoAnimationCurve* maxCurve;
};

class ParticleSystemInitState
{
public:
	ParticleSystemInitState();

	void InitFromMono(ParticleSystemInitState* pInitState)
	{
		memcpy(this, pInitState, sizeof(ParticleSystemInitState));
		//MonoCurve* src = (MonoCurve*)GetLogicObjectMemoryLayout((ScriptingObjectPtr)this->sizeModuleCurve);
  //      this->sizeModuleCurve = new MonoCurve();
		//memcpy(this->sizeModuleCurve, src, sizeof(MonoCurve));
		//this->sizeModuleCurve->InitFromMono(src);
        InitCurveFromMono(this->initModuleLiftTime);
        InitCurveFromMono(this->initModuleSpeed);
        InitCurveFromMono(this->initModuleSize);
        InitCurveFromMono(this->initModuleRotation);
        InitCurveFromMono(this->sizeModuleCurve);
        InitCurveFromMono(this->rotationModuleCurve);
	}

    void InitCurveFromMono(MonoCurve* &pMonoCurve)
    {
        MonoCurve* src = (MonoCurve*)GetLogicObjectMemoryLayout((ScriptingObjectPtr)pMonoCurve);
        pMonoCurve = new MonoCurve();
        memcpy(pMonoCurve, src, sizeof(MonoCurve));
        pMonoCurve->InitFromMono(src);
    }

	bool looping;
	bool prewarm;
	int randomSeed;
	bool playOnAwake;
	float startDelay;
	float speed;
	float lengthInSec;
	bool useLocalSpace;
	int maxNumParticles;
    MonoCurve* initModuleLiftTime;
    MonoCurve* initModuleSpeed;
    MonoCurve* initModuleSize;
    MonoCurve* initModuleRotation;
	bool rotationModuleEnable;
    MonoCurve* rotationModuleCurve;
    float rotationMin;
    float rotationMax;
	float emissionRate;
	bool sizeModuleEnable;
	MonoCurve* sizeModuleCurve;
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

    static void InitCurveFromMono(MinMaxCurve& curve, const MonoCurve* monoCurve);

private:
	bool m_Enabled;
};