#pragma once

#include "Common.h"

#define MAX_CALLBACKS_LOAD_IMAGE     64
#define MAX_CALLBACKS_CREATE_PROCESS 64
#define MAX_CALLBACKS_CREATE_THREAD  64
#define MAX_CALLBACKS_REGISTRY       100
#define OBJECT_HASH_TABLE_SIZE	     37

enum class WINDOWS_INDEX
{
	W7_07600 = 0,
	W8_08102,
	W8_09431,
	WX_10240, // Win10 1507
	WX_10586, // Win10 1511
	WX_14393, // Win10 1607
	WX_15063, // Win10 1703
	WX_16299, // Win10 1709
	WX_17134, // Win10 1803
	WX_17763, // Win10 1809
	WX_18362, // Win10 1903
	WX_18363, // Win10 1909
	WX_19041, // Win10 2004
	Unsupported
};

enum class OBJECT_CALLBACK_TYPE
{
	Unknown = 0,
	Process,
	Thread,
	Desktop
};

typedef struct _OBJECT_DIRECTORY_ENTRY
{
	struct _OBJECT_DIRECTORY_ENTRY* ChainLink;
	PVOID                           ObjectType;
} OBJECT_DIRECTORY_ENTRY, *POBJECT_DIRECTORY_ENTRY;

typedef struct _OBJECT_DIRECTORY
{
	POBJECT_DIRECTORY_ENTRY HashBuckets[OBJECT_HASH_TABLE_SIZE];
	/* ... */
} OBJECT_DIRECTORY, *POBJECT_DIRECTORY;

typedef struct _OBJECT_CALLBACK_ENTRY
{
	LIST_ENTRY                  CallbackList;
	OB_OPERATION                Operations;
	ULONG                       Active;
	PVOID                       Handle;
	POBJECT_TYPE                ObjectType;
	POB_PRE_OPERATION_CALLBACK  PreOperation;
	POB_POST_OPERATION_CALLBACK PostOperation;
	/* ... */
} OBJECT_CALLBACK_ENTRY, *POBJECT_CALLBACK_ENTRY;

typedef struct _CALLBACK_SEARCH_QUERY
{
	WINDOWS_INDEX  WindowsIndex;
	UNICODE_STRING StartFunction;
	UNICODE_STRING EndFunction;
	PUCHAR         PatternBuffer;
	SIZE_T         PatternSize;
	LONG           PatternOffset;
} CALLBACK_SEARCH_QUERY, *PCALLBACK_SEARCH_QUERY;

NTSTATUS GetLoadImageNotifyCallbacks(OUT CALLBACK_INFO* CallbackArray, OUT ULONG* CallbackCount);
NTSTATUS GetCreateProcessNotifyCallbacks(OUT CALLBACK_INFO* CallbackArray, OUT ULONG* CallbackCount);
NTSTATUS GetCreateThreadNotifyCallbacks(OUT CALLBACK_INFO* CallbackArray, OUT ULONG* CallbackCount);
NTSTATUS GetRegistryCallbacks(OUT CALLBACK_INFO* CallbackArray, OUT ULONG* CallbackCount);
NTSTATUS GetObjectCallbacks(OUT CALLBACK_INFO* CallbackArray, OUT ULONG* CallbackCount);
NTSTATUS GetMinifilterCallbacks(OUT CALLBACK_INFO* CallbackArray, OUT ULONG* CallbackCount);
