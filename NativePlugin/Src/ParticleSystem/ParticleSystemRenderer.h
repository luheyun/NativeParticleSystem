#pragma once

#include "Graphics/Renderer.h"
#include "Math/Vector3.h"
#include "Math/Vector2.h"
#include "Math/Vector4.h"
#include "Math/Color.h"
#include "Graphics/Mesh/GenericDynamicVBO.h"

struct ParticleSystemVertex
{
	Vector3f vert;
	Vector3f normal;
	ColorRGBA32 color;
	Vector2f uv;
	Vector4f tangent;
};

class ParticleSystem;

class ParticleSystemRenderer : Renderer
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
	void RenderMultiple();

	void PrepareForRender(ParticleSystem& system);
	void PrepareForMeshRender() {}

private:
	void GenerateParticleGeometry(ParticleSystemVertex* vbPtr);
    void GenerateIndicesForBillBoard(UInt16* ibPtr, UInt32 indexCount);

    DrawCallData m_DrawCallData;
};