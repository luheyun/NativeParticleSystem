#include "PluginPrefix.h"
#include "ParticleSystemRenderer.h"
#include "GfxDevice/GfxDevice.h"
#include "Graphics/Mesh/DynamicVBO.h"
#include "Shaders/GraphicsCaps.h"
#include "ParticleSystem/ParticleSystem.h"

void ParticleSystemRenderer::Render()
{

}

void ParticleSystemRenderer::RenderMultiple()
{

}

void ParticleSystemRenderer::PrepareForRender(ParticleSystem& system)
{
    m_DrawCallData.m_NumPaticles = system.GetParticleCount();
	GfxDevice& device = GetGfxDevice();
	DynamicVBO& vbo = device.GetDynamicVBO();
	DynamicVBOChunkHandle& chunkHandle = m_DrawCallData.m_VBOChunk;
    UInt32 stride = sizeof(ParticleSystemVertex);
	vbo.GetChunk(stride, m_DrawCallData.m_NumPaticles * stride, 0, GetGraphicsCaps().hasNativeQuad ? kPrimitiveQuads : kPrimitiveTriangles, &chunkHandle);
	GenerateParticleGeometry((ParticleSystemVertex*)chunkHandle.vbPtr);
}

void ParticleSystemRenderer::GenerateParticleGeometry(ParticleSystemVertex* vbPtr)
{
    Vector2f uv[4] = { Vector2f(0.0f, 1.0f),
                       Vector2f(1.0f, 1.0f),
                       Vector2f(1.0f, 0.0f),
                       Vector2f(0.0f, 0.0f) };

    Vector4f uv2[4] = { Vector4f(0.0f, 1.0f, 0.0f, 0.0f),
        Vector4f(1.0f, 1.0f, 0.0f, 0.0f),
        Vector4f(1.0f, 0.0f, 0.0f, 0.0f),
        Vector4f(0.0f, 0.0f, 0.0f, 0.0f) };

    vbPtr[0].vert = {0.0f, 0.5f, 0.0f};
    vbPtr[0].uv = uv[0];
    vbPtr[0].tangent = uv2[0];

    vbPtr[1].vert = { 0.5f, 0.5f, 0.0f };
    vbPtr[1].uv = uv[1];
    vbPtr[1].tangent = uv2[1];

    vbPtr[2].vert = { 0.5f, 0.0f, 0.0f };
    vbPtr[2].uv = uv[2];
    vbPtr[2].tangent = uv2[2];

    vbPtr[3].vert = { 0.0f, 0.0f, 0.0f };
    vbPtr[3].uv = uv[3];
    vbPtr[3].tangent = uv2[3];

    vbPtr += 4;
}