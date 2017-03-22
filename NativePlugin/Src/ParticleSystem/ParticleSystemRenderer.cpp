#include "PluginPrefix.h"
#include "ParticleSystemRenderer.h"
#include "GfxDevice/GfxDevice.h"
#include "GfxDevice/ChannelAssigns.h"
#include "Graphics/Mesh/DynamicVBO.h"
#include "Shaders/GraphicsCaps.h"
#include "ParticleSystem/ParticleSystem.h"
#include "Graphics/Mesh/MeshVertexFormat.h"

static DefaultMeshVertexFormat gParticleVertexFormat(VERTEX_FORMAT5(Vertex, Normal, TexCoord0, Color, Tangent));
const int BILL_BOARD_VERTEX_COUNT = 4;

void ParticleSystemRenderer::Render()
{

}

void ParticleSystemRenderer::RenderMultiple()
{
	DynamicVBO::DrawParams params(m_DrawCallData.m_Stride, 0, BILL_BOARD_VERTEX_COUNT, 0, m_DrawCallData.m_IndexCount);
	GfxDevice& device = GetGfxDevice();
	DynamicVBO& vbo = device.GetDynamicVBO();
	ChannelAssigns* channel = new ChannelAssigns();
	channel->Bind(kShaderChannelVertex, kVertexCompVertex);
	channel->Bind(kShaderChannelColor, kVertexCompColor);
    channel->Bind(kShaderChannelNormal, kVertexCompNormal);
    channel->Bind(kShaderChannelTexCoord0, kVertexCompTexCoord0);
    channel->Bind(kShaderChannelTangent, kVertexCompNone);
	vbo.DrawChunk(m_DrawCallData.m_VBOChunk, *channel, gParticleVertexFormat.GetVertexFormat()->GetAvailableChannels(), gParticleVertexFormat.GetVertexFormat()->GetVertexDeclaration(channel->GetSourceMap()), &params, 1);
}

void ParticleSystemRenderer::PrepareForRender(ParticleSystem& system)
{
    m_DrawCallData.m_NumPaticles = system.GetParticleCount();
	GfxDevice& device = GetGfxDevice();
	DynamicVBO& vbo = device.GetDynamicVBO();
	DynamicVBOChunkHandle& chunkHandle = m_DrawCallData.m_VBOChunk;
	m_DrawCallData.m_Stride = sizeof(ParticleSystemVertex);
	m_DrawCallData.m_IndexCount = 0;

	if (!GetGraphicsCaps().hasNativeQuad)
		m_DrawCallData.m_IndexCount = 6;

	vbo.GetChunk(m_DrawCallData.m_Stride, m_DrawCallData.m_NumPaticles * m_DrawCallData.m_Stride * BILL_BOARD_VERTEX_COUNT, m_DrawCallData.m_NumPaticles * m_DrawCallData.m_IndexCount, GetGraphicsCaps().hasNativeQuad ? kPrimitiveQuads : kPrimitiveTriangles, &chunkHandle);

    if (!GetGraphicsCaps().hasNativeQuad)
        GenerateIndicesForBillBoard(chunkHandle.ibPtr, m_DrawCallData.m_IndexCount);

	GenerateParticleGeometry((ParticleSystemVertex*)chunkHandle.vbPtr);
	vbo.ReleaseChunk(chunkHandle, m_DrawCallData.m_NumPaticles * m_DrawCallData.m_Stride * BILL_BOARD_VERTEX_COUNT, m_DrawCallData.m_NumPaticles * m_DrawCallData.m_IndexCount);
}

void ParticleSystemRenderer::GenerateIndicesForBillBoard(UInt16* ibPtr, UInt32 indexCount)
{
    UInt16* dest = ibPtr;
    UInt32 numParticles = indexCount / 6;
    UInt32 numVertices = numParticles * 4;
    for (int i = 0; i < numVertices; i += 4)
    {
        *dest++ = i + 0;
        *dest++ = i + 1;
        *dest++ = i + 2;
        *dest++ = i + 0;
        *dest++ = i + 2;
        *dest++ = i + 3;
    }

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
	vbPtr[0].color = 0xFFff00ff;
    vbPtr[0].uv = uv[0];
    vbPtr[0].tangent = uv2[0];

    vbPtr[1].vert = { 0.5f, 0.5f, 0.0f };
	vbPtr[1].color = 0xFF00ffff;
    vbPtr[1].uv = uv[1];
    vbPtr[1].tangent = uv2[1];

    vbPtr[2].vert = { 0.5f, 0.0f, 0.0f };
	vbPtr[2].color = 0xFF0000ff;
    vbPtr[2].uv = uv[2];
    vbPtr[2].tangent = uv2[2];

    vbPtr[3].vert = { 0.0f, 0.0f, 0.0f };
	vbPtr[3].color = 0xFF0000ff;
    vbPtr[3].uv = uv[3];
    vbPtr[3].tangent = uv2[3];

	vbPtr += BILL_BOARD_VERTEX_COUNT;
}