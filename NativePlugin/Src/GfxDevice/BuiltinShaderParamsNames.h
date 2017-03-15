#pragma once

// We need the set of "known" glstate parameters for both non-OpenGL rendering runtimes and
// the CgBatch compiler (so that it can report errors for unsupported ones).
// Hence they are in a separate file here.

const char* GetShaderInstanceMatrixParamName(int paramIndex);

const char* GetBuiltinVectorParamName(int paramIndex);
const char* GetBuiltinMatrixParamName(int paramIndex);
const char* GetBuiltinTexEnvParamName(int paramIndex);

bool IsShaderInstanceMatrixParam(const char* name, int* paramIndex=0);

bool IsVectorBuiltinParam(const char* name, int* paramIndex=0);
bool IsMatrixBuiltinParam(const char* name, int* paramIndex=0);
bool IsTexEnvBuiltinParam(const char* name, int* paramIndex=0);

void InitializeBuiltinShaderParamNames ();
void CleanupBuiltinShaderParamNames ();

bool IsBuiltinArrayName(const char* name);

// Matrices set by unity as "per-instance" data for each object.
enum ShaderBuiltinInstanceMatrixParam
{
	kShaderInstanceMatMVP = 0,
	kShaderInstanceMatMV,
	kShaderInstanceMatM,
	kShaderInstanceMatInvM,
	kShaderInstanceMatTransMV,
	kShaderInstanceMatInvTransMV,
	
	kShaderInstanceMatCount
};

enum BuiltinShaderMatrixParam
{
	kShaderMatProj = 0,
	kShaderMatView,
	kShaderMatInvView,
	kShaderMatViewProj,
	kShaderMatWorldToCamera,
	kShaderMatCameraToWorld,

	kShaderMatWorldToShadow,
	kShaderMatWorldToShadow1,
	kShaderMatWorldToShadow2,
	kShaderMatWorldToShadow3,
	kShaderMatLightmapMatrix,
	kShaderMatProjector,
	kShaderMatProjectorDistance,
	kShaderMatProjectorClip,
	kShaderMatGUIClip,
	kShaderMatLightMatrix,

	kShaderMatCameraProjection,
	kShaderMatCameraInvProjection,
	kShaderMatProbeVolumeWorldToObject,

	kShaderMatCount
};

enum BuiltinShaderVectorParam
{
	kShaderVecLight0Diffuse = 0,
	kShaderVecLight1Diffuse,
	kShaderVecLight2Diffuse,
	kShaderVecLight3Diffuse,
	kShaderVecLight4Diffuse,
	kShaderVecLight5Diffuse,
	kShaderVecLight6Diffuse,
	kShaderVecLight7Diffuse,
	kShaderVecLight0Position,
	kShaderVecLight1Position,
	kShaderVecLight2Position,
	kShaderVecLight3Position,
	kShaderVecLight4Position,
	kShaderVecLight5Position,
	kShaderVecLight6Position,
	kShaderVecLight7Position,
	kShaderVecLight0SpotDirection,
	kShaderVecLight1SpotDirection,
	kShaderVecLight2SpotDirection,
	kShaderVecLight3SpotDirection,
	kShaderVecLight4SpotDirection,
	kShaderVecLight5SpotDirection,
	kShaderVecLight6SpotDirection,
	kShaderVecLight7SpotDirection,
	kShaderVecLight0Atten,
	kShaderVecLight1Atten,
	kShaderVecLight2Atten,
	kShaderVecLight3Atten,
	kShaderVecLight4Atten,
	kShaderVecLight5Atten,
	kShaderVecLight6Atten,
	kShaderVecLight7Atten,
	kShaderVecVertexLightParams,
	kShaderVecLightModelAmbient,
	kShaderVecWorldSpaceLightPos0,
	kShaderVecLightColor0,
	kShaderVecWorldSpaceCameraPos,
	kShaderVecWorldTransformParams,
	kShaderVecProjectionParams,
	kShaderVecScreenParams,
	kShaderVecZBufferParams,
	kShaderVecOrthoParams,
	kShaderVecLightPositionRange,
	kShaderVecUnityAmbient, //@TODO: kill it; replace all uses with LightModelAmbient
	kShaderVecLightmapFade,
	kShaderVecShadowOffset0,
	kShaderVecShadowOffset1,
	kShaderVecShadowOffset2,
	kShaderVecShadowOffset3,
	kShaderVecLightShadowData,
	kShaderVecLightShadowBias,
	kShaderVecLightSplitsNear,
	kShaderVecLightSplitsFar,
	kShaderVecShadowSplitSpheres0,
	kShaderVecShadowSplitSpheres1,
	kShaderVecShadowSplitSpheres2,
	kShaderVecShadowSplitSpheres3,
	kShaderVecShadowSplitSqRadii,
	kShaderVecShadowCascadeScales,
	kShaderVecShadowFadeCenterAndType,
	kShaderVecLODFade,
	kShaderVecUnityLightmapST,
	kShaderVecUnityDynamicLightmapST,
	kShaderVecSHAr,
	kShaderVecSHAg,
	kShaderVecSHAb,
	kShaderVecSHBr,
	kShaderVecSHBg,
	kShaderVecSHBb,
	kShaderVecSHC,
	kShaderVecTime,
	kShaderVecSinTime,
	kShaderVecCosTime,
	kShaderVecPiTime,
	kShaderVecDeltaTime,
	kShaderVecVertexLightPosX0,
	kShaderVecVertexLightPosY0,
	kShaderVecVertexLightPosZ0,
	kShaderVecVertexLightAtten0,
	kShaderVecUnityFogStart,
	kShaderVecUnityFogEnd,
	kShaderVecUnityFogDensity,
	kShaderVecUnityFogColor,
	kShaderVecUnityFogParams,
	kShaderVecColorSpaceGrey,
	kShaderVecColorSpaceDouble,
	kShaderVecColorSpaceLuminance,
	kShaderVecNPOTScale,
	kShaderVecCameraWorldClipPlanes0,
	kShaderVecCameraWorldClipPlanes1,
	kShaderVecCameraWorldClipPlanes2,
	kShaderVecCameraWorldClipPlanes3,
	kShaderVecCameraWorldClipPlanes4,
	kShaderVecCameraWorldClipPlanes5,
	kShaderVecAmbientSky,
	kShaderVecAmbientEquator,
	kShaderVecAmbientGround,
	kShaderVecColorSpaceDielectricSpec,

	kShaderVecPlanarReflection0,
	kShaderVecPlanarReflection1,
	kShaderVecPlanarReflection2,
	kShaderVecPlanarReflectionTangent0,
	kShaderVecPlanarReflectionTangent1,
	kShaderVecPlanarReflectionTangent2,
	kShaderVecPlanarReflectionBiTangent0,
	kShaderVecPlanarReflectionBiTangent1,
	kShaderVecPlanarReflectionBiTangent2,

	kShaderVecIndirectSpecColor,
	kShaderVecSpecCubeHDR,
	kShaderVecSpecCubeBoxMax,
	kShaderVecSpecCubeBoxMin,
	kShaderVecSpecCubeProbePosition,

	kShaderVecSpecCubeHDR1,
	kShaderVecSpecCubeBoxMax1,
	kShaderVecSpecCubeBoxMin1,
	kShaderVecSpecCubeProbePosition1,

	kShaderVecBillboardNormal,
	kShaderVecBillboardTangent,
	kShaderVecBillboardCameraParams,

	kShaderVecProbeVolumeMin,
	kShaderVecProbeVolumeSizeInv,
	kShaderVecProbeVolumeParams,

	kShaderVecStereoEyeIndex,

	kShaderVecCount
};

enum BuiltinShaderTexEnvParam
{
	kShaderTexEnvWhite = 0,
	kShaderTexEnvBlack,
	kShaderTexEnvRed,
	kShaderTexEnvGray,
	kShaderTexEnvGrey, // TODO: synonims
	kShaderTexEnvGrayscaleRamp,
	kShaderTexEnvGreyscaleRamp, // TODO: synonims
	kShaderTexEnvBump,
	kShaderTexEnvBlackCube,
	kShaderTexEnvLightmap,
	kShaderTexEnvUnityLightmapLight,
	kShaderTexEnvUnityLightmapDir,
	kShaderTexEnvUnityDynamicLightmap,
	kShaderTexEnvUnityDynamicDirectionality,
	kShaderTexEnvUnityDynamicNormal,
	kShaderTexEnvDitherMaskLOD,
	kShaderTexEnvDitherMaskLOD2D,
	kShaderTexEnvRandomRotation,
	kShaderTexEnvNHxRoughness,
	kShaderTexEnvSpecCube,
	kShaderTexEnvSpecCube1,
	kShaderTexEnvPlanarReflectionTex0,
	kShaderTexEnvPlanarReflectionTex1,
	kShaderTexEnvPlanarReflectionTex2,
	kShaderTexEnvProbeVolumeSH,
	kShaderTexEnvCount
};

UInt16 GetBuiltinVectorParamArraySize(BuiltinShaderVectorParam param);
UInt16 GetBuiltinMatrixParamArraySize(BuiltinShaderMatrixParam param);

#define BUILTIN_SHADER_PARAMS_INSTANCE_MATRICES	\
	"glstate_matrix_mvp",						\
	"glstate_matrix_modelview0",				\
	"unity_ObjectToWorld",						\
	"unity_WorldToObject",						\
	"glstate_matrix_transpose_modelview0",		\
	"glstate_matrix_invtrans_modelview0",		\


#define BUILTIN_SHADER_PARAMS_MATRICES			\
	"glstate_matrix_projection",				\
	"unity_MatrixV",							\
	"unity_MatrixInvV",							\
	"unity_MatrixVP",							\
	"unity_WorldToCamera",						\
	"unity_CameraToWorld",						\
	"unity_WorldToShadow0",						\
	"unity_WorldToShadow1",						\
	"unity_WorldToShadow2",						\
	"unity_WorldToShadow3",						\
	"unity_LightmapMatrix",						\
	"unity_Projector",							\
	"unity_ProjectorDistance",					\
	"unity_ProjectorClip",						\
	"unity_GUIClipTextureMatrix",				\
	"unity_WorldToLight",						\
	"unity_CameraProjection",					\
	"unity_CameraInvProjection",				\
	"unity_ProbeVolumeWorldToObject",			\
 

#define BUILTIN_SHADER_PARAMS_VECTORS		\
	"unity_LightColor0", \
	"unity_LightColor1", \
	"unity_LightColor2", \
	"unity_LightColor3", \
	"unity_LightColor4", \
	"unity_LightColor5", \
	"unity_LightColor6", \
	"unity_LightColor7", \
	"unity_LightPosition0", \
	"unity_LightPosition1", \
	"unity_LightPosition2", \
	"unity_LightPosition3", \
	"unity_LightPosition4", \
	"unity_LightPosition5", \
	"unity_LightPosition6", \
	"unity_LightPosition7", \
	"unity_SpotDirection0", \
	"unity_SpotDirection1", \
	"unity_SpotDirection2", \
	"unity_SpotDirection3", \
	"unity_SpotDirection4", \
	"unity_SpotDirection5", \
	"unity_SpotDirection6", \
	"unity_SpotDirection7", \
	"unity_LightAtten0", \
	"unity_LightAtten1", \
	"unity_LightAtten2", \
	"unity_LightAtten3", \
	"unity_LightAtten4", \
	"unity_LightAtten5", \
	"unity_LightAtten6", \
	"unity_LightAtten7", \
	"unity_VertexLightParams",				\
	"glstate_lightmodel_ambient",			\
	"_WorldSpaceLightPos0",					\
	"_LightColor0",							\
	"_WorldSpaceCameraPos",					\
	"unity_WorldTransformParams",			\
	"_ProjectionParams",					\
	"_ScreenParams",						\
	"_ZBufferParams",						\
	"unity_OrthoParams",					\
	"_LightPositionRange",					\
	"unity_Ambient",						\
	"unity_LightmapFade",					\
	"_ShadowOffsets0",						\
	"_ShadowOffsets1",						\
	"_ShadowOffsets2",						\
	"_ShadowOffsets3",						\
	"_LightShadowData",						\
	"unity_LightShadowBias",				\
	"_LightSplitsNear",						\
	"_LightSplitsFar",						\
	"unity_ShadowSplitSpheres0",			\
	"unity_ShadowSplitSpheres1",			\
	"unity_ShadowSplitSpheres2",			\
	"unity_ShadowSplitSpheres3",			\
	"unity_ShadowSplitSqRadii",				\
	"unity_ShadowCascadeScales",			\
	"unity_ShadowFadeCenterAndType",		\
	"unity_LODFade"	,						\
	"unity_LightmapST",						\
	"unity_DynamicLightmapST",				\
	"unity_SHAr",							\
	"unity_SHAg",							\
	"unity_SHAb",							\
	"unity_SHBr",							\
	"unity_SHBg",							\
	"unity_SHBb",							\
	"unity_SHC",							\
	"_Time",								\
	"_SinTime",								\
	"_CosTime",								\
	"_PiTime",								\
	"unity_DeltaTime",						\
	"unity_4LightPosX0",					\
	"unity_4LightPosY0",					\
	"unity_4LightPosZ0",					\
	"unity_4LightAtten0",					\
	"unity_FogStart",						\
	"unity_FogEnd",							\
	"unity_FogDensity",						\
	"unity_FogColor",						\
	"unity_FogParams",						\
	"unity_ColorSpaceGrey",					\
	"unity_ColorSpaceDouble",				\
	"unity_ColorSpaceLuminance",			\
	"unity_NPOTScale",						\
	"unity_CameraWorldClipPlanes0",			\
	"unity_CameraWorldClipPlanes1",			\
	"unity_CameraWorldClipPlanes2",			\
	"unity_CameraWorldClipPlanes3",			\
	"unity_CameraWorldClipPlanes4",			\
	"unity_CameraWorldClipPlanes5",			\
	"unity_AmbientSky",						\
	"unity_AmbientEquator",					\
	"unity_AmbientGround",					\
	"unity_ColorSpaceDielectricSpec",		\
											\
	"_PlanarReflection0", \
	"_PlanarReflection1", \
	"_PlanarReflection2", \
	"_PlanarReflectionTangent0", \
	"_PlanarReflectionTangent1", \
	"_PlanarReflectionTangent2", \
	"_PlanarReflectionBiTangent0", \
	"_PlanarReflectionBiTangent1", \
	"_PlanarReflectionBiTangent2", \
											\
	"unity_IndirectSpecColor",				\
	"unity_SpecCube0_HDR",					\
	"unity_SpecCube0_BoxMax",				\
	"unity_SpecCube0_BoxMin",				\
	"unity_SpecCube0_ProbePosition",		\
											\
	"unity_SpecCube1_HDR",					\
	"unity_SpecCube1_BoxMax",				\
	"unity_SpecCube1_BoxMin",				\
	"unity_SpecCube1_ProbePosition",		\
											\
	"unity_BillboardNormal",				\
	"unity_BillboardTangent",				\
	"unity_BillboardCameraParams",			\
											\
	"unity_ProbeVolumeMin",					\
	"unity_ProbeVolumeSizeInv",				\
	"unity_ProbeVolumeParams",				\
											\
	"unity_StereoEyeIndex",					\



#define BUILTIN_SHADER_PARAMS_TEXENVS	\
	"white",							\
	"black",							\
	"red",								\
	"gray",								\
	"grey",								\
	"grayscaleRamp",					\
	"greyscaleRamp",					\
	"bump",								\
	"blackCube",						\
	"lightmap",							\
	"unity_Lightmap",					\
	"unity_LightmapInd",				\
	"unity_DynamicLightmap",			\
	"unity_DynamicDirectionality",		\
	"unity_DynamicNormal",				\
	"_DitherMaskLOD",					\
	"_DitherMaskLOD2D",					\
	"unity_RandomRotation16",			\
	"unity_NHxRoughness",				\
	"unity_SpecCube0",					\
	"unity_SpecCube1",					\
	"_PlanarReflectionTex0",			\
	"_PlanarReflectionTex1",			\
	"_PlanarReflectionTex2",			\
	"unity_ProbeVolumeSH"				\

// End of file (after empty line after trailing backslash)
