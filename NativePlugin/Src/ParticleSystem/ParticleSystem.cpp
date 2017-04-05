#include "PluginPrefix.h"
#include "ParticleSystem.h"
#include "ParticleSystemRenderer.h"
#include "ParticleSystemModule.h"
#include "RotationModule.h"
#include "ColorModule.h"
#include "SizeModule.h"
#include "UVModule.h"
#include "ParticleSystemParticles.h"
#include "ParticleSystemCommon.h"
#include "ParticleSystemUtils.h"
#include "Graphics/Transform.h"
#include "Input/TimeManager.h"
#include "Utilities/Utility.h"
#include "Math/Random/Random.h"
#include "Mono/ScriptingTypes.h"
#include "Mono/ScriptingAPI.h"
#include "Log/Log.h"
#include "Shaders/GraphicsCaps.h"

struct ParticleSystemManager
{
	ParticleSystemManager()
		: needSync(false)
	{
		activeEmitters.reserve(8);
	}

	std::vector<ParticleSystem*> activeEmitters;
	std::vector<ParticleSystem*> particleSystems;

	bool needSync;
};

ParticleSystemManager* gParticleSystemManager = nullptr;

class ParticleSystemUpdateData
{
public:
	Matrix4x4f worldMatrix;
};

void Internal_CreateParticleSystem(ScriptingObject* initState)
{
	DebugLog("Internal_CreateParticleSystem");
	ParticleSystemInitState* pInitState = (ParticleSystemInitState*)GetLogicObjectMemoryLayout(initState);
	ParticleSystem::CreateParticleSystrem(pInitState);
}


Matrix4x4f gWorldMatrix; // todo

void Internal_ParticleSystem_Update(ScriptingObject* updateData)
{
	ParticleSystemUpdateData* gUpdateData = (ParticleSystemUpdateData*)GetLogicObjectMemoryLayout(updateData);
	gWorldMatrix = gUpdateData->worldMatrix;
}

static const char* s_ParticleSystem_IcallNames[] =
{
	"NativeParticleSystem::Internal_CreateParticleSystem",
	"NativeParticleSystem::Internal_ParticleSystem_Update",
	NULL
};

static const void* s_ParticleSystem_IcallFuncs[] =
{
	(const void*)&Internal_CreateParticleSystem,
	(const void*)&Internal_ParticleSystem_Update,
	NULL
};

static void RegisterParticleSystemBindings()
{
	for (int i = 0; s_ParticleSystem_IcallNames[i] != NULL; ++i)
	{
		script_add_internal_call(s_ParticleSystem_IcallNames[i], s_ParticleSystem_IcallFuncs[i]);
	}
}

#define MAX_TIME_STEP (0.03f)
static float GetTimeStep(float dt)
{
	if (dt > MAX_TIME_STEP)
		return dt / Ceilf(dt / MAX_TIME_STEP);
	else
		return dt;
}

static void ApplyStartDelay(float& delayT, float& accumulatedDt)
{
	if (delayT > 0.0f)
	{
		delayT -= accumulatedDt;
		accumulatedDt = std::max(-delayT, 0.0f);
		delayT = std::max(delayT, 0.0f);
	}
}

void ParticleSystem::CreateParticleSystrem(ParticleSystemInitState* initState)
{
	ParticleSystem* ps = new ParticleSystem(initState);
	gParticleSystemManager->particleSystems.push_back(ps);
}

void ParticleSystem::Init()
{
	gParticleSystemManager = new ParticleSystemManager();
	RegisterParticleSystemBindings();
}

void ParticleSystem::ShutDown()
{
	for (int i = 0; i < gParticleSystemManager->particleSystems.size(); ++i)
	{
		if (gParticleSystemManager->particleSystems[i] != nullptr)
			delete gParticleSystemManager->particleSystems[i];

		gParticleSystemManager->particleSystems[i] = nullptr;
	}

	gParticleSystemManager->particleSystems.clear();
	gParticleSystemManager->activeEmitters.clear();
}

void ParticleSystem::BeginUpdateAll()
{
	float deltaTime = GetDeltaTime();
	if (deltaTime == 0.0f)
		return;

	for (size_t i = 0; i < gParticleSystemManager->activeEmitters.size(); ++i)
	{
		ParticleSystem& system = *gParticleSystemManager->activeEmitters[i];

		if (!system.IsActive())
		{
			system.RemoveFromManager();
			continue;
		}

		Update0(system, *system.m_InitState, *system.m_State, deltaTime, false);
	}

	gParticleSystemManager->needSync = true;

#if ENABLE_MULTITHREADED_PARTICLES
	// todo
#else
	for (size_t i = 0; i < gParticleSystemManager->activeEmitters.size(); ++i)
	{
		ParticleSystem& system = *gParticleSystemManager->activeEmitters[i];
		system.Update1(system, system.GetParticles(), deltaTime, false, false);
	}
#endif

}

void ParticleSystem::EndUpdateAll()
{
	SyncJobs();

	// Remove emitters that are finished (no longer emitting)
	for (size_t i = 0; i < gParticleSystemManager->activeEmitters.size();)
	{
		ParticleSystem& system = *gParticleSystemManager->activeEmitters[i];
		ParticleSystemState& state = *system.m_State;
		const size_t particleCount = system.GetParticleCount();
		if ((particleCount == 0) && state.playing && state.stopEmitting)
		{
			// collision subemitters may not have needRestart==true when being restarted
			// from a paused state
			//Assert (state.needRestart);
			state.playing = false;
			system.RemoveFromManager();
			continue;
		}

		i++;
	}
}

ParticleSystem::ParticleSystem(ParticleSystemInitState* initState)
	: m_Renderer(nullptr)
	, m_EmittersIndex(-1)
	, m_InitState(initState)
{
	m_InitState = new ParticleSystemInitState();
	memcpy(m_InitState, initState, sizeof(ParticleSystemInitState));
	gParticleSystemManager->activeEmitters.push_back(this);
	m_Renderer = new ParticleSystemRenderer();
	m_State = new ParticleSystemState();
	m_ShapeModule = ShapeModule();
	m_SizeModule = new SizeModule();
	m_RotationModule = new RotationModule();
	m_ColorModule = new ColorModule();
	m_UVModule = new UVModule();

	m_InitialModule.SetMaxNumParticles(m_InitState->maxNumParticles);
	m_Particles = new ParticleSystemParticles();

	if (m_InitState->playOnAwake)
		Play(false);
}

ParticleSystem::~ParticleSystem()
{
	auto it = std::find(gParticleSystemManager->activeEmitters.begin(), gParticleSystemManager->activeEmitters.end(), this);
	gParticleSystemManager->activeEmitters.erase(it);

	if (m_Renderer != nullptr)
		delete m_Renderer;

	if (m_InitState != nullptr)
		delete m_InitState;

	if (m_State != nullptr)
		delete m_State;

	m_Renderer = nullptr;
	m_InitState = nullptr;
	m_State = nullptr;
}

void ParticleSystem::Prepare()
{
	auto it = gParticleSystemManager->activeEmitters.begin();

	for (; it != gParticleSystemManager->activeEmitters.end(); ++it)
		(*it)->PrepareForRender();
}

void ParticleSystem::Render()
{
	auto it = gParticleSystemManager->activeEmitters.begin();

	for (; it != gParticleSystemManager->activeEmitters.end(); ++it)
		(*it)->m_Renderer->RenderMultiple(**it);
}

void ParticleSystem::Update(ParticleSystem& system, float deltaTime, bool fixedTimeStep, bool useProcedural, int rayBudget)
{
	Update0(system, *system.m_InitState, *system.m_State, deltaTime, fixedTimeStep);
	Update1(system, system.GetParticles(), deltaTime, fixedTimeStep, useProcedural, rayBudget);
	Update2(system, *system.m_InitState, *system.m_State, fixedTimeStep);
}

void ParticleSystem::SyncJobs(bool syncRenderJobs)
{
	if (gParticleSystemManager->needSync)
	{
#if ENABLE_MULTITHREADED_PARTICLES
		SyncFence(gParticleSystemManager->jobGroup);
#endif

		gParticleSystemManager->needSync = false;

		float deltaTime = GetDeltaTime();
		if (deltaTime == 0.0f)
			return;

		for (size_t i = 0; i < gParticleSystemManager->activeEmitters.size(); ++i)
		{
			ParticleSystem& system = *gParticleSystemManager->activeEmitters[i];
			system.Update2(system, *system.m_InitState, *system.m_State, false);
		}
	}

	// todo
	//if (syncRenderJobs)
	//SyncRenderJobs();
}

void ParticleSystem::ResetSeeds()
{
	m_InitialModule.ResetSeed(*m_InitState);
	m_ShapeModule.ResetSeed(*m_InitState);
}

size_t ParticleSystem::EmitFromData(ParticleSystemEmissionState& emissionState, size_t& numContinuous, const ParticleSystemEmissionData& emissionData, const Vector3f velocity, float fromT, float toT, float dt, float length)
{
	size_t amountOfParticlesToEmit = 0;
	EmissionModule::Emit(emissionState, amountOfParticlesToEmit, numContinuous, emissionData, velocity, fromT, toT, dt, length);
	return amountOfParticlesToEmit;
}

size_t ParticleSystem::EmitFromModules(const ParticleSystem& system, const ParticleSystemInitState& initState
	, ParticleSystemEmissionState& emissionState, size_t& numContinuous, const Vector3f velocity, float fromT, float toT, float dt)
{
	if (system.m_EmissionModule.GetEnabled())
		return EmitFromData(emissionState, numContinuous, system.m_EmissionModule.GetEmissionDataRef(), velocity, fromT, toT, dt, initState.lengthInSec);
	return 0;
}

void ParticleSystem::Update0(ParticleSystem& system, const ParticleSystemInitState& initState, ParticleSystemState& state, float dt, bool fixedTimeStep)
{
	state.oldPosition = state.localToWorld.GetPosition();
	Matrix4x4f::Invert_General3D(state.localToWorld, state.WorldToLocal);

	// todo subEmitter

	if (system.m_ShapeModule.GetEnabled())
		system.m_ShapeModule.AcquireMeshData(state.WorldToLocal);
}

void ParticleSystem::Update1(ParticleSystem& system, ParticleSystemParticles& ps, float dt, bool fixedTimeStep, bool useProcedural, int rayBudget)
{
	const ParticleSystemInitState& initState = *system.m_InitState;
	ParticleSystemState& state = *system.m_State;

	// Exposed through scrip
	dt *= std::max<float>(initState.speed, 0.0f);

	float timeStep = GetTimeStep(dt); // todo fixed time

	if (timeStep < 0.00001f)
		return;

	if (state.playing)
	{
		state.accumulatedDt += dt;
		Update1Incremental(system, initState, state, ps, 0, timeStep, useProcedural);

		if (useProcedural)
			UpdateProcedural(system, initState, state, ps);
	}
}

void ParticleSystem::Update2(ParticleSystem& system, const ParticleSystemInitState& initState, ParticleSystemState& state, bool fixedTimeStep)
{
	// todo

	// Update renderer
	if (system.m_Renderer != nullptr)
	{
		// todo
		//MinMaxAABB result;
		//ParticleSystemRenderer::CombineBoundsRec(system, result, true);
		//renderer->Update(result);
	}
}

void ParticleSystem::Update1Incremental(ParticleSystem& system, const ParticleSystemInitState& initState, ParticleSystemState& state
	, ParticleSystemParticles& ps, size_t fromIndex, float dt, bool useProcedural)
{
	if (state.t == 0)
		ApplyStartDelay(state.delayT, state.accumulatedDt);

	float desiredDt = dt;
	int numTimeSteps = 0;
	const int numTimeStepsTotal = int(state.accumulatedDt / dt);

	while (state.accumulatedDt >= dt)
	{
		const float prevT = state.t;
		state.Tick(initState, dt);
		const float t = state.t;
		const bool timePassedDuration = t >= initState.lengthInSec;
		const float frameOffset = float(numTimeStepsTotal - 1 - numTimeSteps);

		if (!initState.looping && timePassedDuration)
			system.Stop();

		for (size_t i = 0; i < state.emitReplay.size(); i++)
			state.emitReplay[i].aliveTime += dt;

		// Emission
		bool emit = !state.stopEmitting;

		if (emit)
		{
			size_t numContinuous = 0;
			size_t amountOfParticlesToEmit = system.EmitFromModules(system, initState, state.emissionState, numContinuous, state.emitterVelocity, prevT, t, dt);
			if (useProcedural)
				StartParticlesProcedural(system, ps, prevT, t, dt, numContinuous, amountOfParticlesToEmit, frameOffset);
			else
				StartParticles(system, ps, prevT, t, dt, numContinuous, amountOfParticlesToEmit, frameOffset);
		}

		state.accumulatedDt -= dt;

		// todo
		// Workaround for external forces being dependent on AABB (need to update it before the next time step)
		//if (!useProcedural && (state.accumulatedDt >= dt) && system.m_ExternalForcesModule->GetEnabled())
		//	UpdateBounds(system, ps, state);

		numTimeSteps++;
	}
}

void ParticleSystem::UpdateProcedural(ParticleSystem& system, const ParticleSystemInitState& initState, ParticleSystemState& state
	, ParticleSystemParticles& ps)
{
	// Clear all particles
	ps.array_resize(0);

	const Matrix4x4f localToWorld = !initState.useLocalSpace ? state.localToWorld : Matrix4x4f::identity;

	// Emit all particles
	for (size_t i = 0; i < state.emitReplay.size(); i++)
	{
		const ParticleSystemEmitReplay& emit = state.emitReplay[i];

		//@TODO: remove passing m_State since that is very dangerous when making things procedural compatible
		size_t previousParticleCount = ps.array_size();
		system.m_InitialModule.GenerateProcedural(initState, state, ps, emit);

		//@TODO: This can be moved out of the emit all particles loop...
		if (system.m_ShapeModule.GetEnabled())
			system.m_ShapeModule.Start(initState, state, ps, localToWorld, previousParticleCount, emit.t);

		// Apply gravity & integrated velocity after shape module so that it picks up any changes done in shapemodule (for example rotating the velocity)
		Vector3f gravity = system.m_InitialModule.GetGravity(initState, state);
		float particleIndex = 0.0f;
		const size_t particleCount = ps.array_size();
		for (size_t q = previousParticleCount; q < particleCount; q++)
		{
			const float normalizedT = emit.t / initState.lengthInSec;
			ps.velocity[q] *= Evaluate(system.m_InitialModule.GetSpeedCurve(), normalizedT, GenerateRandom(ps.randomSeed[q] + kParticleSystemStartSpeedCurveId));
			Vector3f velocity = ps.velocity[q];
			float frameOffset = (particleIndex + emit.emissionOffset) * emit.emissionGap * float(particleIndex < emit.numContinuous);
			float aliveTime = emit.aliveTime + frameOffset;

			ps.position[q] += velocity * aliveTime + gravity * aliveTime * aliveTime * 0.5F;
			ps.velocity[q] += gravity * aliveTime;

			particleIndex += 1.0f;
		}

		// If no particles were emitted we can get rid of the emit replay state...
		if (previousParticleCount == ps.array_size())
		{
			state.emitReplay[i] = state.emitReplay.back();
			state.emitReplay.pop_back();
			i--;
		}
	}

	if (system.m_RotationModule->GetEnabled())
		system.m_RotationModule->UpdateProcedural(state, ps);
}

void ParticleSystem::UpdateModulesPreSimulationIncremental(const ParticleSystem& system, const ParticleSystemInitState& initState, const ParticleSystemState& state, ParticleSystemParticles& ps, const size_t fromIndex, const size_t toIndex, float dt)
{
	const size_t count = ps.array_size();
	system.m_InitialModule.Update(initState, state, ps, fromIndex, toIndex, dt);
	if (system.m_RotationModule->GetEnabled())
		system.m_RotationModule->Update(initState, state, ps, fromIndex, toIndex);
}

void ParticleSystem::UpdateModulesNonIncremental(const ParticleSystem& system, const ParticleSystemParticles& ps, ParticleSystemParticlesTempData& psTemp, size_t fromIndex, size_t toIndex)
{
	for (size_t i = fromIndex; i < toIndex; ++i)
		psTemp.color[i] = ps.color[i];
	for (size_t i = fromIndex; i < toIndex; ++i)
		psTemp.size[i] = ps.size[i];

	if (system.m_ColorModule->GetEnabled())
		system.m_ColorModule->Update(ps, psTemp.color, fromIndex, toIndex);

	if (system.m_SizeModule->GetEnabled())
		system.m_SizeModule->Update(ps, psTemp.size, fromIndex, toIndex);

	if (GetGraphicsCaps().needsToSwizzleVertexColors)
		std::transform(&psTemp.color[fromIndex], &psTemp.color[toIndex], &psTemp.color[fromIndex], SwizzleColorForPlatform);

	if (system.m_UVModule->GetEnabled())
	{
		if (!psTemp.sheetIndex)
		{
			psTemp.sheetIndex = new float[psTemp.particleCount];
			for (size_t i = 0; i < fromIndex; ++i)
				psTemp.sheetIndex[i] = 0.0f;
		}

		system.m_UVModule->Update(ps, psTemp.sheetIndex, fromIndex, toIndex);
	}
}

void ParticleSystem::StartModules(ParticleSystem& system, const ParticleSystemInitState& initState, ParticleSystemState& state
	, const ParticleSystemEmissionState& emissionState, Vector3f initialVelocity, const Matrix4x4f& matrix, ParticleSystemParticles& ps, size_t fromIndex, float dt, float t, size_t numContinuous, float frameOffset)
{
	system.m_InitialModule.Start(initState, state, ps, matrix, fromIndex, t);
	if (system.m_ShapeModule.GetEnabled())
		system.m_ShapeModule.Start(initState, state, ps, matrix, fromIndex, t);

	const float normalizedT = t / initState.lengthInSec;

	size_t count = ps.array_size();
	const Vector3f velocityOffset = system.m_InitialModule.GetInheritVelocity() * initialVelocity;
	for (size_t q = fromIndex; q < count; q++)
	{
		const float randomValue = GenerateRandom(ps.randomSeed[q] + kParticleSystemStartSpeedCurveId);
		ps.velocity[q] *= Evaluate(system.m_InitialModule.GetSpeedCurve(), normalizedT, randomValue);
		ps.velocity[q] += velocityOffset;
	}

	for (size_t q = fromIndex; q < count;) // array size changes
	{
		// subFrameOffset allows particles to be spawned at increasing times, thus spacing particles within a single frame.
		// For example if you spawn particles with high velocity you will get a continous streaming instead of a clump of particles.
		const int particleIndex = q - fromIndex;
		float subFrameOffset = (particleIndex < (int)numContinuous) ? (float(particleIndex) + emissionState.m_ToEmitAccumulator) * emissionState.m_ParticleSpacing : 0.0f;
		subFrameOffset = clamp01(subFrameOffset);

		// Update from curves and apply forces etc.
		UpdateModulesPreSimulationIncremental(system, initState, state, ps, q, q + 1, subFrameOffset * dt);

		// Position change due to where the emitter was at time of emission
		ps.position[q] -= initialVelocity * (frameOffset + subFrameOffset) * dt;

		// Position, rotation and energy change due to how much the particle has travelled since time of emission
		// @TODO: Call Simulate instead?
		ps.lifetime[q] -= subFrameOffset * dt;
		if ((ps.lifetime[q] < 0.0f) && (count > 0))
		{
			KillParticle(initState, state, ps, q, count);
			continue;
		}

		ps.position[q] += (ps.velocity[q] + ps.animatedVelocity[q]) * subFrameOffset * dt;

		if (ps.usesRotationalSpeed)
			ps.rotation[q] += ps.rotationalSpeed[q] * subFrameOffset * dt;

		++q;
	}
	ps.array_resize(count);
}

void ParticleSystem::StartParticles(ParticleSystem& system, ParticleSystemParticles& ps, const float prevT, const float t, const float dt, const size_t numContinuous, size_t amountOfParticlesToEmit, float frameOffset)
{
	if (amountOfParticlesToEmit <= 0)
		return;

	const ParticleSystemInitState& initState = *system.m_InitState;
	ParticleSystemState& state = *system.m_State;
	size_t fromIndex = system.AddNewParticles(ps, amountOfParticlesToEmit);
	const Matrix4x4f localToWorld = !initState.useLocalSpace ? state.localToWorld : Matrix4x4f::identity;
	StartModules(system, initState, state, state.emissionState, state.emitterVelocity, localToWorld, ps, fromIndex, dt, t, numContinuous, frameOffset);
}

void ParticleSystem::StartParticlesProcedural(ParticleSystem& system, ParticleSystemParticles& ps, const float prevT, const float t, const float dt, const size_t numContinuous, size_t amountOfParticlesToEmit, float frameOffset)
{
	ParticleSystemState& state = *system.m_State;

	int numParticlesRecorded = 0;
	for (size_t i = 0; i < state.emitReplay.size(); i++)
		numParticlesRecorded += state.emitReplay[i].particlesToEmit;

	float emissionOffset = state.emissionState.m_ToEmitAccumulator;
	float emissionGap = state.emissionState.m_ParticleSpacing * dt;
	amountOfParticlesToEmit = system.LimitParticleCount(numParticlesRecorded + amountOfParticlesToEmit) - numParticlesRecorded;

	if (amountOfParticlesToEmit > 0)
	{
		UInt32 randomSeed = 0;
		state.emitReplay.push_back(ParticleSystemEmitReplay(t, amountOfParticlesToEmit, emissionOffset, emissionGap, numContinuous, randomSeed));
	}
}

bool ParticleSystem::CheckSupportsProcedural(const ParticleSystem& system)
{
	// todo
	return true;
}

void ParticleSystem::Cull()
{
	m_State->culled = true;

	Clear(false);
	m_State->cullTime = GetCurTime();
	RemoveFromManager();
}

size_t ParticleSystem::LimitParticleCount(size_t requestSize) const
{
	const size_t maxNumParticles = m_InitialModule.GetMaxNumParticles();
	return std::min<size_t>(requestSize, maxNumParticles);
}

size_t ParticleSystem::AddNewParticles(ParticleSystemParticles& particles, size_t newParticles) const
{
	const size_t fromIndex = particles.array_size();
	const size_t newSize = LimitParticleCount(fromIndex + newParticles);
	particles.array_resize(newSize);
	return std::min(fromIndex, newSize);
}

bool ParticleSystem::ComputePrewarmStartParameters(float& prewarmTime, float t)
{
	// todo
	return true;
}

void ParticleSystem::Play(bool autoPrewarm)
{
	// If the ParticleSystem is a Sub Emitter, it needs to be added back to the Manager if it is not already playing
	// this is to fix bug 593535 where Sub Emitters do not play after being paused.
	if (m_State->GetIsSubEmitter())
	{
		if (!m_State->playing)
		{
			m_State->playing = true;
			m_State->needRestart = true;
			AddToManager();
		}

		return; // Must return so that we do not prewarm the Sub Emitters (This will cause an assert)
	}

	m_State->stopEmitting = false;
	m_State->playing = true;
	m_State->firstUpdate = true;

	if (m_State->needRestart)
	{
		if (m_InitState->prewarm)
		{
			if (autoPrewarm)
				AutoPrewarm();
		}
		else
		{
			m_State->delayT = m_InitState->startDelay;
		}

		m_State->playing = true;
		m_State->t = 0.0f;
		m_State->numLoops = 0;
		m_State->invalidateProcedural = false;
		m_State->accumulatedDt = 0.0f;
		m_State->emissionState.Clear();
	}

	if (m_State->culled && CheckSupportsProcedural(*this))
		Cull();
	else
		AddToManager();
}

void ParticleSystem::Stop()
{
	m_State->needRestart = true;
	m_State->stopEmitting = true;
	m_State->stopTime = GetCurTime();

	if (GetParticleCount() == 0)
		Clear(kStop | kUpdateBounds);
}

void ParticleSystem::Pause()
{
	m_State->playing = false;
	m_State->needRestart = false;
	RemoveFromManager();
}

void ParticleSystem::Clear(UInt32 flags)
{
	m_Particles->array_resize(0);
	m_State->emitReplay.resize_uninitialized(0);
	m_State->cullTime = 0.0f;
	m_State->stopTime = 0.0f;

	if (m_State->stopEmitting)
		m_State->playing = false;

	if (flags & kUpdateBounds)
	{
		// todo
		//UpdateBounds(*this, *m_Particles[kParticleBuffer0], *m_State);
	}
}

ParticleSystemParticles& ParticleSystem::GetParticles()
{
	return *m_Particles;
}

void ParticleSystem::PrepareForRender()
{
	m_Renderer->PrepareForRender(*this);
}

void ParticleSystem::AutoPrewarm()
{
	if (m_InitState->prewarm && m_InitState->looping)
	{
		Simulate(0.0f, true, true);
	}
}

void ParticleSystem::SetUsesAxisOfRotation()
{
	if (!m_Particles->usesAxisOfRotation)
		m_Particles->SetUsesAxisOfRotation();
}

void ParticleSystem::Simulate(float deltaTime, bool restart, bool fixedTimeStep)
{
	if (restart)
	{
		ResetSeeds();
		Stop();
		Clear(kUpdateBounds);
		Play(false);

		float updateTimeRaw = 0.0f;

		bool enableProcedural = !m_InitState->looping || (deltaTime == 0.0f);	// Procedural mode fails when applied to looping systems beyond their first cycle
		if (enableProcedural)
		{
			updateTimeRaw = deltaTime;

			// Don't apply the start delay if we are mid way through playing.
			if (m_State->t == 0)
				ApplyStartDelay(m_State->delayT, updateTimeRaw);
		}

		float prewarmTime;
		if (ComputePrewarmStartParameters(prewarmTime, updateTimeRaw))
		{
			if (enableProcedural)
			{
				Update(*this, prewarmTime, fixedTimeStep, CheckSupportsProcedural(*this));
			}
			else
			{
				Update(*this, prewarmTime, fixedTimeStep, CheckSupportsProcedural(*this));
				Update(*this, deltaTime, fixedTimeStep, false);
			}
			Pause();
		}
		else
		{
			Stop();
			Clear(kUpdateBounds);
		}
	}
	else
	{
		m_State->playing = true;
		Update(*this, deltaTime, fixedTimeStep, false);
		Pause();
	}
}

void ParticleSystem::AddToManager()
{
	if (m_EmittersIndex >= 0)
		return;

	size_t index = gParticleSystemManager->activeEmitters.size();
	gParticleSystemManager->activeEmitters.push_back(this);
	m_EmittersIndex = index;
}

void ParticleSystem::RemoveFromManager()
{
	if (m_EmittersIndex < 0)
		return;

	const int index = m_EmittersIndex;
	gParticleSystemManager->activeEmitters[index]->m_EmittersIndex = -1;
	gParticleSystemManager->activeEmitters[index] = gParticleSystemManager->activeEmitters.back();

	if (gParticleSystemManager->activeEmitters[index] != this)
		gParticleSystemManager->activeEmitters[index]->m_EmittersIndex = index;

	gParticleSystemManager->activeEmitters.pop_back();
}

int ParticleSystem::GetParticleCount() const
{
	return m_Particles->array_size();
}