#include <ntddk.h>
#include <intrin.h>

#include "Memory.h"

#pragma intrinsic(__readmsr)

NTSTATUS MemorySearch(PCUCHAR StartAddress, PCUCHAR EndAddress, PCUCHAR PatternBuffer, SIZE_T PatternLength, PUCHAR* FoundAddress)
{
	*FoundAddress = (PUCHAR)StartAddress;

	while (*FoundAddress < EndAddress)
	{
		if (RtlEqualMemory(PatternBuffer, *FoundAddress, PatternLength))
			return STATUS_SUCCESS;
		else
			*FoundAddress += 1;
	}

	return STATUS_NOT_FOUND;
}

NTSTATUS SetWriteProtect(ULONG Enable)
{
	for (ULONG i = 0; i < KeQueryActiveProcessorCount(0); i++)
	{
		KAFFINITY OldAffinity = KeSetSystemAffinityThreadEx((KAFFINITY)(1i64 << i));

		CR0 cr0;
		cr0.flags = __readcr0();
		cr0.write_protect = (Enable) ? 1 : 0;
		__writecr0(cr0.flags);

		KeRevertToUserAffinityThreadEx(OldAffinity);
	}

	return STATUS_SUCCESS;
}

NTSTATUS WriteVirtualMemory(PVOID Destination, PVOID Source, SIZE_T Length)
{
	PHYSICAL_ADDRESS PhysicalAddress = MmGetPhysicalAddress(Destination);
	PVOID MappedDestination = MmMapIoSpace(PhysicalAddress, Length, MmNonCached);
	RtlCopyMemory(MappedDestination, Source, Length);
	MmUnmapIoSpace(MappedDestination, Length);

	return STATUS_SUCCESS;
}