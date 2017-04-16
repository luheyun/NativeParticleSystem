#include "PluginPrefix.h"

#pragma warning(disable:4530) // exception handler used

#define UNITY_WIN 1
#include "Runtime/Misc/SystemInfo.h"

// system includes
#include <comutil.h>
#include <wbemidl.h>
#include <Shlobj.h>
#include <WinSock2.h>
#include <Iphlpapi.h>
#include <Psapi.h>
#include <tlhelp32.h>

#include <cstdlib> // for atoi

int systeminfo::GetProcessorCount()
{
	SYSTEM_INFO	sysInfo;
	sysInfo.dwNumberOfProcessors = 1;
	GetSystemInfo( &sysInfo );
	return sysInfo.dwNumberOfProcessors;
}

#define kMaxProcessorCoreCount 64
static DWORD g_ProcessorCoreCount = 0;
static ULONG_PTR g_CoreProcessorMasks[kMaxProcessorCoreCount];

typedef BOOL (WINAPI *LPFN_GLPI)(
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,
	PDWORD);

static void DetectProcessorCores()
{
	if (g_ProcessorCoreCount != 0)
	{
		return;
	}

	// Default core affinity mask
	g_CoreProcessorMasks[0] = 0xffffffff;
	
	LPFN_GLPI getLogicalProcInfo;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
	DWORD returnLength = 0;

	getLogicalProcInfo = (LPFN_GLPI) GetProcAddress(GetModuleHandleW(L"kernel32"),
			"GetLogicalProcessorInformation");

	if (getLogicalProcInfo == NULL)
	{
		// GetLogicalProcessorInformation is not supported
		g_ProcessorCoreCount = 1;
		return;
	}

	if (getLogicalProcInfo(buffer, &returnLength) == FALSE &&
		GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION) malloc(returnLength);
	}

	if (buffer == NULL)
	{
		// Failed to allocate buffer
		g_ProcessorCoreCount = 1;
		return;
	}

	if (getLogicalProcInfo(buffer, &returnLength) == FALSE)
	{
		// Failed to get information
		g_ProcessorCoreCount = 1;
		free(buffer);
		return;
	}

	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION curInfo = buffer;
	DWORD processorCoreCount = 0;
	DWORD byteOffset = 0;
	while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength)
	{
		if (curInfo->Relationship == RelationProcessorCore)
		{
			if (processorCoreCount < kMaxProcessorCoreCount)
			{
				g_CoreProcessorMasks[processorCoreCount] = curInfo->ProcessorMask;
				processorCoreCount++;
			}
		}
		byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		curInfo++;
	}
	free(buffer);

	if (processorCoreCount == 0)
	{
		// Don't believe this!
		processorCoreCount = 1;
	}
	g_ProcessorCoreCount = processorCoreCount;
}

int systeminfo::GetPhysicalProcessorCount()
{
	DetectProcessorCores();
	return g_ProcessorCoreCount;
}
