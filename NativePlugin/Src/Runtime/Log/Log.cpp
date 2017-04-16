#include "PluginPrefix.h"
#include "Log.h"

extern "C"
{
	void(_stdcall* _debugLog)(char*) = nullptr;
}

void SetDebugLog(void(_stdcall*debugLog)(char*))
{
	_debugLog = debugLog;
}

void DebugLog(char* str)
{
#if _DEBUG
	if (_debugLog) _debugLog(str);
#endif
}