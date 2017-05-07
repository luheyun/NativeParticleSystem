#pragma once

#include "ApiTypeGLES.h"

static const GLenum GL_TRUE									= 1;
static const GLenum GL_FALSE								= 0;

static const GLenum GL_MAP_READ_BIT							= 0x0001;
static const GLenum GL_MAP_WRITE_BIT						= 0x0002;
static const GLenum GL_MAP_INVALIDATE_RANGE_BIT				= 0x0004;
static const GLenum GL_MAP_INVALIDATE_BUFFER_BIT			= 0x0008;
static const GLenum GL_MAP_FLUSH_EXPLICIT_BIT				= 0x0010;
static const GLenum GL_MAP_UNSYNCHRONIZED_BIT				= 0x0020;

static const GLenum GL_COLOR								= 0x1800;
static const GLenum GL_DEPTH								= 0x1801;
static const GLenum GL_STENCIL								= 0x1802;

static const GLenum GL_DEPTH_STENCIL_ATTACHMENT	= 0x821A;

static const GLenum GL_CULL_FACE_MODE			= 0x0B45;
static const GLenum GL_CULL_FACE				= 0x0B44;
static const GLenum GL_FRONT					= 0x0404;
static const GLenum GL_BACK						= 0x0405;
static const GLenum GL_BACK_LEFT				= 0x0402;
static const GLenum GL_BACK_RIGHT				= 0x0403;
static const GLenum GL_STEREO					= 0x0C33;
static const GLenum GL_CW						= 0x0900;
static const GLenum GL_CCW						= 0x0901;

static const GLenum GL_WRITE_ONLY				= 0x88B9;

static const GLenum GL_ONE						= 1;
static const GLenum GL_ZERO						= 0;

static const GLenum GL_DEPTH_BUFFER_BIT			= 0x00000100;
static const GLenum GL_STENCIL_BUFFER_BIT		= 0x00000400;
static const GLenum GL_COLOR_BUFFER_BIT			= 0x00004000;
static const GLenum GL_COVERAGE_BUFFER_BIT_NV	= 0x00008000;

static const GLenum GL_INTERLEAVED_ATTRIBS		= 0x8C8C;

static const GLenum GL_POINTS					= 0x0000;
static const GLenum GL_LINES					= 0x0001;
static const GLenum GL_LINE_LOOP				= 0x0002;
static const GLenum GL_LINE_STRIP				= 0x0003;
static const GLenum GL_TRIANGLES				= 0x0004;
static const GLenum GL_TRIANGLE_STRIP			= 0x0005;
static const GLenum GL_TRIANGLE_FAN				= 0x0006;

static const GLenum GL_TEXTURE_IMMUTABLE_FORMAT	= 0x912F;
static const GLenum GL_TEXTURE_MAX_LEVEL		= 0x813D;

static const GLenum GL_TEXTURE_2D				= 0x0DE1;
static const GLenum GL_TEXTURE_3D				= 0x806F;
static const GLenum GL_TEXTURE_CUBE_MAP			= 0x8513;
static const GLenum GL_TEXTURE_2D_ARRAY			= 0x8C1A;
static const GLenum GL_TEXTURE_2D_MULTISAMPLE	= 0x9100;

static const GLenum GL_RENDERBUFFER				= 0x8D41;

static const GLenum GL_COLOR_ATTACHMENT0		= 0x8CE0;
static const GLenum GL_DEPTH_ATTACHMENT			= 0x8D00;
static const GLenum GL_STENCIL_ATTACHMENT		= 0x8D20;
static const GLenum GL_COVERAGE_ATTACHMENT_NV	= 0x8ED2;
static const GLenum GL_COVERAGE_COMPONENT4_NV	= 0x8ED1;

static const GLenum GL_FRAMEBUFFER				= 0x8D40;
static const GLenum GL_READ_FRAMEBUFFER			= 0x8CA8;
static const GLenum GL_DRAW_FRAMEBUFFER			= 0x8CA9;
static const GLenum GL_SAMPLE_BUFFERS			= 0x80A8;
static const GLenum GL_STENCIL_INDEX8			= 0x8D48;

static const GLenum GL_FRAMEBUFFER_COMPLETE								= 0x8CD5;
static const GLenum GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT				= 0x8CD6;
static const GLenum GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT		= 0x8CD7;
static const GLenum GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS				= 0x8CD9;
static const GLenum GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT				= 0x8CDA; // GL_EXT_framebuffer_object
static const GLenum GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER				= 0x8CDB;
static const GLenum GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER				= 0x8CDC;
static const GLenum GL_FRAMEBUFFER_UNSUPPORTED							= 0x8CDD;
static const GLenum GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE				= 0x8D56;

static const GLenum GL_NEVER					= 0x0200;
static const GLenum GL_LESS						= 0x0201;
static const GLenum GL_EQUAL					= 0x0202;
static const GLenum GL_LEQUAL					= 0x0203;
static const GLenum GL_GREATER					= 0x0204;
static const GLenum GL_NOTEQUAL					= 0x0205;
static const GLenum GL_GEQUAL					= 0x0206;
static const GLenum GL_ALWAYS					= 0x0207;

static const GLenum GL_TEXTURE_COMPARE_MODE		= 0x884C;
static const GLenum GL_TEXTURE_COMPARE_FUNC		= 0x884D;
static const GLenum GL_COMPARE_REF_TO_TEXTURE	= 0x884E;

static const GLenum GL_QUERY_RESULT_AVAILABLE	= 0x8867;
static const GLenum GL_QUERY_RESULT				= 0x8866;
static const GLenum GL_GPU_DISJOINT_EXT			= 0x8FBB;
static const GLenum GL_TIMESTAMP				= 0x8E28;

static const GLenum GL_LINK_STATUS				= 0x8B82;
static const GLenum GL_TESS_CONTROL_OUTPUT_VERTICES	= 0x8E75;
static const GLenum GL_GEOMETRY_LINKED_INPUT_TYPE	= 0x8917;

static const GLenum GL_PROGRAM_BINARY_RETRIEVABLE_HINT	= 0x8257;
static const GLenum GL_PROGRAM_BINARY_LENGTH	= 0x8741;
static const GLenum GL_PROGRAM_BINARY_FORMATS	= 0x87FF;
static const GLenum GL_NUM_PROGRAM_BINARY_FORMATS = 0x87FE;

static const GLenum GL_STREAM_DRAW				= 0x88E0;
static const GLenum GL_STATIC_DRAW				= 0x88E4;
static const GLenum GL_DYNAMIC_DRAW				= 0x88E8;
static const GLenum GL_STATIC_COPY				= 0x88E6;

static const GLenum GL_BYTE								= 0x1400;
static const GLenum GL_UNSIGNED_BYTE					= 0x1401;
static const GLenum GL_SHORT							= 0x1402;
static const GLenum GL_UNSIGNED_SHORT					= 0x1403;
static const GLenum GL_INT								= 0x1404;
static const GLenum GL_UNSIGNED_INT						= 0x1405;
static const GLenum GL_FLOAT							= 0x1406;
static const GLenum GL_FIXED							= 0x140C;

static const GLenum GL_FLOAT_VEC2 = 0x8B50;
static const GLenum GL_FLOAT_VEC3 = 0x8B51;
static const GLenum GL_FLOAT_VEC4 = 0x8B52;
static const GLenum GL_INT_VEC2 = 0x8B53;
static const GLenum GL_INT_VEC3 = 0x8B54;
static const GLenum GL_INT_VEC4 = 0x8B55;
static const GLenum GL_UNSIGNED_INT_VEC2 = 0x8DC6;
static const GLenum GL_UNSIGNED_INT_VEC3 = 0x8DC7;
static const GLenum GL_UNSIGNED_INT_VEC4 = 0x8DC8;

static const GLenum GL_HIGH_FLOAT = 0x8DF2;

static const GLenum GL_RGBA32I							= 0x8D82;
static const GLenum GL_RG32I							= 0x823B;
static const GLenum GL_R32I								= 0x8235;

static const GLenum GL_RGBA8							= 0x8058;
static const GLenum GL_SRGB8_ALPHA8						= 0x8C43;
static const GLenum GL_DEPTH_COMPONENT16				= 0x81A5;
static const GLenum GL_DEPTH_COMPONENT24				= 0x81A6;
static const GLenum GL_DEPTH24_STENCIL8					= 0x88F0;

static const GLenum GL_DEPTH_COMPONENT					= 0x1902;
static const GLenum GL_ALPHA							= 0x1906;
static const GLenum GL_RGB								= 0x1907;
static const GLenum GL_RGBA								= 0x1908;
static const GLenum GL_LUMINANCE						= 0x1909;

static const GLenum GL_UNSIGNED_SHORT_4_4_4_4			= 0x8033;
static const GLenum GL_UNSIGNED_SHORT_5_5_5_1			= 0x8034;
static const GLenum GL_UNSIGNED_SHORT_5_6_5				= 0x8363;

static const GLenum GL_TEXTURE_WRAP_S						= 0x2802;
static const GLenum GL_TEXTURE_WRAP_T						= 0x2803;
static const GLenum GL_TEXTURE_WRAP_R						= 0x8072;
static const GLenum GL_TEXTURE_MAG_FILTER					= 0x2800;
static const GLenum GL_TEXTURE_MIN_FILTER					= 0x2801;

static const GLenum GL_TEXTURE_CUBE_MAP_POSITIVE_X			= 0x8515;

static const GLenum GL_MAX_VERTEX_ATTRIBS					= 0x8869;

static const GLenum GL_RED									= 0x1903;
static const GLenum GL_GREEN								= 0x1904;
static const GLenum GL_BLUE									= 0x1905;
static const GLenum GL_NONE									= 0;

static const GLenum GL_RG										= 0x8227;
static const GLenum GL_BGRA										= 0x80E1;
static const GLenum GL_DEPTH_STENCIL							= 0x84F9;
static const GLenum GL_RED_INTEGER								= 0x8D94;
static const GLenum GL_RG_INTEGER								= 0x8228;
static const GLenum GL_RGB_INTEGER								= 0x8D98;
static const GLenum GL_RGBA_INTEGER								= 0x8D99;

static const GLenum GL_HALF_FLOAT_OES							= 0x8D61;
static const GLenum GL_HALF_FLOAT								= 0x140B;
static const GLenum GL_COMPRESSED_RGBA8_ETC2_EAC				= 0x9278;

static const GLenum GL_SYNC_GPU_COMMANDS_COMPLETE				= 0x9117;
static const GLenum GL_ALREADY_SIGNALED							= 0x911A;
static const GLenum GL_TIMEOUT_EXPIRED							= 0x911B;
static const GLenum GL_CONDITION_SATISFIED						= 0x911C;
static const GLenum GL_WAIT_FAILED								= 0x911D;
static const GLuint64 GL_TIMEOUT_IGNORED						= 0xFFFFFFFFFFFFFFFF;

static const GLenum GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT			= 0x00000001;
static const GLenum GL_ELEMENT_ARRAY_BARRIER_BIT				= 0x00000002;
static const GLenum GL_UNIFORM_BARRIER_BIT						= 0x00000004;
static const GLenum GL_TEXTURE_FETCH_BARRIER_BIT				= 0x00000008;
static const GLenum GL_SHADER_IMAGE_ACCESS_BARRIER_BIT			= 0x00000020;
static const GLenum GL_COMMAND_BARRIER_BIT						= 0x00000040;
static const GLenum GL_PIXEL_BUFFER_BARRIER_BIT					= 0x00000080;
static const GLenum GL_TEXTURE_UPDATE_BARRIER_BIT				= 0x00000100;
static const GLenum GL_BUFFER_UPDATE_BARRIER_BIT				= 0x00000200;
static const GLenum GL_FRAMEBUFFER_BARRIER_BIT					= 0x00000400;
static const GLenum GL_TRANSFORM_FEEDBACK_BARRIER_BIT			= 0x00000800;
static const GLenum GL_ATOMIC_COUNTER_BARRIER_BIT				= 0x00001000;
static const GLenum GL_SHADER_STORAGE_BARRIER_BIT				= 0x00002000;
static const GLenum GL_ALL_BARRIER_BITS							= 0xFFFFFFFF;

static const GLenum GL_FUNC_ADD					= 0x8006;
static const GLenum GL_BLEND_EQUATION			= 0x8009;
static const GLenum GL_BLEND_EQUATION_RGB		= 0x8009;
static const GLenum GL_BLEND_EQUATION_ALPHA		= 0x883D;
static const GLenum GL_FUNC_SUBTRACT			= 0x800A;
static const GLenum GL_FUNC_REVERSE_SUBTRACT	= 0x800B;

static const GLenum GL_MULTIPLY_KHR				= 0x9294;
static const GLenum GL_SCREEN_KHR				= 0x9295;
static const GLenum GL_OVERLAY_KHR				= 0x9296;
static const GLenum GL_DARKEN_KHR				= 0x9297;
static const GLenum GL_LIGHTEN_KHR				= 0x9298;
static const GLenum GL_COLORDODGE_KHR			= 0x9299;
static const GLenum GL_COLORBURN_KHR			= 0x929A;
static const GLenum GL_HARDLIGHT_KHR			= 0x929B;
static const GLenum GL_SOFTLIGHT_KHR			= 0x929C;
static const GLenum GL_DIFFERENCE_KHR			= 0x929E;
static const GLenum GL_EXCLUSION_KHR			= 0x92A0;
static const GLenum GL_HSL_HUE_KHR				= 0x92AD;
static const GLenum GL_HSL_SATURATION_KHR		= 0x92AE;
static const GLenum GL_HSL_COLOR_KHR			= 0x92AF;
static const GLenum GL_HSL_LUMINOSITY_KHR		= 0x92B0;

static const GLenum GL_MIN							= 0x8007;
static const GLenum GL_MAX							= 0x8008;

static const GLenum GL_ELEMENT_ARRAY_BUFFER			= 0x8893;
static const GLenum GL_ARRAY_BUFFER					= 0x8892;
static const GLenum GL_UNIFORM_BUFFER				= 0x8A11;
static const GLenum GL_COPY_WRITE_BUFFER			= 0x8F37;
static const GLenum GL_COPY_READ_BUFFER				= 0x8F36;
static const GLenum GL_TRANSFORM_FEEDBACK_BUFFER	= 0x8C8E;
static const GLenum GL_SHADER_STORAGE_BUFFER		= 0x90D2;
static const GLenum GL_PIXEL_PACK_BUFFER			= 0x88EB;
static const GLenum GL_PIXEL_UNPACK_BUFFER			= 0x88EC;
static const GLenum GL_DISPATCH_INDIRECT_BUFFER		= 0x90EE;
static const GLenum GL_DRAW_INDIRECT_BUFFER			= 0x8F3F;
static const GLenum GL_PARAMETER_BUFFER_ARB			= 0x80EE;
static const GLenum GL_QUERY_BUFFER					= 0x9192;
static const GLenum GL_ATOMIC_COUNTER_BUFFER		= 0x92C0;
static const GLenum GL_COMPUTE_WORK_GROUP_SIZE		= 0x8267;

static const GLenum GL_TEXTURE_BINDING_CUBE_MAP			= 0x8514;
static const GLenum GL_BLEND							= 0x0BE2;
static const GLenum GL_DITHER							= 0x0BD0;
static const GLenum GL_STENCIL_TEST						= 0x0B90;
static const GLenum GL_DEPTH_TEST						= 0x0B71;
static const GLenum GL_SCISSOR_TEST						= 0x0C11;
static const GLenum GL_POLYGON_OFFSET_FILL				= 0x8037;
static const GLenum GL_SAMPLE_ALPHA_TO_COVERAGE			= 0x809E;
static const GLenum GL_SAMPLE_COVERAGE					= 0x80A0;
static const GLenum GL_COLOR_LOGIC_OP					= 0x0BF2; // OpenGL 1.1
static const GLenum GL_DEPTH_CLAMP						= 0x864F; // OpenGL 3.2 / ARB_depth_clamp
static const GLenum GL_FRAMEBUFFER_SRGB					= 0x8DB9; // OpenGL 3.0 / ARB_framebuffer_object / EXT_framebuffer_srgb
static const GLenum GL_MULTISAMPLE						= 0x809D; // OpenGL 1.3
static const GLenum GL_SAMPLE_ALPHA_TO_ONE				= 0x809F; // OpenGL 1.3
static const GLenum GL_SAMPLE_MASK						= 0x8E51; // OpenGL 3.2
static const GLenum GL_PROGRAM_POINT_SIZE				= 0x8642; // OpenGL 3.2
static const GLenum GL_SAMPLE_SHADING					= 0x8C36; // OpenGL 4.0
static const GLenum GL_POLYGON_OFFSET_POINT				= 0x2A01; // OpenGL 1.1
static const GLenum GL_POLYGON_OFFSET_LINE				= 0x2A02; // OpenGL 1.1
static const GLenum GL_POLYGON_SMOOTH					= 0x0B41; // OpenGL 1.1
static const GLenum GL_PRIMITIVE_RESTART				= 0x8F9D; // OpenGL 3.1
static const GLenum GL_PRIMITIVE_RESTART_FIXED_INDEX	= 0x8D69; // OpenGL 4.3 / OpenGL ES 3.0
static const GLenum GL_ALPHA_TEST						= 0x0BC0; // OpenGL compatibility profile and GL_QCOM_alpha_test
static const GLenum GL_DEBUG_OUTPUT						= 0x92E0; // KHR_debug
static const GLenum GL_DEBUG_OUTPUT_SYNCHRONOUS			= 0x8242; // KHR_debug
static const GLenum GL_LINE_SMOOTH						= 0x0B20; // OpenGL line smoothing, only for OpenGL core and compatibility profile
static const GLenum GL_RASTERIZER_DISCARD				= 0x8C89; // OpenGL 3.2 / OpenGL ES 3.0
static const GLenum GL_TEXTURE_CUBE_MAP_SEAMLESS		= 0x884F; // OpenGL 3.2 / GL_ARB_seamless_cube_map

static const GLenum GL_FRAGMENT_SHADER				= 0x8B30;
static const GLenum GL_VERTEX_SHADER				= 0x8B31;
static const GLenum GL_GEOMETRY_SHADER				= 0x8DD9;
static const GLenum GL_TESS_EVALUATION_SHADER		= 0x8E87;
static const GLenum GL_TESS_CONTROL_SHADER			= 0x8E88;
static const GLenum GL_COMPUTE_SHADER				= 0x91B9;

static const GLenum GL_REPEAT						= 0x2901;
static const GLenum GL_CLAMP_TO_EDGE				= 0x812F;
static const GLenum GL_MIRRORED_REPEAT				= 0x8370;
static const GLenum GL_NEAREST						= 0x2600;
static const GLenum GL_LINEAR						= 0x2601;
static const GLenum GL_NEAREST_MIPMAP_NEAREST		= 0x2700;
static const GLenum GL_LINEAR_MIPMAP_NEAREST		= 0x2701;
static const GLenum GL_NEAREST_MIPMAP_LINEAR		= 0x2702;
static const GLenum GL_LINEAR_MIPMAP_LINEAR			= 0x2703;

// KHR_debug object type identifiers
static const GLenum GL_BUFFER						= 0x82E0;
static const GLenum GL_SHADER						= 0x82E1;
static const GLenum GL_PROGRAM						= 0x82E2;
static const GLenum GL_VERTEX_ARRAY					= 0x8074;
static const GLenum GL_QUERY						= 0x82E3;
static const GLenum GL_PROGRAM_PIPELINE				= 0x82E4;
static const GLenum GL_SAMPLER						= 0x82E6;
static const GLenum GL_TRANSFORM_FEEDBACK			= 0x8E22;
static const GLenum GL_TEXTURE						= 0x1702;

static const GLenum GL_SRC_COLOR				= 0x0300;
static const GLenum GL_ONE_MINUS_SRC_COLOR		= 0x0301;
static const GLenum GL_SRC_ALPHA				= 0x0302;
static const GLenum GL_ONE_MINUS_SRC_ALPHA		= 0x0303;
static const GLenum GL_DST_ALPHA				= 0x0304;
static const GLenum GL_ONE_MINUS_DST_ALPHA		= 0x0305;
static const GLenum GL_DST_COLOR				= 0x0306;
static const GLenum GL_ONE_MINUS_DST_COLOR		= 0x0307;
static const GLenum GL_SRC_ALPHA_SATURATE		= 0x0308;

static const GLenum GL_KEEP			= 0x1E00;
static const GLenum GL_REPLACE		= 0x1E01;
static const GLenum GL_INCR			= 0x1E02;
static const GLenum GL_DECR			= 0x1E03;
static const GLenum GL_INVERT		= 0x150A;
static const GLenum GL_INCR_WRAP	= 0x8507;
static const GLenum GL_DECR_WRAP	= 0x8508;
