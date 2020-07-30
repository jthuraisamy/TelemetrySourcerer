#pragma once

#define TELEMETRY_SOURCERER_DEVICE 0x8000
#define IOCTL_SANDBOX              CTL_CODE(TELEMETRY_SOURCERER_DEVICE, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_MODULES          CTL_CODE(TELEMETRY_SOURCERER_DEVICE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_CALLBACKS        CTL_CODE(TELEMETRY_SOURCERER_DEVICE, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_QWORD            CTL_CODE(TELEMETRY_SOURCERER_DEVICE, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SET_QWORD            CTL_CODE(TELEMETRY_SOURCERER_DEVICE, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define MAX_MODULES   512
#define MAX_CALLBACKS 512

typedef struct _MODULE_INFO
{
	UCHAR Name[256];
	PVOID Address;
	ULONG Size;
} MODULE_INFO, *PMODULE_INFO;

enum class CALLBACK_TYPE
{
	Unknown = 0,
	PsLoadImage,
	PsProcessCreation,
	PsThreadCreation,
	CmRegistry,
	ObProcessHandlePre,
	ObProcessHandlePost,
	ObThreadHandlePre,
	ObThreadHandlePost,
	ObDesktopHandlePre,
	ObDesktopHandlePost,
	MfCreatePre,
	MfCreatePost,
	MfCreateNamedPipePre,
	MfCreateNamedPipePost,
	MfClosePre,
	MfClosePost,
	MfReadPre,
	MfReadPost,
	MfWritePre,
	MfWritePost,
	MfQueryInformationPre,
	MfQueryInformationPost,
	MfSetInformationPre,
	MfSetInformationPost,
	MfQueryEaPre,
	MfQueryEaPost,
	MfSetEaPre,
	MfSetEaPost,
	MfFlushBuffersPre,
	MfFlushBuffersPost,
	MfQueryVolumeInformationPre,
	MfQueryVolumeInformationPost,
	MfSetVolumeInformationPre,
	MfSetVolumeInformationPost,
	MfDirectoryControlPre,
	MfDirectoryControlPost,
	MfFileSystemControlPre,
	MfFileSystemControlPost,
	MfDeviceControlPre,
	MfDeviceControlPost,
	MfInternalDeviceControlPre,
	MfInternalDeviceControlPost,
	MfShutdownPre,
	MfShutdownPost,
	MfLockControlPre,
	MfLockControlPost,
	MfCleanupPre,
	MfCleanupPost,
	MfCreateMailslotPre,
	MfCreateMailslotPost,
	MfQuerySecurityPre,
	MfQuerySecurityPost,
	MfSetSecurityPre,
	MfSetSecurityPost,
	MfPowerPre,
	MfPowerPost,
	MfSystemControlPre,
	MfSystemControlPost,
	MfDeviceChangePre,
	MfDeviceChangePost,
	MfQueryQuotaPre,
	MfQueryQuotaPost,
	MfSetQuotaPre,
	MfSetQuotaPost,
	MfPnpPre,
	MfPnpPost,
};

typedef struct _CALLBACK_INFO
{
	CALLBACK_TYPE Type    = CALLBACK_TYPE::Unknown;
	PVOID         Address = nullptr;
} CALLBACK_INFO, *PCALLBACK_INFO;

typedef struct _QWORD_INFO
{
	PULONG64 Address = nullptr;
	ULONG64  Value   = 0;
} QWORD_INFO, *PQWORD_INFO;