#pragma once

#include <Windows.h>

#define VERSION L"0.9.0"

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK KmcWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK UmhWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK UmeWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

VOID PaintWindow(HWND);
VOID ResizeWindow(HWND);

VOID KmcLoadResults();
VOID KmcSuppressCallback();
VOID KmcRevertCallback();

VOID UmhLoadResults();
VOID UmhRestoreFunction();
VOID UmhLoadDll();

VOID UmeLoadResults();
VOID UmeDisableSelectedProvider();
VOID UmeStopTracingSession();
