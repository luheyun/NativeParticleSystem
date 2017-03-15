#pragma once

#include "D3D9Includes.h"
#include "UnityPluginInterface.h"

bool InitializeD3D(D3DDEVTYPE devtype);
IDirect3DDevice9* GetD3DDevice();
void SetD3DDevice(IDirect3DDevice9* device, GfxDeviceEventType eventType);

extern D3DDEVTYPE g_D3DDevType;
extern DWORD g_D3DAdapter;