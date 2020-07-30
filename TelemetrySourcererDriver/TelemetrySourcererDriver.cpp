#include <ntddk.h>

#include "TelemetrySourcererDriver.h"
#include "Common.h"
#include "Memory.h"
#include "Modules.h"
#include "Callbacks.h"

extern "C" NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS Status = STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = UnloadDriver;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;

	// Create device.
	PDEVICE_OBJECT DeviceObject;
	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(LR"(\Device\TelemetrySourcererDriver)");
	Status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
	if (NT_SUCCESS(Status))
	{
		DeviceObject->Flags |= DO_BUFFERED_IO;
	}
	else
	{
		DbgPrint("> TelemetrySourcererDriver: Failed to create device (0x%08X).\n", Status);
		return Status;
	}

	// Create symbolic link for device.
	UNICODE_STRING SymbolicLink = RTL_CONSTANT_STRING(LR"(\??\TelemetrySourcererDriver)");
	Status = IoCreateSymbolicLink(&SymbolicLink, &DeviceName);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("> TelemetrySourcererDriver: Failed to create symbolic link (0x%08X).\n", Status);
		IoDeleteDevice(DeviceObject);
		return Status;
	}

	DbgPrint("> TelemetrySourcererDriver: Compiled on %s %s.\n", __DATE__, __TIME__);
	DbgPrint("> TelemetrySourcererDriver: Initialized successfully.\n");
	return Status;
}

void UnloadDriver(_In_ PDRIVER_OBJECT DriverObject)
{
	// Delete symbolic link and device.
	UNICODE_STRING SymbolicLink = RTL_CONSTANT_STRING(LR"(\??\TelemetrySourcererDriver)");
	IoDeleteSymbolicLink(&SymbolicLink);
	IoDeleteDevice(DriverObject->DeviceObject);

	DbgPrint("> TelemetrySourcererDriver: Unloaded successfully.\n");
}

_Use_decl_annotations_
NTSTATUS DispatchCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS DispatchDeviceControl(PDEVICE_OBJECT, PIRP Irp)
{
	NTSTATUS Status = STATUS_SUCCESS;
	ULONG OutBufferSize = 0;
	
	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);

	switch (Stack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_SANDBOX:
	{
		DbgPrint("> TelemetrySourcererDriver: Hello, world!\n");
		break;
	}
	case IOCTL_GET_MODULES:
	{
		DbgPrint("> TelemetrySourcererDriver: Getting modules...\n");
		OutBufferSize = Stack->Parameters.DeviceIoControl.InputBufferLength;
		MODULE_INFO* Modules = (MODULE_INFO*)Irp->AssociatedIrp.SystemBuffer;
		Status = GetModules(Modules);
		break;
	}
	case IOCTL_GET_CALLBACKS:
	{
		ULONG i = 0;
		ULONG n = 0;
		CALLBACK_INFO* Callbacks = (CALLBACK_INFO*)Irp->AssociatedIrp.SystemBuffer;
		OutBufferSize = Stack->Parameters.DeviceIoControl.InputBufferLength;

		// Get load image notification callbacks and update the output index.
		DbgPrint("> TelemetrySourcererDriver: Getting load image notification callbacks...\n");
		if (NT_SUCCESS(GetLoadImageNotifyCallbacks(&Callbacks[i], &n)))
			i += n;				

		// Get create process notification callbacks and update the output index.
		DbgPrint("> TelemetrySourcererDriver: Getting create process notification callbacks...\n");
		if (NT_SUCCESS(GetCreateProcessNotifyCallbacks(&Callbacks[i], &n)))
			i += n;

		// Get create thread notification callbacks and update the output index.
		DbgPrint("> TelemetrySourcererDriver: Getting create thread notification callbacks...\n");
		if (NT_SUCCESS(GetCreateThreadNotifyCallbacks(&Callbacks[i], &n)))
			i += n;

		// Get registry callbacks and and update the output index.
		DbgPrint("> TelemetrySourcererDriver: Getting registry notification callbacks...\n");
		if (NT_SUCCESS(GetRegistryCallbacks(&Callbacks[i], &n)))
			i += n;

		// Get object callbacks and and update the output index.
		DbgPrint("> TelemetrySourcererDriver: Getting object notification callbacks...\n");
		if (NT_SUCCESS(GetObjectCallbacks(&Callbacks[i], &n)))
			i += n;

		// Get minifilter callbacks and and update the output index.
		DbgPrint("> TelemetrySourcererDriver: Getting minifilter notification callbacks...\n");
		if (NT_SUCCESS(GetMinifilterCallbacks(&Callbacks[i], &n)))
			i += n;

		break;
	}
	case IOCTL_GET_QWORD:
	{
		OutBufferSize = Stack->Parameters.DeviceIoControl.InputBufferLength;
		QWORD_INFO* Buffer = (QWORD_INFO*)Irp->AssociatedIrp.SystemBuffer;
		KdPrint(("> TelemetrySourcererDriver: Getting QWORD at 0x%p...\n", Buffer->Address));
		Buffer->Value = *Buffer->Address;
		break;
	}
	case IOCTL_SET_QWORD:
	{
		QWORD_INFO* Buffer = (QWORD_INFO*)Irp->AssociatedIrp.SystemBuffer;
		KdPrint(("> TelemetrySourcererDriver: Setting QWORD at 0x%p...\n", Buffer->Address));
		WriteVirtualMemory(Buffer->Address, &Buffer->Value, sizeof(ULONG64));
		break;
	}
	default:
	{
		Status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	}

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = OutBufferSize;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}