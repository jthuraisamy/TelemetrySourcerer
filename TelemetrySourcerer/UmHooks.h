#pragma once

#include <Windows.h>

#include <vector>

using namespace std;

// Struct:    HOOKED_FUNCTION
// Describes: A hooked function.
// Members:
// - ModuleHandle: Handle to the function's module.
// - Ordinal:      The ordinal number of the function.
// - Address:      The address of the function.
// - Name:         Name of the function, if it exists.
typedef struct _HOOKED_FUNCTION {
	HMODULE ModuleHandle   = 0;
	DWORD   Ordinal        = 0;
	LPVOID  Address        = nullptr;
	WCHAR   Name[MAX_PATH] = { 0 };
	UCHAR   FreshBytes[16] = { 0 };
} HOOKED_FUNCTION, *PHOOKED_FUNCTION;

// Struct:    LOADED_MODULE
// Describes: A loaded module.
// Members:
// - Handle:          Handle to the function's module.
// - Path:            File path of module.
// - HookedFunctions: Array of hooked functions.
typedef struct _LOADED_MODULE {
	HMODULE Handle = 0;
	WCHAR Path[MAX_PATH] = { 0 };
	std::vector<PHOOKED_FUNCTION> HookedFunctions;
} LOADED_MODULE, *PLOADED_MODULE;

std::vector<PLOADED_MODULE> CheckAllModulesForHooks();
std::vector<PLOADED_MODULE> GetModules();
VOID CheckModuleForHooks(PLOADED_MODULE Module);
BOOL RestoreHookedFunction(PHOOKED_FUNCTION HookedFunction);