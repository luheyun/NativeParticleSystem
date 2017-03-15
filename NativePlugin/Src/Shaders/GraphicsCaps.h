#pragma once

struct GraphicsCaps
{
	GraphicsCaps();

	bool usesOpenGLTextureCoords; // OpenGL: texture V coordinate is 0 at the bottom; 1 at the top. Otherwise: texture V coordinate is 0 at the top; 1 at the bottom.
};

GraphicsCaps& GetGraphicsCaps();