#pragma once

#include <vector>

#include "../TelemetrySourcererDriver/Common.h"

using namespace std;

// Struct:    CALLBACK_ENTRY
// Describes: A kernel-mode callback.
// Members:
// - Type:          The type of callback.
// - Address:       The address of the callback.
// - ModuleName:    The name of the module hosting the callback.
// - ModuleOffset:  The offset of the callback from the module's base address.
// - OriginalQword: The first 8 bytes of the callback.
// - Suppressed:    Whether the callback has been suppressed.
// - Notable:       Whether the callback should be highlighted.
typedef struct _CALLBACK_ENTRY
{
	CALLBACK_TYPE Type                 = CALLBACK_TYPE::Unknown;
	PVOID         Address              = nullptr;
	WCHAR         ModuleName[MAX_PATH] = { 0 };
	SIZE_T        ModuleOffset         = 0;
	ULONG64       OriginalQword        = 0;
	BOOL          Suppressed           = FALSE;
	BOOL          Notable              = FALSE;
} CALLBACK_ENTRY, *PCALLBACK_ENTRY;

DWORD LoadDriver();
DWORD UnloadDriver();
BOOL IsProcessElevated();
HANDLE GetDeviceHandle();
VOID PopulateKmcModules();
PMODULE_INFO GetCallbackModule(PMODULE_INFO Modules, PVOID CallbackAddress);
ULONG64 GetQword(PVOID Address);
BOOL SetQword(PVOID Address, ULONG64 Value);
std::vector<PCALLBACK_ENTRY> GetCallbacks(std::vector<PCALLBACK_ENTRY> OldCallbackEntries);
ULONG64 GetSuppressionValue(CALLBACK_TYPE CallbackType);
BOOL SuppressCallback(PCALLBACK_ENTRY Callback);
BOOL RevertCallback(PCALLBACK_ENTRY Callback);