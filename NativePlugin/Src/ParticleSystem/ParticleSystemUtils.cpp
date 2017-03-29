#include "PluginPrefix.h"
#include "ParticleSystemUtils.h"
#include "ParticleSystem.h"

UInt32 randomSeed = 0x1337;

UInt32 GetGlobalRandomSeed ()
{
	return ++randomSeed;
}

void ResetGlobalRandomSeed ()
{
	randomSeed = 0x1337;
}

Vector2f CalculateInverseLerpOffsetScale (const Vector2f& range)
{
	float scale = 1.0F / (range.y - range.x);
	return Vector2f (scale, -range.x * scale);
}

void CalculatePositionAndVelocity(Vector3f& initialPosition, Vector3f& initialVelocity, const ParticleSystemInitState& roState, const ParticleSystemState& state, const ParticleSystemParticles& ps, const size_t index)
{
	initialPosition = ps.position[index];
	initialVelocity = ps.velocity[index] + ps.animatedVelocity[index];
	if(roState.useLocalSpace)
	{
		// If we are in local space, transform to world space to make independent of this emitters transform
		initialPosition = state.localToWorld.MultiplyPoint3(initialPosition);
		initialVelocity = state.localToWorld.MultiplyVector3(initialVelocity);
	}
}

void KillParticle(const ParticleSystemInitState& initState, ParticleSystemState& state, ParticleSystemParticles& ps, size_t index, size_t& particleCount)
{
	ps.element_assign (index, particleCount - 1);
	--particleCount;
}

bool GetTransformationMatrix(Matrix4x4f& output, const bool isSystemInWorld, const bool isCurveInWorld, const Matrix4x4f& localToWorld)
{
	if(isCurveInWorld != isSystemInWorld)
	{
		if(isSystemInWorld)
			output = localToWorld;
		else 
			Matrix4x4f::Invert_General3D(localToWorld, output);
		return true;
	}
	else
	{
		output = Matrix4x4f::identity;
		return false;
	}	
}

bool GetTransformationMatrices(Matrix4x4f& output, Matrix4x4f& outputInverse, const bool isSystemInWorld, const bool isCurveInWorld, const Matrix4x4f& localToWorld)
{
	if(isCurveInWorld != isSystemInWorld)
	{
		if(isSystemInWorld)
		{
			output = localToWorld;
			Matrix4x4f::Invert_General3D(localToWorld, outputInverse);
		}
		else 
		{
			Matrix4x4f::Invert_General3D(localToWorld, output);
			outputInverse = localToWorld;
		}
		return true;
	}
	else
	{
		output = Matrix4x4f::identity;
		outputInverse = Matrix4x4f::identity;
		return false;
	}	
}
