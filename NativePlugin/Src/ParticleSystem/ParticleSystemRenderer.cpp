#include "PluginPrefix.h"
#include "ParticleSystemRenderer.h"
#include "UVModule.h"
#include "GfxDevice/GfxDevice.h"
#include "GfxDevice/ChannelAssigns.h"
#include "Graphics/Mesh/DynamicVBO.h"
#include "Shaders/GraphicsCaps.h"
#include "ParticleSystem/ParticleSystem.h"
#include "Graphics/Mesh/MeshVertexFormat.h"

static DefaultMeshVertexFormat gParticleVertexFormat(VERTEX_FORMAT5(Vertex, Normal, TexCoord0, Color, Tangent));
const int BILL_BOARD_VERTEX_COUNT = 4;

struct ParticleSystemGeomConstInputData
{
	Matrix4x4f m_ViewMatrix;
	//Vector3f m_CameraVelocity; // todo
	//Renderer* m_Renderer; // todo
	//UInt16 const* m_MeshIndexBuffer[ParticleSystemRendererData::kMaxNumParticleMeshes];
	//int m_MeshIndexCount[ParticleSystemRendererData::kMaxNumParticleMeshes];
	int m_NumTilesX;
	int m_NumTilesY;
	float numUVFrame;
	float animUScale;
	float animVScale;
	Vector3f xSpan;
	Vector3f ySpan;
	bool usesSheetIndex;
	float bentNormalFactor;
	Vector3f bentNormalVector;
};

void ParticleSystemRenderer::Render()
{

}

void ParticleSystemRenderer::Reset()
{
	m_Data.renderMode = kSRMBillboard;
	m_Data.lengthScale = 2.0F;
	m_Data.velocityScale = 0.0F;
	m_Data.cameraVelocityScale = 0.0F;
	m_Data.maxParticleSize = 0.5F;
	m_Data.sortingFudge = 0.0F;
	// todo
	//m_Data.sortMode = kSSMNone;
	m_Data.normalDirection = 1.0f;
}

void ParticleSystemRenderer::RenderMultiple(ParticleSystem& system)
{
	if (system.GetParticleCount() > 0)
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
}

extern Matrix4x4f gWorldMatrix;

void ParticleSystemRenderer::PrepareForRender(ParticleSystem& system)
{
	const bool needsAxisOfRotation = !GetScreenSpaceRotation();

	if (needsAxisOfRotation)
	{
		system.SetUsesAxisOfRotation();
	}

	ParticleSystemParticles* ps = &system.GetParticles();
	size_t particleCount = ps->array_size();
	ParticleSystemParticlesTempData psTemp;
	psTemp.color = new ColorRGBA32[particleCount];
	psTemp.size = new float[particleCount];
	psTemp.sheetIndex = 0;
	psTemp.particleCount = particleCount;

	if (system.m_UVModule->GetEnabled())
		psTemp.sheetIndex = new float[psTemp.particleCount];

	ParticleSystem::UpdateModulesNonIncremental(system, *ps, psTemp, 0, particleCount);

    m_DrawCallData.m_NumPaticles = system.GetParticleCount();

	if (m_DrawCallData.m_NumPaticles <= 0)
		return;

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

	Matrix4x4f viewMatrix = device.GetViewMatrix(); // todo

	ParticleSystemGeomConstInputData constData;
	constData.m_ViewMatrix = viewMatrix;
	constData.numUVFrame = constData.m_NumTilesX * constData.m_NumTilesY;
	constData.animUScale = 1.0f / (float)constData.m_NumTilesX;
	constData.animVScale = 1.0f / (float)constData.m_NumTilesY;
	constData.xSpan = Vector3f(-1.0f, 0.0f, 0.0f);
	constData.ySpan = Vector3f(0.0f, 0.0f, 1.0f);
	constData.xSpan = viewMatrix.MultiplyVector3(constData.xSpan);
	constData.ySpan = viewMatrix.MultiplyVector3(constData.ySpan);
	constData.usesSheetIndex = psTemp.sheetIndex != NULL;

	const float bentNormalAngle = m_Data.normalDirection * 90.0f * kDeg2Rad;
	const float scale = (m_Data.renderMode == kSRMBillboard) ? 0.707106781f : 1.0f;

	Matrix4x4f viewToWorldMatrix;
	Matrix4x4f::Invert_General3D(viewMatrix, viewToWorldMatrix);

	Matrix4x4f worldMatrix = gWorldMatrix;
	Matrix4x4f worldViewMatrix;
	MultiplyMatrices4x4(&viewMatrix, &worldMatrix, &worldViewMatrix);

	Vector3f billboardNormal = Vector3f::zAxis;
	constData.bentNormalVector = viewToWorldMatrix.MultiplyVector3(Sin(bentNormalAngle) * billboardNormal);
	constData.bentNormalFactor = Cos(bentNormalAngle) * scale;

	GenerateParticleGeometry((ParticleSystemVertex*)chunkHandle.vbPtr, constData, *ps, psTemp, worldViewMatrix, viewToWorldMatrix);
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

void ParticleSystemRenderer::GenerateParticleGeometry(ParticleSystemVertex* vbPtr
	, const ParticleSystemGeomConstInputData& constData
	, const ParticleSystemParticles& ps
	, const ParticleSystemParticlesTempData& psTemp
	, const Matrix4x4f& worldViewMatrix, const Matrix4x4f& viewToWorldMatrix)
{
	float numUVFrame = constData.numUVFrame;
	Vector3f xSpan = constData.xSpan;
	Vector3f ySpan = constData.ySpan;
	int numTilesX = constData.m_NumTilesX;
	float animUScale = constData.animUScale;
	float animVScale = constData.animVScale;
	bool usesSheetIndex = constData.usesSheetIndex;
	float invAnimVScale = 1.0f - animVScale;
	float bentNormalFactor = constData.bentNormalFactor;
	Vector3f bentNormalVector = constData.bentNormalVector;

	Vector2f uv[4] = { Vector2f(0.0f, 1.0f),
		Vector2f(1.0f, 1.0f),
		Vector2f(1.0f, 0.0f),
		Vector2f(0.0f, 0.0f) };

    Vector4f uv2[4] = { Vector4f(0.0f, 1.0f, 0.0f, 0.0f),
        Vector4f(1.0f, 1.0f, 0.0f, 0.0f),
        Vector4f(1.0f, 0.0f, 0.0f, 0.0f),
        Vector4f(0.0f, 0.0f, 0.0f, 0.0f) };

	for (int i = 0; i < ps.array_size(); ++i)
	{
		Vector3f vert[4];
		Vector3f n0, n1;
		Vector3f position;
		
		worldViewMatrix.MultiplyPoint3(ps.position[i], position);

		// todo fraction of size
		// 1. fraction of size
		// 2. only supported billBoard

		float hsize = psTemp.size[i] * 0.5f;
		float s = Sin(ps.rotation[i]);
		float c = Cos(ps.rotation[i]);
		n0 = Vector3f(-c + s, s + c, 0.0f);
		n1 = Vector3f(c + s, -s + c, 0.0f);
		vert[0] = position + n0 * hsize;
		vert[1] = position + n1 * hsize;
		vert[2] = position - n0 * hsize;
		vert[3] = position - n1 * hsize;

		// UV animation
		float sheetIndex;

		if (usesSheetIndex)
		{
			// TODO: Pretty much the perfect candidate for SIMD

			sheetIndex = psTemp.sheetIndex[i] * numUVFrame;

			const int index0 = FloorfToIntPos(sheetIndex);
			const int index1 = index0 + 1;
			Vector2f offset0, offset1;
			const float blend = sheetIndex - (float)index0;

			int vIdx = index0 / numTilesX;
			int uIdx = index0 - vIdx * numTilesX;
			offset0.x = (float)uIdx * animUScale;
			offset0.y = invAnimVScale - (float)vIdx * animVScale;

			vIdx = index1 / numTilesX;
			uIdx = index1 - vIdx * numTilesX;
			offset1.x = (float)uIdx * animUScale;
			offset1.y = invAnimVScale - (float)vIdx * animVScale;

			uv[0].Set(offset0.x, offset0.y + animVScale);
			uv[1].Set(offset0.x + animUScale, offset0.y + animVScale);
			uv[2].Set(offset0.x + animUScale, offset0.y);
			uv[3].Set(offset0.x, offset0.y);

			uv2[0].Set(offset1.x, offset1.y + animVScale, blend, 0.0f);
			uv2[1].Set(offset1.x + animUScale, offset1.y + animVScale, blend, 0.0f);
			uv2[2].Set(offset1.x + animUScale, offset1.y, blend, 0.0f);
			uv2[3].Set(offset1.x, offset1.y, blend, 0.0f);
		}

		n0 = viewToWorldMatrix.MultiplyVector3(n0 * bentNormalFactor);
		n1 = viewToWorldMatrix.MultiplyVector3(n1 * bentNormalFactor);
		ColorRGBA32 color = psTemp.color[i];

		vbPtr[0].vert = { 0.0f, 0.5f, 0.0f };
		vbPtr[0].color = color;
		//vbPtr[0].color = 0xff000000;
		vbPtr[0].normal = bentNormalVector + n0;
		vbPtr[0].uv = uv[0];
		vbPtr[0].tangent = uv2[0];

		vbPtr[1].vert = { 0.5f, 0.5f, 0.0f };
		vbPtr[1].color = color;
		//vbPtr[1].color = 0xff000000;
		vbPtr[1].normal = bentNormalVector + n1;
		vbPtr[1].uv = uv[1];
		vbPtr[1].tangent = uv2[1];

		vbPtr[2].vert = { 0.5f, 0.0f, 0.0f };
		vbPtr[2].color = color;
		//vbPtr[2].color = 0xff000000;
		vbPtr[2].normal = bentNormalVector - n0;
		vbPtr[2].uv = uv[2];
		vbPtr[2].tangent = uv2[2];

		vbPtr[3].vert = { 0.0f, 0.0f, 0.0f };
		vbPtr[3].color = color;
		//vbPtr[3].color = 0xff000000;
		vbPtr[3].normal = bentNormalVector - n1;
		vbPtr[3].uv = uv[3];
		vbPtr[3].tangent = uv2[3];

		vbPtr += BILL_BOARD_VERTEX_COUNT;
	}
}