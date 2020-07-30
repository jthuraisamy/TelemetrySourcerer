#pragma once

#include <Windows.h>
#include <evntrace.h>

#include <vector>

using namespace std;

#define MAX_SESSIONS 64
#define MAX_SESSION_NAME_LEN 1024
#define MAX_LOGFILE_PATH_LEN 1024

// Struct:    TRACE_PROVIDER
// Describes: An ETW trace provider.
// Members:
// - ProviderId:   GUID of the provider.
// - ProviderName: Name of the provider.
// - Notable:      Whether the provider should be highlighted.
typedef struct _TRACE_PROVIDER
{
	GUID  ProviderId             = { 0 };
	WCHAR ProviderName[MAX_PATH] = { 0 };
	BOOL  Notable                = FALSE;
} TRACE_PROVIDER, *PTRACE_PROVIDER;

// Struct:    TRACING_SESSION
// Describes: An ETW tracing session.
// Members:
// - LoggerId:         GUID of the session.
// - InstanceName:     Name of the session.
// - EnabledProviders: List of providers enabled for the session.
// - Notable:          Whether the session should be highlighted.
typedef struct _TRACING_SESSION
{
	USHORT                       LoggerId                           = 0;
	WCHAR                        InstanceName[MAX_SESSION_NAME_LEN] = { 0 };
	std::vector<PTRACE_PROVIDER> EnabledProviders;
	BOOL                         Notable                            = FALSE;
} TRACING_SESSION, *PTRACING_SESSION;

VOID PopulateUmeHashes();
std::vector<PTRACING_SESSION> GetSessions();
VOID PopulateSessionProviders(std::vector<PTRACING_SESSION> Sessions);
DWORD DisableProvider(USHORT LoggerId, LPCGUID ProviderGuid);
DWORD StopTracingSession(USHORT LoggerId);
