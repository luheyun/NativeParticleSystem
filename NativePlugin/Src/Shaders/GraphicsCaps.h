#pragma once

#ifdef GFX_SUPPORTS_D3D9
#include "GfxDevice/d3d/D3D9Includes.h"

struct GraphicsCapsD3D9
{
	D3DCAPS9 d3dcaps;
};
#endif

struct GraphicsCaps
{
	GraphicsCaps();

	bool usesOpenGLTextureCoords; // OpenGL: texture V coordinate is 0 at the bottom; 1 at the top. Otherwise: texture V coordinate is 0 at the top; 1 at the bottom.

	UInt32	requiredShaderChannels; // Shader channels that need to be in the vertex buffer if used by the shader

#ifdef GFX_SUPPORTS_D3D9
	GraphicsCapsD3D9 d3d;
#endif
};

GraphicsCaps& GetGraphicsCaps();