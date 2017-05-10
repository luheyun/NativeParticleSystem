#pragma once

#include "Graphics/Renderer.h"
#include "Math/Vector3.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Math/Color.h"
#include "Graphics/Mesh/GenericDynamicVBO.h"

enum ParticleSystemRenderMode {
	kSRMBillboard = 0,
	kSRMStretch3D = 1,
	kSRMBillboardFixedHorizontal = 2,
	kSRMBillboardFixedVertical = 3,
	kSRMMesh = 4,
};

struct ParticleSystemRendererData
{
	enum { kMaxNumParticleMeshes = 4 };

	int		renderMode;				///< enum { Billboard = 0, Stretched = 1, Horizontal Billboard = 2, Vertical Billboard = 3, Mesh = 4 }
	int		sortMode;				///< enum { None = 0, By Distance = 1, Youngest First = 2, Oldest First = 3 }
	float	maxParticleSize = 0.5F;		///< How large is a particle allowed to be on screen at most? 1 is entire viewport. 0.5 is half viewport.
	float	cameraVelocityScale;	///< How much the camera motion is factored in when determining particle stretching.
	float	velocityScale;			///< When Stretch Particles is enabled, defines the length of the particle compared to its velocity.
	float	lengthScale;			///< When Stretch Particles is enabled, defines the length of the particle compared to its width.
	float	sortingFudge;			///< Lower the number, most likely that these particles will appear in front of other transparent objects, including other particles.	
	float	normalDirection;		///< Value between 0.0 and 1.0. If 1.0 is used, normals will point towards camera. If 0.0 is used, normals will point out in the corner direction of the particle.
	//Mesh*	cachedMesh[kMaxNumParticleMeshes];

	/// Node hooked into the mesh user list of cached meshes so we get notified
	/// when a mesh goes away.
	///
	/// NOTE: Must be initialized properly after construction to point to the
	///		  ParticleSystemRenderer.
	//ListNode<Object> cachedMeshUserNode[kMaxNumParticleMeshes];
};

struct ParticleSystemVertex
{
	Vector3f vert;
	Vector3f normal;
	ColorRGBA32 color;
	Vector2f uv;
	Vector4f tangent;
};

class ParticleSystem;
class Matrix4x4f;
struct ParticleSystemParticles;
struct ParticleSystemGeomConstInputData;
struct ParticleSystemParticlesTempData;

class ParticleSystemRenderer : public Renderer
{
public:
    struct DrawCallData
    {
        DynamicVBOChunkHandle m_VBOChunk;
        UInt32 m_NumPaticles;
		UInt16 m_Stride;
		UInt16 m_IndexCount;
    };

	virtual void Render() override;
	virtual void Reset();
	void RenderMultiple(ParticleSystem& system);

	void PrepareForRender(ParticleSystem& system);
	void PrepareForMeshRender() {}

	// For mesh we use world space rotation, else screen space
	bool GetScreenSpaceRotation() const { return m_Data.renderMode != kSRMMesh; };

    friend class ParticleSystemGeometryJob;

private:
	void GenerateParticleGeometry(ParticleSystemVertex* vbPtr, const ParticleSystemGeomConstInputData& constData
		, const ParticleSystemParticles& ps, const ParticleSystemParticlesTempData& psTemp, const Matrix4x4f& worldViewMatrix
		, const Matrix4x4f& viewToWorldMatrix);
    void GenerateIndicesForBillBoard(UInt16* ibPtr, UInt32 indexCount);

    DrawCallData m_DrawCallData;
	ParticleSystemRendererData m_Data;
};