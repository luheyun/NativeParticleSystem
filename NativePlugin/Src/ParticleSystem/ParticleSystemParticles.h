#pragma once

#include "Math/Color.h"
#include "Utilities/dynamic_array.h"
#include "Math/Vector3.h"

// Keep in sync with struct ParticleSystem.Particle
enum{ kParticleSystemMaxNumEmitAccumulators = 2 };

// todo
typedef dynamic_array<float> ParticleSystemFloatArray;
typedef dynamic_array<ColorRGBA32> ParticleSystemColor32Array;
typedef dynamic_array<UInt32> ParticleSystemUInt32Array;
typedef dynamic_array<Vector3f> ParticleSystemVector3Array;

struct ParticleSystemParticles
{
	ParticleSystemParticles()
		:usesAxisOfRotation(false)
		, usesRotationalSpeed(false)
		, uses3DRotation(false)
		, uses3DSize(false)
		, usesInitialVelocity(false)
		, usesCollisionEvents(false)
		, usesTriggerEvents(false)
		, numEmitAccumulators(0)
		, refCount(1)
	{}

	ParticleSystemVector3Array position;
	ParticleSystemVector3Array velocity;
	ParticleSystemVector3Array animatedVelocity; // Would actually only need this when modules with force and velocity curves are used
	ParticleSystemVector3Array initialVelocity;
	ParticleSystemVector3Array axisOfRotation;
	ParticleSystemFloatArray rotation;
	ParticleSystemFloatArray rotationalSpeed;
	ParticleSystemFloatArray size;
	ParticleSystemColor32Array color;
	ParticleSystemUInt32Array randomSeed;
	ParticleSystemFloatArray lifetime;
	ParticleSystemFloatArray startLifetime;
	ParticleSystemFloatArray emitAccumulator[kParticleSystemMaxNumEmitAccumulators]; // Usage: Only needed if particle system has time sub emitter

	bool usesAxisOfRotation;
	bool usesRotationalSpeed;
	bool uses3DRotation;
	bool uses3DSize;
	bool usesInitialVelocity;
	bool usesCollisionEvents;
	bool usesTriggerEvents;
	int numEmitAccumulators;

	volatile int refCount;

	size_t array_size() const { return position.size(); }
	void array_resize(size_t i);
	void element_assign(size_t i, size_t j);
};

inline float NormalizedTime(const ParticleSystemParticles& ps, size_t i)
{
	return (ps.startLifetime[i] - ps.lifetime[i]) / ps.startLifetime[i];
}