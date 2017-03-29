#ifndef PARTICLESYSTEMCOMMON_H
#define PARTICLESYSTEMCOMMON_H

enum ParticleSystemSubType
{
	kParticleSystemSubTypeBirth,
	kParticleSystemSubTypeCollision,
	kParticleSystemSubTypeDeath,
};

enum ParticleSystemEmitMode
{
	kParticleSystemEMDirect,
	kParticleSystemEMStaging,
};

enum
{
	kParticleSystemMaxSubBirth = 2,
	kParticleSystemMaxSubCollision = 2,
	kParticleSystemMaxSubDeath = 2,
	kParticleSystemMaxSubTotal = kParticleSystemMaxSubBirth + kParticleSystemMaxSubCollision + kParticleSystemMaxSubDeath,
};

// Curve id's needed to offset randomness for curves, to avoid visible patterns due to only storing 1 random value per particle
enum ParticleSystemRandomnessIds
{
	// Curves
	kParticleSystemClampVelocityCurveId = 0x13371337,
	kParticleSystemForceCurveId = 0x12460f3b,
	kParticleSystemRotationCurveId = 0x6aed452e,
	kParticleSystemRotationBySpeedCurveId = 0xdec4aea1,
	kParticleSystemStartSpeedCurveId = 0x96aa4de3,
	kParticleSystemSizeCurveId = 0x8d2c8431,
	kParticleSystemSizeBySpeedCurveId = 0xf3857f6f,
	kParticleSystemVelocityCurveId = 0xe0fbd834,
	kParticleSystemUVCurveId = 0x13740583,

	// Gradient
	kParticleSystemColorGradientId = 0x591bc05c,
	kParticleSystemColorByVelocityGradientId = 0x40eb95e4,

	// Misc
	kParticleSystemMeshSelectionId = 0xbc524e5f,
	kParticleSystemUVRowSelectionId = 0xaf502044,
};

#endif // PARTICLESYSTEMCOMMON_H
