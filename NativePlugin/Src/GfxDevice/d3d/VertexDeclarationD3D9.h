#pragma once

#include "GfxDevice/VertexDeclaration.h"
#include "D3D9Includes.h"


class VertexDeclarationD3D9 : public VertexDeclaration
{
public:
	VertexDeclarationD3D9(IDirect3DVertexDeclaration9* decl) : m_D3DDecl(decl) {}
	~VertexDeclarationD3D9();

	IDirect3DVertexDeclaration9* GetD3DDecl() { return m_D3DDecl; }

private:
	IDirect3DVertexDeclaration9* m_D3DDecl;
};
