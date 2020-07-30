#pragma once

typedef struct _MEMORY_SEARCH_QUERY {
	ULONG                 OsIndex;
	MEMORY_SEARCH_NEEDLE  Needle;
	PWCHAR                HaystackStartFunction;
	PWCHAR                HaystackEndFunction;
	MEMORY_SEARCH_OFFSETS Offsets;
} MEMORY_SEARCH_QUERY, *PMEMORY_SEARCH_QUERY;

#pragma warning(disable:4201)
typedef union {
	struct {
		UINT64 protection_enable   : 1;
		UINT64 monitor_coprocessor : 1;
		UINT64 emulate_fpu         : 1;
		UINT64 task_switched       : 1;
		UINT64 extension_type      : 1;
		UINT64 numeric_error       : 1;
		UINT64 reserved_1          : 10;
		UINT64 write_protect       : 1;
		UINT64 reserved_2          : 1;
		UINT64 alignment_mask      : 1;
		UINT64 reserved_3          : 10;
		UINT64 not_write_through   : 1;
		UINT64 cache_disable       : 1;
		UINT64 paging_enable       : 1;
	};

	UINT64 flags;
} CR0;

NTSTATUS MemorySearch(PCUCHAR StartAddress, PCUCHAR EndAddress, PCUCHAR PatternBuffer, SIZE_T PatternLength, PUCHAR* FoundAddress);
NTSTATUS SetWriteProtect(ULONG Enable);
NTSTATUS WriteVirtualMemory(PVOID Destination, PVOID Source, SIZE_T Length);