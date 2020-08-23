#pragma once

#include <Windows.h>

#define TOOL_NAME L"Telemetry Sourcerer"
#define VERSION   L"0.9.1"

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK KmcWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK UmhWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK UmeWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

VOID PaintWindow(HWND);
VOID ResizeWindow(HWND);

VOID KmcLoadResults();
VOID KmcSuppressCallback();
VOID KmcRevertCallback();
int CALLBACK KmcCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

VOID UmhLoadResults();
VOID UmhRestoreFunction();
VOID UmhLoadDll();
int CALLBACK UmhCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

VOID UmeLoadResults();
VOID UmeDisableSelectedProvider();
VOID UmeStopTracingSession();
int CALLBACK UmeCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
