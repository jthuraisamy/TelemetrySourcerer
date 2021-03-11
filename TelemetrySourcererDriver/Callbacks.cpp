#include <fltKernel.h>

#include "TelemetrySourcererDriver.h"
#include "Callbacks.h"
#include "Memory.h"

#pragma comment(lib, "FltMgr.lib")

UCHAR LI_PATTERN_W7_07600[] = { 0x41, 0x0f, 0xba, 0x6d, 0x00, 0x0a, 0xbb, 0x01, 0x00, 0x00, 0x00, 0x4c, 0x8b, 0xf2, 0x4c, 0x8b, 0xf9 };
UCHAR LI_PATTERN_W8_08102[] = { 0xbf, 0x08, 0x00, 0x00, 0x00, 0x41, 0x89, 0x06, 0x0f, 0x1f, 0x04, 0x00, 0x48, 0x8b, 0xcb, 0xe8 };
UCHAR LI_PATTERN_W8_09431[] = { 0x45, 0x33, 0xc0, 0x48, 0x8b, 0xd7, 0x48, 0x8d, 0x0c, 0xd9, 0xe8 };
UCHAR LI_PATTERN_WX_10240[] = { 0x45, 0x33, 0xC0, 0x48, 0x8D, 0x0C, 0xD9, 0x48, 0x8B, 0xD7, 0xE8 };
CALLBACK_SEARCH_QUERY LoadImageCallbackQueries[14] = {
	{ WINDOWS_INDEX::W7_07600, RTL_CONSTANT_STRING(L"FsRtlReleaseFile"),                    RTL_CONSTANT_STRING(L"IoSetPartitionInformationEx"),               LI_PATTERN_W7_07600, sizeof(LI_PATTERN_W7_07600), -4 },
	{ WINDOWS_INDEX::W8_08102, RTL_CONSTANT_STRING(L"ExSizeOfRundownProtectionCacheAware"), RTL_CONSTANT_STRING(L"MmProbeAndLockProcessPages"),                LI_PATTERN_W8_08102, sizeof(LI_PATTERN_W8_08102), -4 },
	{ WINDOWS_INDEX::W8_09431, RTL_CONSTANT_STRING(L"PsSetLoadImageNotifyRoutine"),         RTL_CONSTANT_STRING(L"PsSetCreateThreadNotifyRoutine"),            LI_PATTERN_W8_09431, sizeof(LI_PATTERN_W8_09431), -4 },
	{ WINDOWS_INDEX::WX_10240, RTL_CONSTANT_STRING(L"PsSetLoadImageNotifyRoutine"),         RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),           LI_PATTERN_WX_10240, sizeof(LI_PATTERN_WX_10240), -4 },
	{ WINDOWS_INDEX::WX_10586, RTL_CONSTANT_STRING(L"PsSetLoadImageNotifyRoutine"),         RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),           LI_PATTERN_WX_10240, sizeof(LI_PATTERN_WX_10240), -4 },
	{ WINDOWS_INDEX::WX_14393, RTL_CONSTANT_STRING(L"PsSetLoadImageNotifyRoutine"),         RTL_CONSTANT_STRING(L"SeRegisterLogonSessionTerminatedRoutineEx"), LI_PATTERN_WX_10240, sizeof(LI_PATTERN_WX_10240), -4 },
	{ WINDOWS_INDEX::WX_15063, RTL_CONSTANT_STRING(L"PsSetLoadImageNotifyRoutine"),         RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),           LI_PATTERN_WX_10240, sizeof(LI_PATTERN_WX_10240), -4 },
	{ WINDOWS_INDEX::WX_16299, RTL_CONSTANT_STRING(L"PsSetLoadImageNotifyRoutine"),         RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),           LI_PATTERN_WX_10240, sizeof(LI_PATTERN_WX_10240), -4 },
	{ WINDOWS_INDEX::WX_17134, RTL_CONSTANT_STRING(L"PsSetLoadImageNotifyRoutine"),         RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),           LI_PATTERN_WX_10240, sizeof(LI_PATTERN_WX_10240), -4 },
	{ WINDOWS_INDEX::WX_17763, RTL_CONSTANT_STRING(L"PsSetLoadImageNotifyRoutineEx"),       RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),           LI_PATTERN_WX_10240, sizeof(LI_PATTERN_WX_10240), -4 },
	{ WINDOWS_INDEX::WX_18362, RTL_CONSTANT_STRING(L"PsSetLoadImageNotifyRoutineEx"),       RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),           LI_PATTERN_WX_10240, sizeof(LI_PATTERN_WX_10240), -4 },
	{ WINDOWS_INDEX::WX_18363, RTL_CONSTANT_STRING(L"PsSetLoadImageNotifyRoutineEx"),       RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),           LI_PATTERN_WX_10240, sizeof(LI_PATTERN_WX_10240), -4 },
	{ WINDOWS_INDEX::WX_19041, RTL_CONSTANT_STRING(L"PsSetLoadImageNotifyRoutineEx"),       RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),           LI_PATTERN_WX_10240, sizeof(LI_PATTERN_WX_10240), -4 },
	{ WINDOWS_INDEX::WX_19042, RTL_CONSTANT_STRING(L"PsSetLoadImageNotifyRoutineEx"),       RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),           LI_PATTERN_WX_10240, sizeof(LI_PATTERN_WX_10240), -4 },
};

UCHAR CP_PATTERN_W7_07600[] = { 0x4c, 0x8b, 0xf9, 0x48, 0x8d, 0x0c, 0xc1, 0xe8 };
UCHAR CP_PATTERN_W8_08102[] = { 0x8b, 0xc3, 0x48, 0x8d, 0x34, 0xc1, 0x48, 0x8b, 0xce, 0xe8 };
UCHAR CP_PATTERN_W8_09431[] = { 0x48, 0x8d, 0x04, 0xc1, 0x48, 0x89, 0x45, 0x70, 0x48, 0x8b, 0xc8, 0xe8 };
UCHAR CP_PATTERN_WX_10240[] = { 0x8b, 0xc3, 0x45, 0x33, 0xc0, 0x48, 0x8b, 0xd6, 0x49, 0x8d, 0x0c, 0xc7, 0xe8 };
UCHAR CP_PATTERN_WX_10586[] = { 0x49, 0x8d, 0x0c, 0xff, 0x45, 0x33, 0xc0, 0x48, 0x8b, 0xd6, 0xe8 };
UCHAR CP_PATTERN_WX_14393[] = { 0x49, 0x8d, 0x0c, 0xfc, 0x45, 0x33, 0xc0, 0x48, 0x8b, 0xd6, 0xe8 };
UCHAR CP_PATTERN_WX_15063[] = { 0x49, 0x8d, 0x0c, 0xdc, 0x45, 0x33, 0xc0, 0x48, 0x8b, 0xd6, 0xe8 };
UCHAR CP_PATTERN_WX_16299[] = { 0x48, 0x8d, 0x0c, 0xdd, 0x00, 0x00, 0x00, 0x00, 0x45, 0x33, 0xc0, 0x49, 0x03, 0xcd, 0x48, 0x8b };
CALLBACK_SEARCH_QUERY CreateProcessCallbackQueries[14] = {
	{ WINDOWS_INDEX::W7_07600, RTL_CONSTANT_STRING(L"RtlAreAllAccessesGranted"),            RTL_CONSTANT_STRING(L"RtlGetIntegerAtom"),                         CP_PATTERN_W7_07600, sizeof(CP_PATTERN_W7_07600), -4 },
	{ WINDOWS_INDEX::W8_08102, RTL_CONSTANT_STRING(L"PsAcquireProcessExitSynchronization"), RTL_CONSTANT_STRING(L"FsRtlAddToTunnelCache"),                     CP_PATTERN_W8_08102, sizeof(CP_PATTERN_W8_08102), -4 },
	{ WINDOWS_INDEX::W8_09431, RTL_CONSTANT_STRING(L"ObCreateObject"),                      RTL_CONSTANT_STRING(L"NtFindAtom"),                                CP_PATTERN_W8_09431, sizeof(CP_PATTERN_W8_09431), -4 },
	{ WINDOWS_INDEX::WX_10240, RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),     RTL_CONSTANT_STRING(L"KeRegisterProcessorChangeCallback"),         CP_PATTERN_WX_10240, sizeof(CP_PATTERN_WX_10240), -4 },
	{ WINDOWS_INDEX::WX_10586, RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),     RTL_CONSTANT_STRING(L"KeRegisterProcessorChangeCallback"),         CP_PATTERN_WX_10586, sizeof(CP_PATTERN_WX_10586), -4 },
	{ WINDOWS_INDEX::WX_14393, RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),     RTL_CONSTANT_STRING(L"KeRegisterProcessorChangeCallback"),         CP_PATTERN_WX_14393, sizeof(CP_PATTERN_WX_14393), -4 },
	{ WINDOWS_INDEX::WX_15063, RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),     RTL_CONSTANT_STRING(L"KeRegisterProcessorChangeCallback"),         CP_PATTERN_WX_15063, sizeof(CP_PATTERN_WX_15063), -4 },
	{ WINDOWS_INDEX::WX_16299, RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),     RTL_CONSTANT_STRING(L"RtlGetSystemBootStatus"),                    CP_PATTERN_WX_16299, sizeof(CP_PATTERN_WX_16299), -4 },
	{ WINDOWS_INDEX::WX_17134, RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),     RTL_CONSTANT_STRING(L"EtwEnableTrace"),                            CP_PATTERN_WX_16299, sizeof(CP_PATTERN_WX_16299), -4 },
	{ WINDOWS_INDEX::WX_17763, RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),     RTL_CONSTANT_STRING(L"IoCreateDriver"),                            CP_PATTERN_WX_16299, sizeof(CP_PATTERN_WX_16299), -4 },
	{ WINDOWS_INDEX::WX_18362, RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),     RTL_CONSTANT_STRING(L"IoCreateDriver"),                            CP_PATTERN_WX_16299, sizeof(CP_PATTERN_WX_16299), -4 },
	{ WINDOWS_INDEX::WX_18363, RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),     RTL_CONSTANT_STRING(L"IoCreateDriver"),                            CP_PATTERN_WX_16299, sizeof(CP_PATTERN_WX_16299), -4 },
	{ WINDOWS_INDEX::WX_19041, RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),     RTL_CONSTANT_STRING(L"IoCreateDriver"),                            CP_PATTERN_WX_16299, sizeof(CP_PATTERN_WX_16299), -4 },
	{ WINDOWS_INDEX::WX_19042, RTL_CONSTANT_STRING(L"PsSetCreateProcessNotifyRoutine"),     RTL_CONSTANT_STRING(L"IoCreateDriver"),                            CP_PATTERN_WX_16299, sizeof(CP_PATTERN_WX_16299), -4 },
};

UCHAR CT_PATTERN_W7_07600[] = { 0xbf, 0x40, 0x00, 0x00, 0x00, 0x48, 0x8b, 0xcb, 0xe8 };
UCHAR CT_PATTERN_WX_10240[] = { 0x48, 0x8b, 0xcd, 0xe8 };
CALLBACK_SEARCH_QUERY CreateThreadCallbackQueries[14] = {
	{ WINDOWS_INDEX::W7_07600, RTL_CONSTANT_STRING(L"RtlUnicodeToMultiByteSize"),           RTL_CONSTANT_STRING(L"MmLockPagableSectionByHandle"),              CT_PATTERN_W7_07600, sizeof(CT_PATTERN_W7_07600), -5 },
	{ WINDOWS_INDEX::W8_08102, RTL_CONSTANT_STRING(L"PsAcquireProcessExitSynchronization"), RTL_CONSTANT_STRING(L"FsRtlAddToTunnelCache"),                     CT_PATTERN_W7_07600, sizeof(CT_PATTERN_W7_07600), -4 },
	{ WINDOWS_INDEX::W8_09431, RTL_CONSTANT_STRING(L"ObCreateObject"),                      RTL_CONSTANT_STRING(L"NtFindAtom"),                                CT_PATTERN_W7_07600, sizeof(CT_PATTERN_W7_07600), -5 },
	{ WINDOWS_INDEX::WX_10240, RTL_CONSTANT_STRING(L"PsRemoveCreateThreadNotifyRoutine"),   RTL_CONSTANT_STRING(L"PsRemoveLoadImageNotifyRoutine"),            CT_PATTERN_WX_10240, sizeof(CT_PATTERN_WX_10240), -8 },
	{ WINDOWS_INDEX::WX_10586, RTL_CONSTANT_STRING(L"PsRemoveCreateThreadNotifyRoutine"),   RTL_CONSTANT_STRING(L"PsRemoveLoadImageNotifyRoutine"),            CT_PATTERN_WX_10240, sizeof(CT_PATTERN_WX_10240), -8 },
	{ WINDOWS_INDEX::WX_14393, RTL_CONSTANT_STRING(L"PsRemoveCreateThreadNotifyRoutine"),   RTL_CONSTANT_STRING(L"PsRemoveLoadImageNotifyRoutine"),            CT_PATTERN_WX_10240, sizeof(CT_PATTERN_WX_10240), -8 },
	{ WINDOWS_INDEX::WX_15063, RTL_CONSTANT_STRING(L"PsRemoveCreateThreadNotifyRoutine"),   RTL_CONSTANT_STRING(L"PsRemoveLoadImageNotifyRoutine"),            CT_PATTERN_WX_10240, sizeof(CT_PATTERN_WX_10240), -8 },
	{ WINDOWS_INDEX::WX_16299, RTL_CONSTANT_STRING(L"PsRemoveCreateThreadNotifyRoutine"),   RTL_CONSTANT_STRING(L"PsRemoveLoadImageNotifyRoutine"),            CT_PATTERN_WX_10240, sizeof(CT_PATTERN_WX_10240), -8 },
	{ WINDOWS_INDEX::WX_17134, RTL_CONSTANT_STRING(L"PsRemoveCreateThreadNotifyRoutine"),   RTL_CONSTANT_STRING(L"PsRemoveLoadImageNotifyRoutine"),            CT_PATTERN_WX_10240, sizeof(CT_PATTERN_WX_10240), -8 },
	{ WINDOWS_INDEX::WX_17763, RTL_CONSTANT_STRING(L"PsRemoveCreateThreadNotifyRoutine"),   RTL_CONSTANT_STRING(L"PsRemoveLoadImageNotifyRoutine"),            CT_PATTERN_WX_10240, sizeof(CT_PATTERN_WX_10240), -8 },
	{ WINDOWS_INDEX::WX_18362, RTL_CONSTANT_STRING(L"PsRemoveCreateThreadNotifyRoutine"),   RTL_CONSTANT_STRING(L"PsRemoveLoadImageNotifyRoutine"),            CT_PATTERN_WX_10240, sizeof(CT_PATTERN_WX_10240), -8 },
	{ WINDOWS_INDEX::WX_18363, RTL_CONSTANT_STRING(L"PsRemoveCreateThreadNotifyRoutine"),   RTL_CONSTANT_STRING(L"PsRemoveLoadImageNotifyRoutine"),            CT_PATTERN_WX_10240, sizeof(CT_PATTERN_WX_10240), -8 },
	{ WINDOWS_INDEX::WX_19041, RTL_CONSTANT_STRING(L"PsRemoveCreateThreadNotifyRoutine"),   RTL_CONSTANT_STRING(L"PsRemoveLoadImageNotifyRoutine"),            CT_PATTERN_WX_10240, sizeof(CT_PATTERN_WX_10240), -8 },
	{ WINDOWS_INDEX::WX_19042, RTL_CONSTANT_STRING(L"PsRemoveCreateThreadNotifyRoutine"),   RTL_CONSTANT_STRING(L"PsRemoveLoadImageNotifyRoutine"),            CT_PATTERN_WX_10240, sizeof(CT_PATTERN_WX_10240), -8 },
};

UCHAR RG_PATTERN_W7_07600[] = { 0x48, 0x8b, 0xf8, 0x48, 0x89, 0x44, 0x24, 0x28, 0x48, 0x3b, 0xc3, 0x0f, 0x84 };
UCHAR RG_PATTERN_W8_08102[] = { 0x49, 0x8b, 0x04, 0x24, 0x48, 0x3b, 0x43, 0x18, 0x74 };
UCHAR RG_PATTERN_WX_10240[] = { 0x48, 0x8b, 0xf8, 0x48, 0x89, 0x44, 0x24, 0x40, 0x48, 0x85, 0xc0, 0x0f, 0x84 };
CALLBACK_SEARCH_QUERY RegistryCallbackQueries[14] = {
	{ WINDOWS_INDEX::W7_07600, RTL_CONSTANT_STRING(L"CmUnRegisterCallback"),                RTL_CONSTANT_STRING(L"CmRegisterCallback"),                        RG_PATTERN_W7_07600, sizeof(RG_PATTERN_W7_07600), -9 },
	{ WINDOWS_INDEX::W8_08102, RTL_CONSTANT_STRING(L"CmSetCallbackObjectContext"),          RTL_CONSTANT_STRING(L"CmGetCallbackVersion"),                      RG_PATTERN_W8_08102, sizeof(RG_PATTERN_W8_08102), -9 },
	{ WINDOWS_INDEX::W8_09431, RTL_CONSTANT_STRING(L"CmSetCallbackObjectContext"),          RTL_CONSTANT_STRING(L"DbgkLkmdUnregisterCallback"),                RG_PATTERN_W8_08102, sizeof(RG_PATTERN_W8_08102), -9 },
	{ WINDOWS_INDEX::WX_10240, RTL_CONSTANT_STRING(L"CmUnRegisterCallback"),                RTL_CONSTANT_STRING(L"FsRtlAllocateResource"),                     RG_PATTERN_WX_10240, sizeof(RG_PATTERN_WX_10240), -9 },
	{ WINDOWS_INDEX::WX_10586, RTL_CONSTANT_STRING(L"CmUnRegisterCallback"),                RTL_CONSTANT_STRING(L"FsRtlAllocateResource"),                     RG_PATTERN_WX_10240, sizeof(RG_PATTERN_WX_10240), -9 },
	{ WINDOWS_INDEX::WX_14393, RTL_CONSTANT_STRING(L"CmUnRegisterCallback"),                RTL_CONSTANT_STRING(L"FsRtlAllocateResource"),                     RG_PATTERN_WX_10240, sizeof(RG_PATTERN_WX_10240), -9 },
	{ WINDOWS_INDEX::WX_15063, RTL_CONSTANT_STRING(L"CmUnRegisterCallback"),                RTL_CONSTANT_STRING(L"DbgkLkmdUnregisterCallback"),                RG_PATTERN_WX_10240, sizeof(RG_PATTERN_WX_10240), -9 },
	{ WINDOWS_INDEX::WX_16299, RTL_CONSTANT_STRING(L"CmUnRegisterCallback"),                RTL_CONSTANT_STRING(L"DbgkLkmdUnregisterCallback"),                RG_PATTERN_WX_10240, sizeof(RG_PATTERN_WX_10240), -9 },
	{ WINDOWS_INDEX::WX_17134, RTL_CONSTANT_STRING(L"CmUnRegisterCallback"),                RTL_CONSTANT_STRING(L"DbgkLkmdUnregisterCallback"),                RG_PATTERN_WX_10240, sizeof(RG_PATTERN_WX_10240), -9 },
	{ WINDOWS_INDEX::WX_17763, RTL_CONSTANT_STRING(L"CmUnRegisterCallback"),                RTL_CONSTANT_STRING(L"DbgkLkmdUnregisterCallback"),                RG_PATTERN_WX_10240, sizeof(RG_PATTERN_WX_10240), -9 },
	{ WINDOWS_INDEX::WX_18362, RTL_CONSTANT_STRING(L"CmUnRegisterCallback"),                RTL_CONSTANT_STRING(L"DbgkLkmdUnregisterCallback"),                RG_PATTERN_WX_10240, sizeof(RG_PATTERN_WX_10240), -9 },
	{ WINDOWS_INDEX::WX_18363, RTL_CONSTANT_STRING(L"CmUnRegisterCallback"),                RTL_CONSTANT_STRING(L"DbgkLkmdUnregisterCallback"),                RG_PATTERN_WX_10240, sizeof(RG_PATTERN_WX_10240), -9 },
	{ WINDOWS_INDEX::WX_19041, RTL_CONSTANT_STRING(L"CmUnRegisterCallback"),                RTL_CONSTANT_STRING(L"DbgkLkmdUnregisterCallback"),                RG_PATTERN_WX_10240, sizeof(RG_PATTERN_WX_10240), -9 },
	{ WINDOWS_INDEX::WX_19042, RTL_CONSTANT_STRING(L"CmUnRegisterCallback"),                RTL_CONSTANT_STRING(L"DbgkLkmdUnregisterCallback"),                RG_PATTERN_WX_10240, sizeof(RG_PATTERN_WX_10240), -9 },
};

UCHAR OB_PATTERN_W7_07600[] = { 0x41, 0x8a, 0xde, 0x44, 0x88, 0x74, 0x24, 0x47, 0x88, 0x5c, 0x24, 0x46, 0x4c, 0x89, 0x74, 0x24, 0x38, 0x4c, 0x89, 0x74, 0x24, 0x30, 0x49, 0x8b, 0xee, 0xc7, 0x44, 0x24, 0x48 };
UCHAR OB_PATTERN_W8_08102[] = { 0x41, 0x8a, 0xd8, 0x44, 0x88, 0x44, 0x24, 0x4f, 0x88, 0x5c, 0x24, 0x4e, 0x4c, 0x89, 0x44, 0x24, 0x38, 0x4d, 0x8b, 0xf0, 0x4c, 0x89, 0x44, 0x24, 0x30, 0xc7, 0x44, 0x24, 0x50 };
UCHAR OB_PATTERN_WX_10240[] = { 0x0f, 0xb7, 0x02, 0xff, 0xc9, 0x49, 0x03 };
CALLBACK_SEARCH_QUERY ObjectCallbackQueries[14] = {
	{ WINDOWS_INDEX::W7_07600, RTL_CONSTANT_STRING(L"ObUnRegisterCallbacks"),               RTL_CONSTANT_STRING(L"ObCreateObjectType"),                        OB_PATTERN_W7_07600, sizeof(OB_PATTERN_W7_07600), -4 },
	{ WINDOWS_INDEX::W8_08102, RTL_CONSTANT_STRING(L"ObCreateObjectType"),                  RTL_CONSTANT_STRING(L"IoCreateController"),                        OB_PATTERN_W8_08102, sizeof(OB_PATTERN_W8_08102), -4 },
	{ WINDOWS_INDEX::W8_09431, RTL_CONSTANT_STRING(L"ObCreateObjectType"),                  RTL_CONSTANT_STRING(L"RtlRunOnceInitialize"),                      OB_PATTERN_W8_08102, sizeof(OB_PATTERN_W8_08102), -4 },
	{ WINDOWS_INDEX::WX_10240, RTL_CONSTANT_STRING(L"ObCreateObjectType"),                  RTL_CONSTANT_STRING(L"RtlRunOnceInitialize"),                      OB_PATTERN_WX_10240, sizeof(OB_PATTERN_WX_10240), 25 },
	{ WINDOWS_INDEX::WX_10586, RTL_CONSTANT_STRING(L"ObCreateObjectType"),                  RTL_CONSTANT_STRING(L"RtlRunOnceInitialize"),                      OB_PATTERN_WX_10240, sizeof(OB_PATTERN_WX_10240), 25 },
	{ WINDOWS_INDEX::WX_14393, RTL_CONSTANT_STRING(L"ObCreateObjectType"),                  RTL_CONSTANT_STRING(L"KseRegisterShim"),                           OB_PATTERN_WX_10240, sizeof(OB_PATTERN_WX_10240), 25 },
	{ WINDOWS_INDEX::WX_15063, RTL_CONSTANT_STRING(L"ObCreateObjectType"),                  RTL_CONSTANT_STRING(L"IoCreateDriver"),                            OB_PATTERN_WX_10240, sizeof(OB_PATTERN_WX_10240), 25 },
	{ WINDOWS_INDEX::WX_16299, RTL_CONSTANT_STRING(L"ObCreateObjectType"),                  RTL_CONSTANT_STRING(L"IoCreateDriver"),                            OB_PATTERN_WX_10240, sizeof(OB_PATTERN_WX_10240), 25 },
	{ WINDOWS_INDEX::WX_17134, RTL_CONSTANT_STRING(L"ObCreateObjectType"),                  RTL_CONSTANT_STRING(L"IoCreateDriver"),                            OB_PATTERN_WX_10240, sizeof(OB_PATTERN_WX_10240), 25 },
	{ WINDOWS_INDEX::WX_17763, RTL_CONSTANT_STRING(L"ObCreateObjectType"),                  RTL_CONSTANT_STRING(L"IoCreateDriver"),                            OB_PATTERN_WX_10240, sizeof(OB_PATTERN_WX_10240), 25 },
	{ WINDOWS_INDEX::WX_18362, RTL_CONSTANT_STRING(L"ObCreateObjectType"),                  RTL_CONSTANT_STRING(L"IoCreateDriver"),                            OB_PATTERN_WX_10240, sizeof(OB_PATTERN_WX_10240), 25 },
	{ WINDOWS_INDEX::WX_18363, RTL_CONSTANT_STRING(L"ObCreateObjectType"),                  RTL_CONSTANT_STRING(L"PoCreateThermalRequest"),                    OB_PATTERN_WX_10240, sizeof(OB_PATTERN_WX_10240), 25 },
	{ WINDOWS_INDEX::WX_19041, RTL_CONSTANT_STRING(L"ObCreateObjectType"),                  RTL_CONSTANT_STRING(L"IoCreateDriver"),                            OB_PATTERN_WX_10240, sizeof(OB_PATTERN_WX_10240), 25 },
	{ WINDOWS_INDEX::WX_19042, RTL_CONSTANT_STRING(L"ObCreateObjectType"),                  RTL_CONSTANT_STRING(L"IoCreateDriver"),                            OB_PATTERN_WX_10240, sizeof(OB_PATTERN_WX_10240), 25 },
};

FLT_POSTOP_CALLBACK_STATUS
PfltPostOperationCallback(
	PFLT_CALLBACK_DATA Data,
	PCFLT_RELATED_OBJECTS FltObjects,
	PVOID CompletionContext,
	FLT_POST_OPERATION_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS
PfltPreOperationCallback(
	PFLT_CALLBACK_DATA Data,
	PCFLT_RELATED_OBJECTS FltObjects,
	PVOID* CompletionContext
)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

WINDOWS_INDEX GetWindowsIndex()
{
	PfltPreOperationCallback(nullptr, nullptr, nullptr);
	PfltPostOperationCallback(nullptr, nullptr, nullptr, NULL);

	OSVERSIONINFOEXW OsVersionInfo;
	OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
	RtlGetVersion((POSVERSIONINFOW)&OsVersionInfo);
	
	switch (OsVersionInfo.dwBuildNumber)
	{
	case 7600:
	case 7601:
		return WINDOWS_INDEX::W7_07600;
		break;
	case 8102:
	case 8250:
	case 9200:
		return WINDOWS_INDEX::W8_08102;
	case 9431:
	case 9600:
		return WINDOWS_INDEX::W8_09431;
		break;
	case 10240: // 1507
		return WINDOWS_INDEX::WX_10240;
		break;
	case 10586: // 1511
		return WINDOWS_INDEX::WX_10586;
		break;
	case 14393: // 1607
		return WINDOWS_INDEX::WX_14393;
		break;
	case 15063: // 1703
		return WINDOWS_INDEX::WX_15063;
		break;
	case 16299: // 1709
		return WINDOWS_INDEX::WX_16299;
		break;
	case 17134: // 1803
		return WINDOWS_INDEX::WX_17134;
		break;
	case 17763: // 1809
		return WINDOWS_INDEX::WX_17763;
		break;
	case 18362: // 1903
		return WINDOWS_INDEX::WX_18362;
		break;
	case 18363: // 1909
		return WINDOWS_INDEX::WX_18363;
		break;
	case 19041: // 2004
		return WINDOWS_INDEX::WX_19041;
		break;
	case 19042: // 20H2
		return WINDOWS_INDEX::WX_19042;
		break;
	default:
		return WINDOWS_INDEX::Unsupported;
	}
}

NTSTATUS GetLoadImageNotifyCallbacks(OUT CALLBACK_INFO* CallbackArray, OUT ULONG* CallbackCount)
{
	// Check if supported for this version of Windows.
	WINDOWS_INDEX WindowsIndex = GetWindowsIndex();
	if (WindowsIndex == WINDOWS_INDEX::Unsupported)
		return STATUS_NOT_SUPPORTED;

	// Get search query.
	CALLBACK_SEARCH_QUERY Query = LoadImageCallbackQueries[(ULONG)WindowsIndex];

	// Resolve function addresses to get the start and end of the search space.
	PUCHAR StartAddress = (PUCHAR)MmGetSystemRoutineAddress(&Query.StartFunction);
	PUCHAR EndAddress = (PUCHAR)MmGetSystemRoutineAddress(&Query.EndFunction);

	// Search for an offset to the PspLoadImageNotifyRoutine array.
	// This array contains the registered load image notification callbacks.
	PUCHAR PspLoadImageNotifyRoutine = nullptr;
	NTSTATUS Status = MemorySearch(StartAddress, EndAddress, Query.PatternBuffer, Query.PatternSize, &PspLoadImageNotifyRoutine);

	if (!NT_SUCCESS(Status))
		return STATUS_NOT_FOUND;

	// Resolve the offset to get the address of PspLoadImageNotifyRoutine.
	PspLoadImageNotifyRoutine += Query.PatternOffset;
	PspLoadImageNotifyRoutine += *(PLONG)(PspLoadImageNotifyRoutine);
	PspLoadImageNotifyRoutine += sizeof(LONG);

	// Copy the callback addresses to the provided array.
	for (int i = 0; i < MAX_CALLBACKS_LOAD_IMAGE; i++)
	{
		// Check if a pointer to the callback exists at this index of the registered callbacks array.
		PVOID PointerToCallback = (PVOID)(*(PULONG64)((ULONG64)PspLoadImageNotifyRoutine + i * 8LL));
		
		// If so, copy the callback address to the output array.
		if (PointerToCallback)
		{
			CallbackArray[i].Type = CALLBACK_TYPE::PsLoadImage;
			CallbackArray[i].Address = (PVOID)(*(PULONG64)((ULONG64)PointerToCallback & 0xFFFFFFFFFFFFFFF8));
		}

		// Otherwise, exit the loop as there are no more callbacks left.
		else
		{
			*CallbackCount = i;
			break;
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS GetCreateProcessNotifyCallbacks(OUT CALLBACK_INFO* CallbackArray, OUT ULONG* CallbackCount)
{
	// Check if supported for this version of Windows.
	WINDOWS_INDEX WindowsIndex = GetWindowsIndex();
	if (WindowsIndex == WINDOWS_INDEX::Unsupported)
		return STATUS_NOT_SUPPORTED;

	// Get search query.
	CALLBACK_SEARCH_QUERY Query = CreateProcessCallbackQueries[(ULONG)WindowsIndex];

	// Resolve function addresses to get the start and end of the search space.
	PUCHAR StartAddress = (PUCHAR)MmGetSystemRoutineAddress(&Query.StartFunction);
	PUCHAR EndAddress = (PUCHAR)MmGetSystemRoutineAddress(&Query.EndFunction);

	// Search for an offset to the PspCreateProcessNotifyRoutine array.
	PUCHAR PspSetCreateProcessNotifyRoutine = nullptr;
	NTSTATUS Status = MemorySearch(StartAddress, EndAddress, Query.PatternBuffer, Query.PatternSize, &PspSetCreateProcessNotifyRoutine);

	if (!NT_SUCCESS(Status))
		return STATUS_NOT_FOUND;

	// Resolve the offset to get the address of PspCreateProcessNotifyRoutine.
	PspSetCreateProcessNotifyRoutine += Query.PatternOffset;
	PspSetCreateProcessNotifyRoutine += *(PLONG)(PspSetCreateProcessNotifyRoutine);
	PspSetCreateProcessNotifyRoutine += sizeof(LONG);

	// Copy the callback addresses to the provided array.
	for (int i = 0; i < MAX_CALLBACKS_CREATE_PROCESS; i++)
	{
		// Check if a pointer to the callback exists at this index of the registered callbacks array.
		PVOID PointerToCallback = (PVOID)(*(PULONG64)((ULONG64)PspSetCreateProcessNotifyRoutine + i * 8LL));

		// If so, copy the callback address to the output array.
		if (PointerToCallback)
		{
			CallbackArray[i].Type = CALLBACK_TYPE::PsProcessCreation;
			CallbackArray[i].Address = (PVOID)(*(PULONG64)((ULONG64)PointerToCallback & 0xFFFFFFFFFFFFFFF8));
		}

		// Otherwise, exit the loop as there are no more callbacks left.
		else
		{
			*CallbackCount = i;
			break;
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS GetCreateThreadNotifyCallbacks(OUT CALLBACK_INFO* CallbackArray, OUT ULONG* CallbackCount)
{
	// Check if supported for this version of Windows.
	WINDOWS_INDEX WindowsIndex = GetWindowsIndex();
	if (WindowsIndex == WINDOWS_INDEX::Unsupported)
		return STATUS_NOT_SUPPORTED;

	// Get search query.
	CALLBACK_SEARCH_QUERY Query = CreateThreadCallbackQueries[(ULONG)WindowsIndex];

	// Resolve function addresses to get the start and end of the search space.
	PUCHAR StartAddress = (PUCHAR)MmGetSystemRoutineAddress(&Query.StartFunction);
	PUCHAR EndAddress = (PUCHAR)MmGetSystemRoutineAddress(&Query.EndFunction);

	// Search for an offset to the PspCreateThreadNotifyRoutine array.
	PUCHAR PspCreateThreadNotifyRoutine = nullptr;
	NTSTATUS Status = MemorySearch(StartAddress, EndAddress, Query.PatternBuffer, Query.PatternSize, &PspCreateThreadNotifyRoutine);

	if (!NT_SUCCESS(Status))
		return STATUS_NOT_FOUND;

	// Resolve the offset to get the address of PspCreateThreadNotifyRoutine.
	PspCreateThreadNotifyRoutine += Query.PatternOffset;
	PspCreateThreadNotifyRoutine += *(PLONG)(PspCreateThreadNotifyRoutine);
	PspCreateThreadNotifyRoutine += sizeof(LONG);

	// Copy the callback addresses to the provided array.
	for (int i = 0; i < MAX_CALLBACKS_CREATE_THREAD; i++)
	{
		// Check if a pointer to the callback exists at this index of the registered callbacks array.
		PVOID PointerToCallback = (PVOID)(*(PULONG64)((ULONG64)PspCreateThreadNotifyRoutine + i * 8LL));

		// If so, copy the callback address to the output array.
		if (PointerToCallback)
		{
			CallbackArray[i].Type = CALLBACK_TYPE::PsThreadCreation;
			CallbackArray[i].Address = (PVOID)(*(PULONG64)((ULONG64)PointerToCallback & 0xFFFFFFFFFFFFFFF8));
		}

		// Otherwise, exit the loop as there are no more callbacks left.
		else
		{
			*CallbackCount = i;
			break;
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS GetRegistryCallbacks(OUT CALLBACK_INFO* CallbackArray, OUT ULONG* CallbackCount)
{
	// Check if supported for this version of Windows.
	WINDOWS_INDEX WindowsIndex = GetWindowsIndex();
	if (WindowsIndex == WINDOWS_INDEX::Unsupported)
		return STATUS_NOT_SUPPORTED;

	// Get search query.
	CALLBACK_SEARCH_QUERY Query = RegistryCallbackQueries[(ULONG)WindowsIndex];

	// Resolve function addresses to get the start and end of the search space.
	PUCHAR StartAddress = (PUCHAR)MmGetSystemRoutineAddress(&Query.StartFunction);
	PUCHAR EndAddress = (PUCHAR)MmGetSystemRoutineAddress(&Query.EndFunction);

	// Search for an offset to the CallbackListHeadOrCmpCallBackVector linked-list head.
	PUCHAR CallbackListHeadOrCmpCallBackVector = nullptr;
	NTSTATUS Status = MemorySearch(StartAddress, EndAddress, Query.PatternBuffer, Query.PatternSize, &CallbackListHeadOrCmpCallBackVector);

	if (!NT_SUCCESS(Status))
		return STATUS_NOT_FOUND;

	// Resolve the offset to get the address of CallbackListHeadOrCmpCallBackVector.
	CallbackListHeadOrCmpCallBackVector += Query.PatternOffset;
	CallbackListHeadOrCmpCallBackVector += *(PLONG)(CallbackListHeadOrCmpCallBackVector);
	CallbackListHeadOrCmpCallBackVector += sizeof(LONG);

	// Iterate through the linked list to populate the provided array.
	int i = 0;
	PLIST_ENTRY ListEntry = (PLIST_ENTRY)CallbackListHeadOrCmpCallBackVector;
	while (1)
	{
		PVOID CallbackFunction = *(PVOID*)((ULONG64)ListEntry + 0x28);
		CallbackArray[i].Type = CALLBACK_TYPE::CmRegistry;
		CallbackArray[i++].Address = CallbackFunction;
		ListEntry = ListEntry->Flink;

		// Break when back to first entry or array limit reached.
		if (((PVOID)ListEntry == (PVOID)CallbackListHeadOrCmpCallBackVector) || (i == MAX_CALLBACKS_REGISTRY))
		{
			*CallbackCount = i;
			break;
		}
	};

	return STATUS_SUCCESS;
}

NTSTATUS GetObjectCallbacks(OUT CALLBACK_INFO* CallbackArray, OUT ULONG* CallbackCount)
{
	// Reset callback count to 0.
	*CallbackCount = 0;

	// Check if supported for this version of Windows.
	WINDOWS_INDEX WindowsIndex = GetWindowsIndex();
	if (WindowsIndex == WINDOWS_INDEX::Unsupported)
		return STATUS_NOT_SUPPORTED;

	// Get search query.
	CALLBACK_SEARCH_QUERY Query = ObjectCallbackQueries[(ULONG)WindowsIndex];

	// Resolve function addresses to get the start and end of the search space.
	PUCHAR StartAddress = (PUCHAR)MmGetSystemRoutineAddress(&Query.StartFunction);
	PUCHAR EndAddress = (PUCHAR)MmGetSystemRoutineAddress(&Query.EndFunction);

	// Search for an offset to the ObpTypeDirectoryObject linked-list head.
	PUCHAR ObpTypeDirectoryObject = nullptr;
	NTSTATUS Status = MemorySearch(StartAddress, EndAddress, Query.PatternBuffer, Query.PatternSize, &ObpTypeDirectoryObject);
	
	if (!NT_SUCCESS(Status))
		return STATUS_NOT_FOUND;

	// Resolve the offset to get the address of CallbackListHeadOrCmpCallBackVector.
	ObpTypeDirectoryObject += Query.PatternOffset;
	ObpTypeDirectoryObject += *(PLONG)(ObpTypeDirectoryObject);
	ObpTypeDirectoryObject += sizeof(LONG);

	UNICODE_STRING TypesWhitelist[3] = {
		RTL_CONSTANT_STRING(L"Process"),
		RTL_CONSTANT_STRING(L"Thread"),
		RTL_CONSTANT_STRING(L"Desktop")};

	POBJECT_DIRECTORY ObjectDirectory = (POBJECT_DIRECTORY)ObpTypeDirectoryObject;
	POBJECT_DIRECTORY_ENTRY* ObjectDirectoryEntries = (POBJECT_DIRECTORY_ENTRY*)ObjectDirectory->HashBuckets[0];

	for (int i = 0; i < OBJECT_HASH_TABLE_SIZE; i++)
	{
		POBJECT_DIRECTORY_ENTRY ObjectDirectoryEntry = ObjectDirectoryEntries[i];

		while (ObjectDirectoryEntry)
		{
			PVOID ObjectType = ObjectDirectoryEntry->ObjectType;
			PUNICODE_STRING ObjectName = (PUNICODE_STRING)((ULONG64)ObjectType + 0x10);
			UCHAR Offset = (GetWindowsIndex() == WINDOWS_INDEX::W7_07600) ? 0xC0 : 0xC8;
			POBJECT_CALLBACK_ENTRY ObjectCallbackEntry = (POBJECT_CALLBACK_ENTRY)((ULONG64)ObjectType + Offset);
			
			OBJECT_CALLBACK_TYPE ObjectCallbackType;
			if (!RtlCompareUnicodeString(ObjectName, &TypesWhitelist[0], TRUE))
				ObjectCallbackType = OBJECT_CALLBACK_TYPE::Process;
			else if (!RtlCompareUnicodeString(ObjectName, &TypesWhitelist[1], TRUE))
				ObjectCallbackType = OBJECT_CALLBACK_TYPE::Thread;
			else if (!RtlCompareUnicodeString(ObjectName, &TypesWhitelist[2], TRUE))
				ObjectCallbackType = OBJECT_CALLBACK_TYPE::Desktop;
			else
				ObjectCallbackType = OBJECT_CALLBACK_TYPE::Unknown;

			if (ObjectCallbackType != OBJECT_CALLBACK_TYPE::Unknown)
			{
				do
				{
					if ((ObjectCallbackEntry->Active == 1) && (ObjectCallbackEntry->ObjectType))
					{
						if (ObjectCallbackEntry->PreOperation)
						{
							switch (ObjectCallbackType)
							{
							case OBJECT_CALLBACK_TYPE::Process:
								CallbackArray[*CallbackCount].Type = CALLBACK_TYPE::ObProcessHandlePre;
								break;
							case OBJECT_CALLBACK_TYPE::Thread:
								CallbackArray[*CallbackCount].Type = CALLBACK_TYPE::ObThreadHandlePre;
								break;
							case OBJECT_CALLBACK_TYPE::Desktop:
								CallbackArray[*CallbackCount].Type = CALLBACK_TYPE::ObDesktopHandlePre;
								break;
							default:
								CallbackArray[*CallbackCount].Type = CALLBACK_TYPE::Unknown;
								break;
							}

							CallbackArray[*CallbackCount].Address = ObjectCallbackEntry->PreOperation;
							*CallbackCount += 1;
						}

						if (ObjectCallbackEntry->PostOperation)
						{
							switch (ObjectCallbackType)
							{
							case OBJECT_CALLBACK_TYPE::Process:
								CallbackArray[*CallbackCount].Type = CALLBACK_TYPE::ObProcessHandlePost;
								break;
							case OBJECT_CALLBACK_TYPE::Thread:
								CallbackArray[*CallbackCount].Type = CALLBACK_TYPE::ObThreadHandlePost;
								break;
							case OBJECT_CALLBACK_TYPE::Desktop:
								CallbackArray[*CallbackCount].Type = CALLBACK_TYPE::ObDesktopHandlePost;
								break;
							default:
								CallbackArray[*CallbackCount].Type = CALLBACK_TYPE::Unknown;
								break;
							}

							CallbackArray[*CallbackCount].Address = ObjectCallbackEntry->PostOperation;
							*CallbackCount += 1;
						}
					}

					ObjectCallbackEntry = (POBJECT_CALLBACK_ENTRY)ObjectCallbackEntry->CallbackList.Flink;
				} while (ObjectCallbackEntry != (POBJECT_CALLBACK_ENTRY)((ULONG64)ObjectType + Offset));
			}
			
			ObjectDirectoryEntry = ObjectDirectoryEntry->ChainLink;
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS GetMinifilterCallbacks(OUT CALLBACK_INFO* CallbackArray, OUT ULONG* CallbackCount)
{
	UNREFERENCED_PARAMETER(CallbackArray);
	UNREFERENCED_PARAMETER(CallbackCount);

	NTSTATUS Status = STATUS_SUCCESS;

	// Reset callback count to 0.
	*CallbackCount = 0;

	ULONG NumberFiltersReturned = 0;
	Status = FltEnumerateFilters(nullptr, 0, &NumberFiltersReturned);
	if (Status != STATUS_BUFFER_TOO_SMALL)
		return Status;

	SIZE_T BufferSize = sizeof(PFLT_FILTER) * NumberFiltersReturned;
	PFLT_FILTER* FilterList = (PFLT_FILTER*)ExAllocatePoolWithTag(NonPagedPool, BufferSize, DRIVER_TAG);
	if (!FilterList)
		return STATUS_INSUFFICIENT_RESOURCES;

	Status = FltEnumerateFilters(FilterList, (ULONG)BufferSize, &NumberFiltersReturned);
	if (!NT_SUCCESS(Status))
		return Status;

	for (ULONG i = 0; i < NumberFiltersReturned; i++)
	{
		ULONG BytesReturned = 0;
		Status = FltGetFilterInformation(FilterList[i], FilterFullInformation, nullptr, 0, &BytesReturned);
		if (Status != STATUS_BUFFER_TOO_SMALL)
			continue;

		PFILTER_FULL_INFORMATION FullFilterInfo = (PFILTER_FULL_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, BytesReturned, DRIVER_TAG);
		if (!FullFilterInfo)
			continue;

		Status = FltGetFilterInformation(FilterList[i], FilterFullInformation, FullFilterInfo, BytesReturned, &BytesReturned);
		if (!NT_SUCCESS(Status))
			continue;

		ULONG NumberInstancesReturned = 0;
		Status = FltEnumerateInstances(nullptr, FilterList[i], nullptr, 0, &NumberInstancesReturned);
		if (Status != STATUS_BUFFER_TOO_SMALL)
			continue;

		BufferSize = sizeof(PFLT_INSTANCE) * NumberInstancesReturned;
		PFLT_INSTANCE* InstanceList = (PFLT_INSTANCE*)ExAllocatePoolWithTag(NonPagedPool, BufferSize, DRIVER_TAG);
		if (!InstanceList)
			continue;

		Status = FltEnumerateInstances(nullptr, FilterList[i], InstanceList, NumberInstancesReturned, &NumberInstancesReturned);
		if (!NT_SUCCESS(Status))
			continue;

		if (!NumberInstancesReturned)
			continue;

		for (ULONG j = 0x16; j < 0x32; j++)
		{
			// Hardcoding the offsets from https://github.com/gentilkiwi/mimikatz/blob/master/mimidrv/kkll_m_filters.c#L34.
			// This should probably be changed in the future.
			PVOID Callback = (PVOID)*(PULONG_PTR)((((ULONG_PTR)InstanceList[0]) + 0x90) + sizeof(PVOID) * j);

			if (Callback)
			{
				PVOID PreCallback = (PVOID) * (PULONG_PTR)(((ULONG_PTR)Callback) + 0x18);
				PVOID PostCallback = (PVOID) * (PULONG_PTR)(((ULONG_PTR)Callback) + 0x20);

				if (PreCallback)
				{
					CallbackArray[*CallbackCount].Type = (CALLBACK_TYPE)((j - 0x16) * 2 + 11);
					CallbackArray[*CallbackCount].Address = PreCallback;
					*CallbackCount += 1;
				}

				if (PostCallback)
				{
					CallbackArray[*CallbackCount].Type = (CALLBACK_TYPE)((j - 0x16) * 2 + 12);
					CallbackArray[*CallbackCount].Address = PostCallback;
					*CallbackCount += 1;
				}
			}
		}
	}

	return STATUS_SUCCESS;
}