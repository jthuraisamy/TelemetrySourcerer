#include <Windows.h>
#include <evntrace.h>
#include <tdh.h>

#include <set>
#include <string>
#include <vector>

#include "UmETW.h"

#pragma comment(lib, "Tdh.lib")

std::hash<std::wstring> HashString;
std::set<SIZE_T> NotableSessionHashes;
std::set<SIZE_T> NotableProviderHashes;

// Function:    PopulateUmeHashes
// Description: Populates a list of notable provider and session hashes so they are highlighted.
// Called from: GetSessions as a one-time call if not populated.
VOID PopulateUmeHashes()
{
	NotableProviderHashes.insert(HashString(wstring(L"Microsoft-Windows-Audit-CVE")));
	NotableProviderHashes.insert(HashString(wstring(L"Microsoft-Windows-Threat-Intelligence")));
	NotableProviderHashes.insert(HashString(wstring(L"Microsoft-Windows-Kernel-Process")));
	NotableProviderHashes.insert(HashString(wstring(L"Microsoft-Windows-Kernel-Network")));
	NotableProviderHashes.insert(HashString(wstring(L"Microsoft-Windows-Kernel-Registry")));
	NotableProviderHashes.insert(HashString(wstring(L"Microsoft-Windows-Kernel-File")));
	NotableProviderHashes.insert(HashString(wstring(L"Microsoft-Windows-WinINet")));
	NotableProviderHashes.insert(HashString(wstring(L"Microsoft-Windows-WinINet-Capture")));
	NotableProviderHashes.insert(HashString(wstring(L"Microsoft-Windows-DNS-Client")));
	NotableProviderHashes.insert(HashString(wstring(L"Microsoft-Windows-SMBClient")));
	NotableProviderHashes.insert(HashString(wstring(L"Microsoft-Windows-SMBServer")));
	NotableProviderHashes.insert(HashString(wstring(L"Microsoft-Windows-WMI-Activity")));
	NotableProviderHashes.insert(HashString(wstring(L"Microsoft-Windows-Sysmon")));

	NotableSessionHashes.insert(0x7CB510BA9B40BEEC);
	NotableSessionHashes.insert(0x0DD1D51CF3AADD14);
	NotableSessionHashes.insert(0x2199DD129071BD5A);
	NotableSessionHashes.insert(0x0EE38A4714DEDC86);
	NotableSessionHashes.insert(0xCD53ED4761EBB7B4);
	NotableSessionHashes.insert(0x301E4178666A34DB);
	NotableSessionHashes.insert(0x40B6B7B240F291E6);
	NotableSessionHashes.insert(0x8CAB419C02FF68DD);
	NotableSessionHashes.insert(0x1F5E4F79416EBBDF);
	NotableSessionHashes.insert(0x2D4F43EC18BBA6C4);
}

// Function:    GetSessions
// Description: Returns a list of ETW tracing sessions.
// Called from: UmeLoadResults
std::vector<PTRACING_SESSION> GetSessions()
{
	if (!NotableSessionHashes.size())
		PopulateUmeHashes();

	std::vector<PTRACING_SESSION> Sessions;

	// Allocate memory for EVENT_TRACE_PROPERTIES.
	ULONG PropertiesSize = sizeof(EVENT_TRACE_PROPERTIES) + (MAX_SESSION_NAME_LEN * sizeof(WCHAR)) + (MAX_LOGFILE_PATH_LEN * sizeof(WCHAR));
	ULONG BufferSize = PropertiesSize * MAX_SESSIONS;
	PEVENT_TRACE_PROPERTIES EtpBuffer = (PEVENT_TRACE_PROPERTIES)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BufferSize);
	PEVENT_TRACE_PROPERTIES EtpSessions[MAX_SESSIONS];

	for (USHORT i = 0; i < MAX_SESSIONS; i++)
	{
		EtpSessions[i] = (EVENT_TRACE_PROPERTIES*)((BYTE*)EtpBuffer + (i * PropertiesSize));
		EtpSessions[i]->Wnode.BufferSize = PropertiesSize;
		EtpSessions[i]->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
		EtpSessions[i]->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + (MAX_SESSION_NAME_LEN * sizeof(WCHAR));
	}

	ULONG SessionCount = 0;
	QueryAllTraces(EtpSessions, (ULONG)MAX_SESSIONS, &SessionCount);

	// Create TRACING_SESSION objects.
	for (USHORT i = 0; i < SessionCount; i++)
	{
		PTRACING_SESSION Session = new TRACING_SESSION;

		Session->LoggerId = (USHORT)EtpSessions[i]->Wnode.HistoricalContext;
		CopyMemory(Session->InstanceName, (LPWSTR)((char*)EtpSessions[i] + EtpSessions[i]->LoggerNameOffset), MAX_SESSION_NAME_LEN);

		if (NotableSessionHashes.count(HashString(std::wstring(Session->InstanceName))))
			Session->Notable = TRUE;

		Sessions.push_back(Session);
	}

	// Populate the TRACING_SESSION objects with providers enabled for them.
	PopulateSessionProviders(Sessions);

	HeapFree(GetProcessHeap(), NULL, EtpBuffer);
	return Sessions;
}

// Function:    PopulateSessionProviders
// Description: Populates a TRACING_SESSION object with its enabled providers.
// Called from: GetSessions
// Remarks:     Adapted from https://docs.microsoft.com/en-us/windows/win32/api/evntrace/nf-evntrace-enumeratetraceguidsex.
VOID PopulateSessionProviders(std::vector<PTRACING_SESSION> Sessions)
{
	PPROVIDER_ENUMERATION_INFO PeiBuffer = nullptr;
	ULONG PeiBufferSize = 0;

	// Get information on each provider (incl. name) used for querying later.
	if (TdhEnumerateProviders(PeiBuffer, &PeiBufferSize) == ERROR_INSUFFICIENT_BUFFER)
	{
		PeiBuffer = (PPROVIDER_ENUMERATION_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, PeiBufferSize);

		if (!PeiBuffer)
			return;

		if (TdhEnumerateProviders(PeiBuffer, &PeiBufferSize) != ERROR_SUCCESS)
			return;
	}

	// Query an array of GUIDs of the providers that are registered on the computer.
	LPGUID ProviderGuidList = (LPGUID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BUFSIZ);
	DWORD ProviderGuidListSize = BUFSIZ;
	DWORD RequiredListSize = 0;
	while (EnumerateTraceGuidsEx(TraceGuidQueryList, nullptr, 0, ProviderGuidList, ProviderGuidListSize, &RequiredListSize) == ERROR_INSUFFICIENT_BUFFER)
	{
		ProviderGuidListSize = RequiredListSize;
		ProviderGuidList = (LPGUID)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ProviderGuidList, RequiredListSize);
	}

	// Iterate through each provider to associate them with sessions.
	DWORD ProviderGuidCount = ProviderGuidListSize / sizeof(GUID);
	for (unsigned int i = 0; i < ProviderGuidCount; i++)
	{
		GUID ProviderGuid = ProviderGuidList[i];

		// Get information about trace providers using EnumerateTraceGuidsEx.
		PTRACE_GUID_INFO TraceGuidInfo = (PTRACE_GUID_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BUFSIZ);
		DWORD InfoListSize = BUFSIZ;
		DWORD RequiredListSize = 0;
		while (EnumerateTraceGuidsEx(TraceGuidQueryInfo, &ProviderGuid, sizeof(GUID), TraceGuidInfo, InfoListSize, &RequiredListSize) == ERROR_INSUFFICIENT_BUFFER)
		{
			InfoListSize = RequiredListSize;
			TraceGuidInfo = (PTRACE_GUID_INFO)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, TraceGuidInfo, RequiredListSize);
		}

		PTRACE_PROVIDER_INSTANCE_INFO TraceProviderInstanceInfo = (PTRACE_PROVIDER_INSTANCE_INFO)((PBYTE)TraceGuidInfo + sizeof(TRACE_GUID_INFO));
		for (unsigned int j = 0; j < TraceGuidInfo->InstanceCount; j++)
		{
			if (TraceProviderInstanceInfo->EnableCount > 0)
			{
				PTRACE_ENABLE_INFO TraceEnableInfo = (PTRACE_ENABLE_INFO)((PBYTE)TraceProviderInstanceInfo + sizeof(TRACE_PROVIDER_INSTANCE_INFO));
				for (unsigned int k = 0; k < TraceProviderInstanceInfo->EnableCount; k++)
				{
					USHORT LoggerId = TraceEnableInfo->LoggerId;

					for (PTRACING_SESSION TracingSession : Sessions)
					{
						if (TracingSession->LoggerId == LoggerId)
						{
							// There tends to be duplicates, so the logic below ensures that only
							// unique providers are added to the session.
							BOOL ShouldAddProvider = TRUE;
							for (PTRACE_PROVIDER TraceProvider : TracingSession->EnabledProviders)
								if (TraceProvider->ProviderId == ProviderGuid)
									ShouldAddProvider = FALSE;

							if (ShouldAddProvider)
							{
								PTRACE_PROVIDER tp = new TRACE_PROVIDER;

								tp->ProviderId = ProviderGuid;

								// Get the provider name.
								LPWSTR ProviderName = nullptr;
								for (unsigned int l = 0; l < PeiBuffer->NumberOfProviders; l++)
								{
									if (ProviderGuid == PeiBuffer->TraceProviderInfoArray[l].ProviderGuid)
									{
										ProviderName = (LPWSTR)((DWORD_PTR)PeiBuffer + PeiBuffer->TraceProviderInfoArray[l].ProviderNameOffset);
										CopyMemory(tp->ProviderName, ProviderName, MAX_PATH);
									}
								}

								// If provider name is not defined, use the GUID.
								if (!ProviderName)
									StringFromGUID2(ProviderGuid, tp->ProviderName, MAX_PATH);

								// Check if the provider name is notable (i.e. should be highlighted).
								if (NotableProviderHashes.count(HashString(std::wstring(tp->ProviderName))))
									tp->Notable = TRUE;

								TracingSession->EnabledProviders.push_back(tp);
							}
						}
					}

					TraceEnableInfo++;
				}
			}

			TraceProviderInstanceInfo = (PTRACE_PROVIDER_INSTANCE_INFO)((PBYTE)TraceProviderInstanceInfo + TraceProviderInstanceInfo->NextOffset);
		}
	}

	HeapFree(GetProcessHeap(), NULL, PeiBuffer);
	return;
}

// Function:    DisableProvider
// Description: Disables the provider for a session, given its logger ID and the provider's GUID.
// Called from: UmeDisableSelectedProvider
DWORD DisableProvider(USHORT LoggerId, LPCGUID ProviderGuid)
{
	return EnableTraceEx2(
		(TRACEHANDLE)LoggerId,
		ProviderGuid,
		EVENT_CONTROL_CODE_DISABLE_PROVIDER,
		TRACE_LEVEL_VERBOSE,
		NULL,
		NULL,
		NULL,
		nullptr);
}

// Function:    StopTracingSession
// Description: Stops a tracing session, given its logger ID.
// Called from: UmeStopTracingSession
DWORD StopTracingSession(USHORT LoggerId)
{
	ULONG PropertiesSize = sizeof(EVENT_TRACE_PROPERTIES) + (MAX_SESSION_NAME_LEN * sizeof(WCHAR)) + (MAX_LOGFILE_PATH_LEN * sizeof(WCHAR));
	PEVENT_TRACE_PROPERTIES EventTraceProperties = (PEVENT_TRACE_PROPERTIES)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, PropertiesSize);
	EventTraceProperties->Wnode.BufferSize = PropertiesSize;
	EventTraceProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
	EventTraceProperties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + (MAX_SESSION_NAME_LEN * sizeof(WCHAR));

	DWORD Status = StopTraceW((TRACEHANDLE)LoggerId, nullptr, EventTraceProperties);

	HeapFree(GetProcessHeap(), NULL, EventTraceProperties);
	return Status;
}