#pragma once

#include "ParticleSystemModule.h"

class ParticleSystemRenderer;
struct ParticleSystemParticles;
class Transform;
class ParticleSystemInitState;
struct ParticleSystemState;
class RotationModule;
class ColorModule;
class SizeModule;
class UVModule;
class ShapeModule;
class InitialModule;
class EmissionModule;
struct Job;

struct ParticleSystemThreadScratchPad
{
	ParticleSystemThreadScratchPad()
		: deltaTime(1.0f)
	{}

	float deltaTime;
};

class ParticleSystem
{
public:
	enum ClearFlags
	{
		kUpdateBounds = (1 << 0),
		kStop = (1 << 1)
	};

	// allocate N particles at a time
	static const size_t kMemAllocGranularity = 64;

	static int CreateParticleSystrem(ParticleSystemInitState* initState);
	static void Init();
	static void ShutDown();
	static void BeginUpdateAll();
	static void EndUpdateAll();
	static void Prepare();
	static void Render();
	static void Update(ParticleSystem& system, float deltaTime, bool fixedTimeStep, bool useProcedural, int rayBudget = 0);
	static void SyncJobs(bool syncRenderJobs = true);
	static void UpdateFunction(ParticleSystem* system);
	static bool CompareJobs(const Job& left, const Job& right);

	ParticleSystem(ParticleSystemInitState* initState);
	~ParticleSystem();

	void Play(bool autoPrewarm = true);
	void Stop();
	void Pause();
	void Clear(UInt32 flags);
	ParticleSystemParticles& GetParticles();
	bool IsActive() { return m_IsActive; }
	void PrepareForRender();
	void AutoPrewarm();
	void SetUsesAxisOfRotation();
	void Simulate(float deltaTime, bool restart, bool fixedTimeStep);	// Fastforwards the particle system by simulating particles over given period of time, then pauses it.

	void AddToManager();
    void RemoveFromManager();

    int GetParticleCount() const;
	static size_t EmitFromData(ParticleSystemEmissionState& emissionState, size_t& numContinuous, const ParticleSystemEmissionData& emissionData, const Vector3f velocity, float fromT, float toT, float dt, float length);

    void SetWorldMatrix(const Matrix4x4f& worldMatrix) { m_WorldMatrix = worldMatrix; }
	ParticleSystemThreadScratchPad& GetThreadScratchPad() { return m_ThreadScratchpad; }

private:
	void ResetSeeds();
	static size_t EmitFromModules(const ParticleSystem& system, const ParticleSystemInitState& initState, ParticleSystemEmissionState& emissionState, size_t& numContinuous, const Vector3f velocity, float fromT, float toT, float dt);
	static void Update0(ParticleSystem& system, const ParticleSystemInitState& initState, ParticleSystemState& state, float dt, bool fixedTimeStep);
	static void Update1(ParticleSystem& system, ParticleSystemParticles& ps, float dt, bool fixedTimeStep, bool useProcedural, int rayBudget = 0);
	static void Update2(ParticleSystem& system, const ParticleSystemInitState& initState, ParticleSystemState& state, bool fixedTimeStep);
	static void Update1Incremental(ParticleSystem& system, const ParticleSystemInitState& initState, ParticleSystemState& state, ParticleSystemParticles& ps, size_t fromIndex, float dt, bool useProcedural);
	static void UpdateProcedural(ParticleSystem& system, const ParticleSystemInitState& initState, ParticleSystemState& state, ParticleSystemParticles& ps);
	static void UpdateModulesPreSimulationIncremental(const ParticleSystem& system, const ParticleSystemInitState& initState, const ParticleSystemState& state, ParticleSystemParticles& ps, const size_t fromIndex, const size_t toIndex, float dt);
	static void UpdateModulesIncremental(const ParticleSystem& system, const ParticleSystemInitState& initState, ParticleSystemState& state, ParticleSystemParticles& ps, size_t fromIndex, float dt);
	static void UpdateModulesNonIncremental(const ParticleSystem& system, const ParticleSystemParticles& ps, ParticleSystemParticlesTempData& psTemp, size_t fromIndex, size_t toIndex);
	static void SimulateParticles(const ParticleSystemInitState& initState, ParticleSystemState& state, ParticleSystemParticles& ps, const size_t fromIndex, float dt);
	static void StartModules(ParticleSystem& system, const ParticleSystemInitState& initState, ParticleSystemState& state, const ParticleSystemEmissionState& emissionState, Vector3f initialVelocity, const Matrix4x4f& matrix, ParticleSystemParticles& ps, size_t fromIndex, float dt, float t, size_t numContinuous, float frameOffset);
	static void StartParticles(ParticleSystem& system, ParticleSystemParticles& ps, const float prevT, const float t, const float dt, const size_t numContinuous, size_t amountOfParticlesToEmit, float frameOffset);
	static void StartParticlesProcedural(ParticleSystem& system, ParticleSystemParticles& ps, const float prevT, const float t, const float dt, const size_t numContinuous, size_t amountOfParticlesToEmit, float frameOffset);
	static bool CheckSupportsProcedural(const ParticleSystem& system);

	void Cull();
	size_t AddNewParticles(ParticleSystemParticles& particles, size_t newParticles) const;
	size_t LimitParticleCount(size_t requestSize) const;
	bool ComputePrewarmStartParameters(float& prewarmTime, float t);

    void SetUsesRotationalSpeed();

private:
    int m_EmittersIndex;
	ParticleSystemRenderer* m_Renderer;
	ParticleSystemParticles* m_Particles;
	ParticleSystemInitState* m_InitState;
	ParticleSystemState* m_State;
	Transform* m_Transform;
	InitialModule* m_InitialModule;
	ShapeModule* m_ShapeModule;
	EmissionModule* m_EmissionModule;
	RotationModule*	m_RotationModule; // @TODO: Requires outputs angular velocity and thus requires integration (Inconsistent with other modules in this group)
	ColorModule* m_ColorModule;
	SizeModule*	m_SizeModule;
	UVModule* m_UVModule;
	bool m_IsActive = true;
    Matrix4x4f m_WorldMatrix;
	ParticleSystemThreadScratchPad	m_ThreadScratchpad;

	friend class ParticleSystemRenderer;
};

extern Matrix4x4f gWorldMatrix;