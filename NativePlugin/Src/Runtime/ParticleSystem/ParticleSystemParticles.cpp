#include "PluginPrefix.h"
#include "ParticleSystemParticles.h"

void ParticleSystemParticles::array_resize(size_t i)
{
	position.resize_uninitialized(i);
	velocity.resize_uninitialized(i);
	animatedVelocity.resize_uninitialized(i);
	rotation.resize_uninitialized(i);
	if (usesRotationalSpeed)
		rotationalSpeed.resize_uninitialized(i);
	size.resize_uninitialized(i);
	color.resize_uninitialized(i);
	randomSeed.resize_uninitialized(i);
	lifetime.resize_uninitialized(i);
	startLifetime.resize_uninitialized(i);
	if (usesAxisOfRotation)
		axisOfRotation.resize_uninitialized(i);
	for (int acc = 0; acc < numEmitAccumulators; acc++)
		emitAccumulator[acc].resize_uninitialized(i);
}

void ParticleSystemParticles::element_assign(size_t i, size_t j)
{
	position[i] = position[j];
	velocity[i] = velocity[j];
	animatedVelocity[i] = animatedVelocity[j];
	rotation[i] = rotation[j];
	if (usesRotationalSpeed)
		rotationalSpeed[i] = rotationalSpeed[j];
	size[i] = size[j];
	color[i] = color[j];
	randomSeed[i] = randomSeed[j];
	lifetime[i] = lifetime[j];
	startLifetime[i] = startLifetime[j];
	if (usesAxisOfRotation)
		axisOfRotation[i] = axisOfRotation[j];
	for (int acc = 0; acc < numEmitAccumulators; acc++)
		emitAccumulator[acc][i] = emitAccumulator[acc][j];
}

void ParticleSystemParticles::SetUsesAxisOfRotation()
{
	usesAxisOfRotation = true;
	const size_t count = position.size();
	axisOfRotation.resize_uninitialized(count);
	for (size_t i = 0; i < count; i++)
		axisOfRotation[i] = Vector3f::yAxis;
}

void ParticleSystemParticles::SetUsesRotationalSpeed()
{
    usesRotationalSpeed = true;
    const size_t count = position.size();
    rotationalSpeed.resize_uninitialized(count);
    for (size_t i = 0; i < count; i++)
        rotationalSpeed[i] = 0.0f;
}

ParticleSystemParticlesTempData::ParticleSystemParticlesTempData(UInt32 numParticles, bool needsSheet, bool needs3DSize)
    : color(NULL)
    , sheetIndex(NULL)
    , particleCount(numParticles)
{
    size[0] = size[1] = size[2] = NULL;

    if (numParticles)
    {
        size_t numParticlesRounded = RoundUpMultiple<size_t>(numParticles, math::simd_width);	// round up to SIMD boundary
        size_t memSize = numParticlesRounded * sizeof(ColorRGBA32);
        memSize += numParticlesRounded * sizeof(float) * (needs3DSize ? 3 : 1);
        if (needsSheet)
            memSize += numParticlesRounded * sizeof(float);
        float* mem = (float*)UNITY_MALLOC_ALIGNED(kMemTempJobAlloc, memSize, kDefaultMemoryAlignment);

        color = (ColorRGBA32*)mem;
        mem += numParticlesRounded;

        size[0] = mem;
        mem += numParticlesRounded;

        if (needs3DSize)
        {
            size[1] = mem;
            mem += numParticlesRounded;

            size[2] = mem;
            mem += numParticlesRounded;
        }
        else
        {
            size[1] = size[0];
            size[2] = size[0];
        }

        if (needsSheet)
        {
            sheetIndex = mem;
            mem += numParticlesRounded;
        }
    }
}

ParticleSystemParticlesTempData::ParticleSystemParticlesTempData()
	:color(0)
	, size(0)
	, sheetIndex(0)
	, particleCount(0)
{}

void ParticleSystemParticlesTempData::element_swap(size_t i, size_t j)
{
	std::swap(color[i], color[j]);
	std::swap(size[i], size[j]);
	if (sheetIndex)
		std::swap(sheetIndex[i], sheetIndex[j]);
}