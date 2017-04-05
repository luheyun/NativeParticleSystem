#include "PluginPrefix.h"
#include "InitialModule.h"

InitialModule::InitialModule() : ParticleSystemModule(true)
, m_GravityModifier(0.0f)
, m_InheritVelocity(0.0f)
, m_MaxNumParticles(1000)
{

}

void InitialModule::Start(const ParticleSystemInitState& initState, const ParticleSystemState& state, ParticleSystemParticles& ps, const Matrix4x4f& matrix, size_t fromIndex, float t)
{
	const float normalizedT = t / initState.lengthInSec;

	Rand& random = GetRandom();

	Vector3f origin = matrix.GetPosition();

	const size_t count = ps.array_size();
	for (size_t q = fromIndex; q < count; ++q)
	{
		UInt32 randUInt32 = random.Get();
		float rand = Rand::GetFloatFromInt(randUInt32);
		UInt32 randByte = Rand::GetByteFromInt(randUInt32);

		const ColorRGBA32 col = Evaluate(m_Color, normalizedT, randByte);
		float sz = std::max<float>(0.0f, Evaluate(m_Size, normalizedT, rand));
		Vector3f vel = matrix.MultiplyVector3(Vector3f::zAxis);
		float ttl = std::max<float>(0.0f, Evaluate(m_Lifetime, normalizedT, rand));
		float rot = Evaluate(m_Rotation, normalizedT, rand);

		ps.position[q] = origin;
		ps.velocity[q] = vel;
		ps.animatedVelocity[q] = Vector3f::zero;
		ps.lifetime[q] = 100.0f;// ttl;
		ps.startLifetime[q] = ttl;
		ps.size[q] = sz;
		ps.rotation[q] = rot;
		if (ps.usesRotationalSpeed)
			ps.rotationalSpeed[q] = 0.0f;
		ps.color[q] = col;
		ps.randomSeed[q] = random.Get(); // One more iteration to avoid visible patterns between random spawned parameters and those used in update
		if (ps.usesAxisOfRotation)
			ps.axisOfRotation[q] = Vector3f::zAxis;
		for (int acc = 0; acc < ps.numEmitAccumulators; acc++)
			ps.emitAccumulator[acc][q] = 0.0f;

	}
}

void InitialModule::Update(const ParticleSystemInitState& initState, const ParticleSystemState& state, ParticleSystemParticles& ps, const size_t fromIndex, const size_t toIndex, float dt) const
{
	// todo
	//Vector3f gravityDelta = GetGravity(initState, state) * dt;
	//if (!CompareApproximately(gravityDelta, Vector3f::zero, 0.0001f))
	//	for (size_t q = fromIndex; q < toIndex; ++q)
	//		ps.velocity[q] += gravityDelta;

	for (size_t q = fromIndex; q < toIndex; ++q)
		ps.animatedVelocity[q] = Vector3f::zero;

	if (ps.usesRotationalSpeed)
		for (size_t q = fromIndex; q < toIndex; ++q)
			ps.rotationalSpeed[q] = 0.0f;
}

void InitialModule::GenerateProcedural(const ParticleSystemInitState& initState, const ParticleSystemState& state, ParticleSystemParticles& ps, const ParticleSystemEmitReplay& emit)
{
	size_t count = emit.particlesToEmit;
	float t = emit.t;
	float alreadyPassedTime = emit.aliveTime;

	const float normalizedT = t / initState.lengthInSec;

	Rand& random = GetRandom();

	const Matrix4x4f localToWorld = !initState.useLocalSpace ? state.localToWorld : Matrix4x4f::identity;
	Vector3f origin = localToWorld.GetPosition();
	for (size_t i = 0; i < count; ++i)
	{
		UInt32 randUInt32 = random.Get();
		float rand = Rand::GetFloatFromInt(randUInt32);
		UInt32 randByte = Rand::GetByteFromInt(randUInt32);

		float frameOffset = (float(i) + emit.emissionOffset) * emit.emissionGap * float(i < emit.numContinuous);

		const ColorRGBA32 col = Evaluate(m_Color, normalizedT, randByte);
		float sz = std::max<float>(0.0f, Evaluate(m_Size, normalizedT, rand));
		Vector3f vel = localToWorld.MultiplyVector3(Vector3f::zAxis);
		float ttlStart = std::max<float>(0.0f, Evaluate(m_Lifetime, normalizedT, rand));
		float ttl = ttlStart - alreadyPassedTime - frameOffset;
		float rot = Evaluate(m_Rotation, normalizedT, rand);

		if (ttl < 0.0F)
			continue;

		size_t q = ps.array_size();
		ps.array_resize(ps.array_size() + 1);

		ps.position[q] = origin;
		ps.velocity[q] = vel;
		ps.animatedVelocity[q] = Vector3f::zero;
		ps.lifetime[q] = ttl;
		ps.startLifetime[q] = ttlStart;
		ps.size[q] = sz;
		ps.rotation[q] = rot;
		if (ps.usesRotationalSpeed)
			ps.rotationalSpeed[q] = 0.0f;
		ps.color[q] = col;
		ps.randomSeed[q] = random.Get(); // One more iteration to avoid visible patterns between random spawned parameters and those used in update
		if (ps.usesAxisOfRotation)
			ps.axisOfRotation[q] = Vector3f::zAxis;
		for (int acc = 0; acc < ps.numEmitAccumulators; acc++)
			ps.emitAccumulator[acc][q] = 0.0f;
	}
}

void InitialModule::ResetSeed(const ParticleSystemInitState& initState)
{
	m_Random.SetSeed(initState.randomSeed);
}

Vector3f InitialModule::GetGravity(const ParticleSystemInitState& initState, const ParticleSystemState& state) const
{
	// todo PHYSICS
	return Vector3f::zero;
}