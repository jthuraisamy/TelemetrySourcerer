#include <Windows.h>
#include <Shlwapi.h>
#include <strsafe.h>

#include <set>
#include <string>

#include "KmCallbacks.h"
#include "../TelemetrySourcererDriver/Common.h"

#pragma comment(lib, "Shlwapi.lib")

using namespace std;

std::set<wstring> CommonModules;

// Function:    LoadDriver
// Description: Checks if the driver is loaded, and loads it if not.
// Called from: KmcLoadResults before requesting results.
DWORD LoadDriver()
{
	// Driver is already loaded if can create a handle.
	HANDLE DeviceHandle = GetDeviceHandle();
	if (DeviceHandle != (HANDLE)-1)
	{
		CloseHandle(DeviceHandle);
		return ERROR_SUCCESS;
	}

	// Cannot load the driver if the process is not elevated.
	if (!IsProcessElevated())
		return ERROR_PRIVILEGE_NOT_HELD;

	// Check if the driver is in the same directory.
	WCHAR ExecutableDirectory[MAX_PATH] = { 0 };
	GetModuleFileNameW(GetModuleHandle(NULL), (LPWSTR)&ExecutableDirectory, MAX_PATH - 1);
	PathRemoveFileSpecW((LPWSTR)&ExecutableDirectory);
	WCHAR DriverPath[MAX_PATH] = { 0 };
	StringCchPrintfW(DriverPath, MAX_PATH, LR"(\??\%ls\TelemetrySourcererDriver.sys)", ExecutableDirectory);
	if (!PathFileExistsW(DriverPath))
		return ERROR_FILE_NOT_FOUND;

	// Check if the service exists.
	SC_HANDLE ScmHandle = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
	SC_HANDLE SvcHandle = OpenServiceW(ScmHandle, L"TelemetrySourcererDriver", SERVICE_ALL_ACCESS);

	// If it does not exist, create one.
	if (!SvcHandle)
	{
		if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
			SvcHandle = CreateServiceW(
				ScmHandle,
				L"TelemetrySourcererDriver",
				L"TelemetrySourcererDriver",
				SERVICE_ALL_ACCESS,
				SERVICE_KERNEL_DRIVER,
				SERVICE_DEMAND_START,
				SERVICE_ERROR_NORMAL,
				DriverPath,
				NULL, NULL, NULL, NULL, NULL);
	}
	// If it does exist, set the correct path of the driver.
	else
	{
		ChangeServiceConfigW(
			SvcHandle,
			SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE,
			DriverPath,
			NULL, NULL, NULL, NULL, NULL, NULL);
	}

	// Start the service.
	BOOL ServiceStarted = StartServiceW(SvcHandle, NULL, nullptr);
	
	// Release resources.
	CloseServiceHandle(SvcHandle);
	CloseHandle(DeviceHandle);

	if (ServiceStarted)
		return ERROR_SUCCESS;
	else
		return GetLastError();
}

// Function:    UnloadDriver
// Description: Unloads the driver and deletes the service.
// Called from: MainWndProc when the window is closed.
DWORD UnloadDriver()
{
	// Cannot unload the driver if the process is not elevated.
	if (!IsProcessElevated())
		return ERROR_PRIVILEGE_NOT_HELD;

	// Check if the service exists.
	SC_HANDLE ScmHandle = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
	SC_HANDLE SvcHandle = OpenServiceW(ScmHandle, L"TelemetrySourcererDriver", SERVICE_ALL_ACCESS);

	// Unload, if so.
	if (SvcHandle)
	{
		SERVICE_STATUS ServiceStatus = { 0 };
		if (!ControlService(SvcHandle, SERVICE_CONTROL_STOP, &ServiceStatus))
			return GetLastError();

		if (!DeleteService(SvcHandle))
			return GetLastError();
	}
	else
	{
		return GetLastError();
	}

	return ERROR_SUCCESS;
}

// Function:    IsProcessElevated
// Description: Checks if process is running elevated or is running as SYSTEM.
// Called from: LoadDriver to check eligibility.
BOOL IsProcessElevated()
{
	HANDLE TokenHandle;
	DWORD ReturnLength;
	TOKEN_ELEVATION_TYPE ElevationType;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &TokenHandle))
	{
		if (GetTokenInformation(TokenHandle, TokenElevationType, &ElevationType, sizeof(TOKEN_ELEVATION_TYPE), &ReturnLength))
		{
			CloseHandle(TokenHandle);

			if (ElevationType == TokenElevationTypeFull)
			{
				return TRUE;
			}
			else
			{
				BOOL IsSystem = FALSE;
				PSID SystemSid = nullptr;
				SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
				if (AllocateAndInitializeSid(&NtAuthority, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &SystemSid))
				{
					if (CheckTokenMembership(NULL, SystemSid, &IsSystem))
					{
						FreeSid(SystemSid);

						if (IsSystem)
						{
							return TRUE;
						}
					}
				}
			}
		}
	}
	
	return FALSE;
}

// Function:    GetDeviceHandle
// Description: Returns a handle to the TelemetrySourcerDriver device.
// Called from: Various functions that interact with the driver.
HANDLE GetDeviceHandle()
{
	HANDLE DeviceHandle = CreateFileW(
		LR"(\\.\TelemetrySourcererDriver)",
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr);

	return DeviceHandle;
}

// Function:    PopulateKmcModules
// Description: Populates a list of known modules so they aren't highlighted.
// Called from: GetCallbacks as a one-time call if not populated.
VOID PopulateKmcModules()
{
	CommonModules.insert(L"ntoskrnl.exe");
	CommonModules.insert(L"ksecdd.sys");
	CommonModules.insert(L"cng.sys");
	CommonModules.insert(L"tcpip.sys");
	CommonModules.insert(L"dxgkrnl.sys");
	CommonModules.insert(L"peauth.sys");
	CommonModules.insert(L"iorate.sys");
	CommonModules.insert(L"mmcss.sys");
	CommonModules.insert(L"ahcache.sys");
	CommonModules.insert(L"CI.dll");
	CommonModules.insert(L"luafv.sys");
	CommonModules.insert(L"npsvctrig.sys");
	CommonModules.insert(L"Wof.sys");
	CommonModules.insert(L"fileinfo.sys");
	CommonModules.insert(L"wcifs.sys");
	CommonModules.insert(L"bindflt.sys");
}

// Function:    GetCallbackModule
// Description: Returns the module associated with a given callback address.
// Called from: GetCallbacks
PMODULE_INFO GetCallbackModule(PMODULE_INFO Modules, PVOID CallbackAddress)
{
	for (int i = 0; i < MAX_CALLBACKS; i++)
	{
		ULONG64 StartAddress = (ULONG64)Modules[i].Address;
		ULONG64 EndAddress = StartAddress + Modules[i].Size;

		if ((StartAddress <= (ULONG64)CallbackAddress) && ((ULONG64)CallbackAddress <= EndAddress))
			return &Modules[i];
	}

	return nullptr;
}

// Function:    GetQword
// Description: Calls the driver and returns the QWORD value at a given pointer.
// Called from: GetCallbacks when getting the original first bytes of a callback.
// Remarks:     This can be abused as a read primitive.
ULONG64 GetQword(PVOID Address)
{
	// Get a handle to the device.
	HANDLE DeviceHandle = GetDeviceHandle();
	if (DeviceHandle == (HANDLE)ERROR_INVALID_HANDLE)
		return NULL;

	// Get pointer.
	DWORD BytesReturned = 0;
	QWORD_INFO InputBuffer = { (PULONG64)Address, NULL };
	BOOL Status = DeviceIoControl(
		DeviceHandle,
		IOCTL_GET_QWORD,
		&InputBuffer, sizeof(QWORD_INFO),
		&InputBuffer, sizeof(QWORD_INFO),
		&BytesReturned, nullptr);
	CloseHandle(DeviceHandle);

	// Return pointer.
	return InputBuffer.Value;
}

// Function:    SetQword
// Description: Calls the driver and sets the QWORD value at a given pointer.
// Called from: SuppressCallback and RevertCallback.
// Remarks:     This can be abused as a write primitive.
BOOL SetQword(PVOID Address, ULONG64 Value)
{
	// Get a handle to the device.
	HANDLE DeviceHandle = GetDeviceHandle();
	if (DeviceHandle == (HANDLE)ERROR_INVALID_HANDLE)
		return FALSE;

	// Set byte.
	DWORD BytesReturned = 0;
	QWORD_INFO InputBuffer = { (PULONG64)Address, Value };
	BOOL Status = DeviceIoControl(
		DeviceHandle,
		IOCTL_SET_QWORD,
		&InputBuffer, sizeof(QWORD_INFO),
		nullptr, 0,
		&BytesReturned, nullptr);
	CloseHandle(DeviceHandle);

	// Return status.
	return Status;
}

// Function:    GetCallbacks
// Description: Returns a list of callback entries to be displayed in the list view.
// Called from: KmcLoadResults
std::vector<PCALLBACK_ENTRY> GetCallbacks(std::vector<PCALLBACK_ENTRY> OldCallbackEntries)
{
	std::vector<PCALLBACK_ENTRY> CallbackEntries;

	// Populate hashes.
	if (!CommonModules.size())
		PopulateKmcModules();

	// Get a handle to the device.
	HANDLE DeviceHandle = GetDeviceHandle();
	if (DeviceHandle == (HANDLE)ERROR_INVALID_HANDLE)
		return CallbackEntries;

	// Get module information.
	BOOL Status = 0;
	DWORD BytesReturned = 0;
	PMODULE_INFO ModuleInfos = (PMODULE_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MODULE_INFO) * MAX_MODULES);
	Status = DeviceIoControl(
		DeviceHandle,
		IOCTL_GET_MODULES,
		ModuleInfos, sizeof(MODULE_INFO) * MAX_MODULES,
		ModuleInfos, sizeof(MODULE_INFO) * MAX_MODULES,
		&BytesReturned, nullptr);

	// Get callback information.
	PCALLBACK_INFO CallbackInfos = (PCALLBACK_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CALLBACK_INFO) * MAX_CALLBACKS);
	Status = DeviceIoControl(
		DeviceHandle,
		IOCTL_GET_CALLBACKS,
		CallbackInfos, sizeof(CALLBACK_INFO) * MAX_CALLBACKS,
		CallbackInfos, sizeof(CALLBACK_INFO) * MAX_CALLBACKS,
		&BytesReturned, nullptr);
	CloseHandle(DeviceHandle);

	if (!BytesReturned)
	{
		HeapFree(GetProcessHeap(), NULL, ModuleInfos);
		HeapFree(GetProcessHeap(), NULL, CallbackInfos);
		return CallbackEntries;
	}

	for (int i = 0; i < MAX_CALLBACKS; i++)
	{
		if (CallbackInfos[i].Address)
		{
			// Populate callback information.
			PCALLBACK_ENTRY CallbackEntry = new CALLBACK_ENTRY;
			CallbackEntry->Type = CallbackInfos[i].Type;
			CallbackEntry->Address = CallbackInfos[i].Address;

			// Populate module information.
			PMODULE_INFO CallbackModule = GetCallbackModule(ModuleInfos, CallbackEntry->Address);
			if (CallbackModule)
			{
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (PCCH)CallbackModule->Name, -1, (LPWSTR)CallbackEntry->ModuleName, MAX_PATH - 1);
				CallbackEntry->ModuleOffset = (ULONG64)CallbackEntry->Address - (ULONG64)CallbackModule->Address;
				if (!CommonModules.count(CallbackEntry->ModuleName))
					CallbackEntry->Notable = TRUE;
			}

			// Populate suppression information.
			ULONG64 SuppressionValue = GetSuppressionValue(CallbackEntry->Type);
			CallbackEntry->OriginalQword = GetQword(CallbackEntry->Address);
			CallbackEntry->Suppressed = (CallbackEntry->OriginalQword == SuppressionValue) ? TRUE : FALSE;
			if (CallbackEntry->OriginalQword == SuppressionValue) // Correct if there's an original non-ret byte.
				if (OldCallbackEntries.size())
					for (PCALLBACK_ENTRY OldEntry : OldCallbackEntries)
						if (CallbackEntry->Address == OldEntry->Address)
							if (OldEntry->OriginalQword != SuppressionValue)
								CallbackEntry->OriginalQword = OldEntry->OriginalQword;

			CallbackEntries.push_back(CallbackEntry);
		}
	}

	HeapFree(GetProcessHeap(), NULL, ModuleInfos);
	HeapFree(GetProcessHeap(), NULL, CallbackInfos);
	return CallbackEntries;
}

// Function:    GetSuppressionValue
// Description: Returns the appropriate patch value for the given callback type.
// Called from: GetCallbacks, SuppressCallback, and KmcRevertCallback.
ULONG64 GetSuppressionValue(CALLBACK_TYPE CallbackType)
{
	switch (CallbackType)
	{
	case CALLBACK_TYPE::PsLoadImage:
	case CALLBACK_TYPE::PsProcessCreation:
	case CALLBACK_TYPE::PsThreadCreation:
		return 0xC3;           // return; (ret)
	case CALLBACK_TYPE::CmRegistry:
	case CALLBACK_TYPE::ObProcessHandlePre:
	case CALLBACK_TYPE::ObProcessHandlePost:
	case CALLBACK_TYPE::ObThreadHandlePre:
	case CALLBACK_TYPE::ObThreadHandlePost:
	case CALLBACK_TYPE::ObDesktopHandlePre:
	case CALLBACK_TYPE::ObDesktopHandlePost:
	case CALLBACK_TYPE::MfCreatePost:
	case CALLBACK_TYPE::MfCreateNamedPipePost:
	case CALLBACK_TYPE::MfClosePost:
	case CALLBACK_TYPE::MfReadPost:
	case CALLBACK_TYPE::MfWritePost:
	case CALLBACK_TYPE::MfQueryInformationPost:
	case CALLBACK_TYPE::MfSetInformationPost:
	case CALLBACK_TYPE::MfQueryEaPost:
	case CALLBACK_TYPE::MfSetEaPost:
	case CALLBACK_TYPE::MfFlushBuffersPost:
	case CALLBACK_TYPE::MfQueryVolumeInformationPost:
	case CALLBACK_TYPE::MfSetVolumeInformationPost:
	case CALLBACK_TYPE::MfDirectoryControlPost:
	case CALLBACK_TYPE::MfFileSystemControlPost:
	case CALLBACK_TYPE::MfDeviceControlPost:
	case CALLBACK_TYPE::MfInternalDeviceControlPost:
	case CALLBACK_TYPE::MfShutdownPost:
	case CALLBACK_TYPE::MfLockControlPost:
	case CALLBACK_TYPE::MfCleanupPost:
	case CALLBACK_TYPE::MfCreateMailslotPost:
	case CALLBACK_TYPE::MfQuerySecurityPost:
	case CALLBACK_TYPE::MfSetSecurityPost:
	case CALLBACK_TYPE::MfPowerPost:
	case CALLBACK_TYPE::MfSystemControlPost:
	case CALLBACK_TYPE::MfDeviceChangePost:
	case CALLBACK_TYPE::MfQueryQuotaPost:
	case CALLBACK_TYPE::MfSetQuotaPost:
	case CALLBACK_TYPE::MfPnpPost:
		return 0xC3C033;       // return STATUS_SUCCESS; (xor eax, eax; ret)
	case CALLBACK_TYPE::MfCreatePre:
	case CALLBACK_TYPE::MfCreateNamedPipePre:
	case CALLBACK_TYPE::MfClosePre:
	case CALLBACK_TYPE::MfReadPre:
	case CALLBACK_TYPE::MfWritePre:
	case CALLBACK_TYPE::MfQueryInformationPre:
	case CALLBACK_TYPE::MfSetInformationPre:
	case CALLBACK_TYPE::MfQueryEaPre:
	case CALLBACK_TYPE::MfSetEaPre:
	case CALLBACK_TYPE::MfFlushBuffersPre:
	case CALLBACK_TYPE::MfQueryVolumeInformationPre:
	case CALLBACK_TYPE::MfSetVolumeInformationPre:
	case CALLBACK_TYPE::MfDirectoryControlPre:
	case CALLBACK_TYPE::MfFileSystemControlPre:
	case CALLBACK_TYPE::MfDeviceControlPre:
	case CALLBACK_TYPE::MfInternalDeviceControlPre:
	case CALLBACK_TYPE::MfShutdownPre:
	case CALLBACK_TYPE::MfLockControlPre:
	case CALLBACK_TYPE::MfCleanupPre:
	case CALLBACK_TYPE::MfCreateMailslotPre:
	case CALLBACK_TYPE::MfQuerySecurityPre:
	case CALLBACK_TYPE::MfSetSecurityPre:
	case CALLBACK_TYPE::MfPowerPre:
	case CALLBACK_TYPE::MfSystemControlPre:
	case CALLBACK_TYPE::MfDeviceChangePre:
	case CALLBACK_TYPE::MfQueryQuotaPre:
	case CALLBACK_TYPE::MfSetQuotaPre:
	case CALLBACK_TYPE::MfPnpPre:
		return 0xC300000001B8; // return FLT_PREOP_SUCCESS_NO_CALLBACK; (mov eax, 1; ret)
	default:
		return 0xC3;           // return; (ret)
	}
}

// Function:    SuppressCallback
// Description: Suppresses a given callback.
// Called from: KmcSuppressCallback
BOOL SuppressCallback(PCALLBACK_ENTRY Callback)
{
	return SetQword(Callback->Address, GetSuppressionValue(Callback->Type));
}

// Function:    RevertCallback
// Description: Reverts a given callback.
// Called from: KmcRevertCallback
// Remarks:     KmcRevertCallback checks for eligibility.
BOOL RevertCallback(PCALLBACK_ENTRY Callback)
{
	return SetQword(Callback->Address, Callback->OriginalQword);
}