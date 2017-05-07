#pragma once

namespace gles
{
	void* GetProcAddress(const char* name);

	// Same as above, but used only for core GL functions
	// This is needed to avoid running out of function slots on bad EGL implementations
	void *GetProcAddress_core(const char *name);

}//namespace gles
