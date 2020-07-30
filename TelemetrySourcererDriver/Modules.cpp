#include <ntddk.h>
#include <aux_klib.h>

#include "TelemetrySourcererDriver.h"
#include "Modules.h"

#pragma comment(lib, "Aux_Klib.lib")

NTSTATUS GetModules(OUT MODULE_INFO* ModuleArray)
{
	NTSTATUS Status = STATUS_SUCCESS;

	Status = AuxKlibInitialize();
	if (!NT_SUCCESS(Status))
		return Status;

	ULONG ModulesBufferSize = 0;
	Status = AuxKlibQueryModuleInformation(&ModulesBufferSize, sizeof(AUX_MODULE_EXTENDED_INFO), nullptr);
	if (!NT_SUCCESS(Status))
		return Status;

	PAUX_MODULE_EXTENDED_INFO ModuleExtendedInfo = (PAUX_MODULE_EXTENDED_INFO)ExAllocatePoolWithTag(PagedPool, ModulesBufferSize, DRIVER_TAG);
	if (!ModuleExtendedInfo)
		return STATUS_INSUFFICIENT_RESOURCES;

	Status = AuxKlibQueryModuleInformation(&ModulesBufferSize, sizeof(AUX_MODULE_EXTENDED_INFO), ModuleExtendedInfo);
	if (!NT_SUCCESS(Status))
	{
		ExFreePoolWithTag(ModuleExtendedInfo, DRIVER_TAG);
		return Status;
	}

	ULONG ModuleCount = ModulesBufferSize / sizeof(AUX_MODULE_EXTENDED_INFO);
	for (unsigned long i = 0; i < ModuleCount; i++)
	{
		ModuleArray[i].Address = ModuleExtendedInfo[i].BasicInfo.ImageBase;
		ModuleArray[i].Size = ModuleExtendedInfo[i].ImageSize;
		RtlCopyMemory(ModuleArray[i].Name, ModuleExtendedInfo[i].FullPathName + ModuleExtendedInfo[i].FileNameOffset, MAXIMUM_FILENAME_LENGTH);
		KdPrint(("> TelemetrySourcererDriver: Getting modules... -> %s (0x%p; 0x%p)\n", ModuleArray[i].Name, ModuleArray[i].Address, ModuleArray[i].Size));
	}
	
	ExFreePoolWithTag(ModuleExtendedInfo, DRIVER_TAG);
	return Status;
}