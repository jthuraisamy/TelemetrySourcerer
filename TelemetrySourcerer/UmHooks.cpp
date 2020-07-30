#include <Windows.h>
#include <Psapi.h>
#include <Shlwapi.h>

#include <string>
#include <vector>

#include "UmHooks.h"

using namespace std;

// Function:    CheckAllModulesForHooks
// Description: Returns a list of all loaded modules with any hooks.
// Called from: UmhLoadResults
std::vector<PLOADED_MODULE> CheckAllModulesForHooks()
{
	// Get all modules.
	std::vector<PLOADED_MODULE> Modules = GetModules();

	// Check each module for hooks.
	for (PLOADED_MODULE Module : Modules)
		CheckModuleForHooks(Module);

	return Modules;
}

// Function:    GetModules
// Description: Returns a list of all loaded modules for this process.
// Called from: CheckAllModulesForHooks
std::vector<PLOADED_MODULE> GetModules()
{
	std::vector<PLOADED_MODULE> Modules;

	DWORD RequredBytes = 0;
	DWORD ModuleCount = 0;
	HMODULE* ModuleHandles = (HMODULE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HMODULE) * 1024);
	DWORD ModuleHandlesSize = sizeof(HMODULE) * 1024;
	BOOL Status = EnumProcessModulesEx(GetCurrentProcess(), ModuleHandles, ModuleHandlesSize, &RequredBytes, LIST_MODULES_DEFAULT);

	// Fail silently if the call was not successful or enough memory wasn't allocated.
	if ((Status == FALSE) || (RequredBytes > ModuleHandlesSize))
	{
		return Modules;
	}

	ModuleCount = RequredBytes / sizeof(HMODULE);
	for (int i = 0; i < ModuleCount; i++)
	{
		PLOADED_MODULE Module = new LOADED_MODULE;

		Module->Handle = ModuleHandles[i];
		GetModuleFileNameExW(GetCurrentProcess(), Module->Handle, Module->Path, MAX_PATH);

		Modules.push_back(Module);
	}

	HeapFree(GetProcessHeap(), NULL, ModuleHandles);
	return Modules;
}

// Function:    CheckModuleForHooks
// Description: Checks a given module for hooked functions by comparing against a fresh copy.
// Called from: CheckAllModulesForHooks
// Remarks:     Adapted from https://github.com/NtRaiseHardError/Antimalware-Research.
VOID CheckModuleForHooks(PLOADED_MODULE Module)
{
	// Load a fresh copy in memory.
	HANDLE FmFileHandle = CreateFileW(Module->Path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	HANDLE FmMappingHandle = CreateFileMapping(FmFileHandle, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL);
	HMODULE FmHandle = (HMODULE)MapViewOfFile(FmMappingHandle, FILE_MAP_READ, 0, 0, 0);
	HMODULE LmHandle = Module->Handle;

	// Parse the original module's PE headers.
	PIMAGE_DOS_HEADER LmDosHeader = (PIMAGE_DOS_HEADER)LmHandle;
	PIMAGE_NT_HEADERS LmNtHeaders = (PIMAGE_NT_HEADERS)((DWORD_PTR)LmHandle + LmDosHeader->e_lfanew);

	// Parse the fresh module's PE headers.
	PIMAGE_DOS_HEADER FmDosHeader = (PIMAGE_DOS_HEADER)FmHandle;
	PIMAGE_NT_HEADERS FmNtHeaders = (PIMAGE_NT_HEADERS)((DWORD_PTR)FmHandle + FmDosHeader->e_lfanew);

	// Check if the export table exists.
	if (LmNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress != 0) {
		// Get the export table for the loaded module.
		PIMAGE_EXPORT_DIRECTORY LmExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((DWORD_PTR)LmHandle + LmNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		LPDWORD LmFunctionTable = (LPDWORD)((DWORD_PTR)LmHandle + LmExportDirectory->AddressOfFunctions);
		LPDWORD LmNameTable = (LPDWORD)((DWORD_PTR)LmHandle + LmExportDirectory->AddressOfNames);
		LPWORD LmOrdinalTable = (LPWORD)((DWORD_PTR)LmHandle + LmExportDirectory->AddressOfNameOrdinals);

		// Get the export table for the fresh module.
		PIMAGE_EXPORT_DIRECTORY FmExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((DWORD_PTR)FmHandle + FmNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		LPDWORD FmFunctionTable = (LPDWORD)((DWORD_PTR)FmHandle + FmExportDirectory->AddressOfFunctions);
		LPDWORD FmNameTable = (LPDWORD)((DWORD_PTR)FmHandle + FmExportDirectory->AddressOfNames);
		LPWORD FmOrdinalTable = (LPWORD)((DWORD_PTR)FmHandle + FmExportDirectory->AddressOfNameOrdinals);

		// Walk the export table.
		for (DWORD i = 0; i < LmExportDirectory->NumberOfNames; i++) {
			// Get the address of the export (loaded + fresh).
			FARPROC LmFunction = (FARPROC)((DWORD_PTR)LmHandle + LmFunctionTable[LmOrdinalTable[i]]);
			FARPROC FmFunction = (FARPROC)((DWORD_PTR)FmHandle + FmFunctionTable[FmOrdinalTable[i]]);

			// Check if the address of the loaded export is executable. Skip if not.
			MEMORY_BASIC_INFORMATION mbi = { 0 };
			VirtualQuery(LmFunction, &mbi, sizeof(mbi));
			if ((mbi.Protect & PAGE_EXECUTE_READ) == 0)
				continue;

			// Check if the function is hooked by comparing memory between the loaded module and the fresh copy.
			if (memcmp(LmFunction, FmFunction, 16))
			{
				PHOOKED_FUNCTION HookedFunction = new HOOKED_FUNCTION;

				HookedFunction->ModuleHandle = Module->Handle;
				HookedFunction->Ordinal = LmOrdinalTable[i];
				HookedFunction->Address = LmFunction;
				MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, (LPCCH)((DWORD_PTR)LmHandle + LmNameTable[i]), -1, (LPWSTR)HookedFunction->Name, MAX_PATH - 1);
				CopyMemory(HookedFunction->FreshBytes, FmFunction, 16);

				Module->HookedFunctions.push_back(HookedFunction);
			}
		}
	}

	// Unmap fresh module.
	UnmapViewOfFile(FmHandle);
	CloseHandle(FmMappingHandle);
	CloseHandle(FmFileHandle);
}

// Function:    RestoreHookedFunction
// Description: Unhooks a function using bytes collected from a fresh copy.
// Called from: UmhRestoreFunction
// Remarks:     Adapted from https://github.com/NtRaiseHardError/Antimalware-Research.
BOOL RestoreHookedFunction(PHOOKED_FUNCTION HookedFunction)
{
	DWORD OldProtection = 0;
	
	if (!VirtualProtect(HookedFunction->Address, 16, PAGE_EXECUTE_READWRITE, &OldProtection))
		return FALSE;
	
	CopyMemory(HookedFunction->Address, HookedFunction->FreshBytes, 16);
	
	if (!VirtualProtect(HookedFunction->Address, 16, OldProtection, &OldProtection))
		return FALSE;

	return TRUE;
}