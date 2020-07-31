#include <Windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <commctrl.h>
#include <windowsx.h>
#include <strsafe.h>
#include <TlHelp32.h>

#include <string>

#include "TelemetrySourcerer.h"
#include "KmCallbacks.h"
#include "UmHooks.h"
#include "UmETW.h"

using namespace std;

// Global variables
HINSTANCE g_hInst;

struct WINDOW_HANDLES {
    HWND Main;
    HWND StatusBar;
    HWND TabControl;

    // Kernel-mode Callbacks
    HWND KmcPage;
    HWND KmcRefreshButton;
    HWND KmcSuppressButton;
    HWND KmcRevertButton;
    HWND KmcCountLabel;
    HWND KmcTipLabel;
    HWND KmcListView;

    // User-mode Hooks
    HWND UmhPage;
    HWND UmhRefreshButton;
    HWND UmhRestoreButton;
    HWND UmhLoadButton;
    HWND UmhCountLabel;
    HWND UmhTipLabel;
    HWND UmhListView;

    // User-mode ETW Traces
    HWND UmePage;
    HWND UmeRefreshButton;
    HWND UmeDisableButton;
    HWND UmeStopButton;
    HWND UmeCountLabel;
    HWND UmeTipLabel;
    HWND UmeListView;

    // About
    HWND AbtPage;
    HWND AbtLabel;
} wh;

struct WINDOW_DATA {
    std::vector<PCALLBACK_ENTRY> KmcCallbacks;
    std::vector<PLOADED_MODULE> UmhModules;
    std::vector<PTRACING_SESSION> UmeSessions;
} wd;

// Function:    WinMain
// Description: Entry point for the application.
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // The main window class name.
    static TCHAR szWindowClass[] = _T("DesktopApp");

    // The string that appears in the application's title bar.
    WCHAR szTitle[MAX_PATH] = { 0 };
    StringCbPrintfW(szTitle, MAX_PATH, L"%s v%s by @Jackson_T (%s %s)", TOOL_NAME, VERSION, _T(__DATE__), _T(__TIME__));

    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = MainWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    RegisterClassEx(&wcex);

    // Store instance handle in our global variable.
    g_hInst = hInstance;

    // Create the main window and show it.
    wh.Main = CreateWindowEx(
        NULL,
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        900, 500,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    ShowWindow(wh.Main, nCmdShow);
    UpdateWindow(wh.Main);

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// Function:    MainWndProc
// Description: Processes messages for the main window.
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;

    switch (message)
    {
    case WM_CREATE:
    {
        PaintWindow(hWnd);

        KmcLoadResults();
        UmhLoadResults();
        UmeLoadResults();
        
        ShowWindow(wh.KmcPage, SW_SHOW);
        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        break;
    }
    case WM_GETMINMAXINFO:
    {
        lpMMI->ptMinTrackSize.x = 900; // Set minimum dimensions to 900x500.
        lpMMI->ptMinTrackSize.y = 500;
        lpMMI->ptMaxTrackSize.x = 900; // Set maximum dimensions to 900x500.
        lpMMI->ptMaxTrackSize.y = 500;
        break;
    }
    case WM_SIZE:
    {
        ResizeWindow(hWnd);
        break;
    }
    case WM_NOTIFY:
    {
        switch (((LPNMHDR)lParam)->code)
        {
        case TCN_SELCHANGE:
        {
            DWORD TabIndex = TabCtrl_GetCurSel(wh.TabControl);
            ShowWindow(wh.KmcPage, (TabIndex == 0) ? SW_SHOW : SW_HIDE);
            ShowWindow(wh.UmhPage, (TabIndex == 1) ? SW_SHOW : SW_HIDE);
            ShowWindow(wh.UmePage, (TabIndex == 2) ? SW_SHOW : SW_HIDE);
            ShowWindow(wh.AbtPage, (TabIndex == 3) ? SW_SHOW : SW_HIDE);
            break;
        }
        }
        break;
    }
    case WM_CLOSE:
        UnloadDriver();
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}

// Function:    KmcWndProc
// Description: Processes messages for the kernel-mode callbacks tab.
LRESULT CALLBACK KmcWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        switch (HIWORD(wParam))
        {
        case BN_CLICKED:
        {
            if (wh.KmcRefreshButton == (HWND)lParam)
                KmcLoadResults();
            else if (wh.KmcSuppressButton == (HWND)lParam)
                KmcSuppressCallback();
            else if (wh.KmcRevertButton == (HWND)lParam)
                KmcRevertCallback();
            break;
        }
        }
        break;
    }
    case WM_NOTIFY:
    {
        NMLVDISPINFO* plvdi;

        switch (((LPNMHDR)lParam)->code)
        {
        case LVN_GETDISPINFO:
        {
            plvdi = (NMLVDISPINFO*)lParam;
            PCALLBACK_ENTRY Callback = wd.KmcCallbacks.at(plvdi->item.lParam);
            LPWSTR Module = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH);
            StringCbPrintfW(Module, MAX_PATH, L"%ls + 0x%x", Callback->ModuleName, Callback->ModuleOffset);

            switch (plvdi->item.iSubItem)
            {
            case 0:
            {
                switch (Callback->Type)
                {
                case CALLBACK_TYPE::PsLoadImage:
                    plvdi->item.pszText = (LPWSTR)L"Image Load";
                    break;
                case CALLBACK_TYPE::PsProcessCreation:
                    plvdi->item.pszText = (LPWSTR)L"Process Creation";
                    break;
                case CALLBACK_TYPE::PsThreadCreation:
                    plvdi->item.pszText = (LPWSTR)L"Thread Creation";
                    break;
                case CALLBACK_TYPE::CmRegistry:
                    plvdi->item.pszText = (LPWSTR)L"Registry";
                    break;
                case CALLBACK_TYPE::ObProcessHandlePre:
                case CALLBACK_TYPE::ObProcessHandlePost:
                case CALLBACK_TYPE::ObThreadHandlePre:
                case CALLBACK_TYPE::ObThreadHandlePost:
                case CALLBACK_TYPE::ObDesktopHandlePre:
                case CALLBACK_TYPE::ObDesktopHandlePost:
                    plvdi->item.pszText = (LPWSTR)L"Object Handle";
                    break;
                case CALLBACK_TYPE::MfCreatePre:
                case CALLBACK_TYPE::MfCreatePost:
                case CALLBACK_TYPE::MfCreateNamedPipePre:
                case CALLBACK_TYPE::MfCreateNamedPipePost:
                case CALLBACK_TYPE::MfClosePre:
                case CALLBACK_TYPE::MfClosePost:
                case CALLBACK_TYPE::MfReadPre:
                case CALLBACK_TYPE::MfReadPost:
                case CALLBACK_TYPE::MfWritePre:
                case CALLBACK_TYPE::MfWritePost:
                case CALLBACK_TYPE::MfQueryInformationPre:
                case CALLBACK_TYPE::MfQueryInformationPost:
                case CALLBACK_TYPE::MfSetInformationPre:
                case CALLBACK_TYPE::MfSetInformationPost:
                case CALLBACK_TYPE::MfQueryEaPre:
                case CALLBACK_TYPE::MfQueryEaPost:
                case CALLBACK_TYPE::MfSetEaPre:
                case CALLBACK_TYPE::MfSetEaPost:
                case CALLBACK_TYPE::MfFlushBuffersPre:
                case CALLBACK_TYPE::MfFlushBuffersPost:
                case CALLBACK_TYPE::MfQueryVolumeInformationPre:
                case CALLBACK_TYPE::MfQueryVolumeInformationPost:
                case CALLBACK_TYPE::MfSetVolumeInformationPre:
                case CALLBACK_TYPE::MfSetVolumeInformationPost:
                case CALLBACK_TYPE::MfDirectoryControlPre:
                case CALLBACK_TYPE::MfDirectoryControlPost:
                case CALLBACK_TYPE::MfFileSystemControlPre:
                case CALLBACK_TYPE::MfFileSystemControlPost:
                case CALLBACK_TYPE::MfDeviceControlPre:
                case CALLBACK_TYPE::MfDeviceControlPost:
                case CALLBACK_TYPE::MfInternalDeviceControlPre:
                case CALLBACK_TYPE::MfInternalDeviceControlPost:
                case CALLBACK_TYPE::MfShutdownPre:
                case CALLBACK_TYPE::MfShutdownPost:
                case CALLBACK_TYPE::MfLockControlPre:
                case CALLBACK_TYPE::MfLockControlPost:
                case CALLBACK_TYPE::MfCleanupPre:
                case CALLBACK_TYPE::MfCleanupPost:
                case CALLBACK_TYPE::MfCreateMailslotPre:
                case CALLBACK_TYPE::MfCreateMailslotPost:
                case CALLBACK_TYPE::MfQuerySecurityPre:
                case CALLBACK_TYPE::MfQuerySecurityPost:
                case CALLBACK_TYPE::MfSetSecurityPre:
                case CALLBACK_TYPE::MfSetSecurityPost:
                case CALLBACK_TYPE::MfPowerPre:
                case CALLBACK_TYPE::MfPowerPost:
                case CALLBACK_TYPE::MfSystemControlPre:
                case CALLBACK_TYPE::MfSystemControlPost:
                case CALLBACK_TYPE::MfDeviceChangePre:
                case CALLBACK_TYPE::MfDeviceChangePost:
                case CALLBACK_TYPE::MfQueryQuotaPre:
                case CALLBACK_TYPE::MfQueryQuotaPost:
                case CALLBACK_TYPE::MfSetQuotaPre:
                case CALLBACK_TYPE::MfSetQuotaPost:
                case CALLBACK_TYPE::MfPnpPre:
                case CALLBACK_TYPE::MfPnpPost:
                    plvdi->item.pszText = (LPWSTR)L"File System";
                    break;
                default:
                    plvdi->item.pszText = (LPWSTR)L"Unknown";
                    break;
                }
                break;
            }
            case 1:
            {
                switch (Callback->Type)
                {
                case CALLBACK_TYPE::PsLoadImage:
                    plvdi->item.pszText = (LPWSTR)L"PsSetLoadImageNotifyRoutine";
                    break;
                case CALLBACK_TYPE::PsProcessCreation:
                    plvdi->item.pszText = (LPWSTR)L"PsSetCreateProcessNotifyRoutine";
                    break;
                case CALLBACK_TYPE::PsThreadCreation:
                    plvdi->item.pszText = (LPWSTR)L"PsSetCreateThreadNotifyRoutine";
                    break;
                case CALLBACK_TYPE::CmRegistry:
                    plvdi->item.pszText = (LPWSTR)L"CmRegisterCallbackEx";
                    break;
                case CALLBACK_TYPE::ObProcessHandlePre:
                    plvdi->item.pszText = (LPWSTR)L"PsProcessType (pre)";
                    break;
                case CALLBACK_TYPE::ObProcessHandlePost:
                    plvdi->item.pszText = (LPWSTR)L"PsProcessType (post)";
                    break;
                case CALLBACK_TYPE::ObThreadHandlePre:
                    plvdi->item.pszText = (LPWSTR)L"PsThreadType (pre)";
                    break;
                case CALLBACK_TYPE::ObThreadHandlePost:
                    plvdi->item.pszText = (LPWSTR)L"PsThreadType (post)";
                    break;
                case CALLBACK_TYPE::ObDesktopHandlePre:
                    plvdi->item.pszText = (LPWSTR)L"ExDesktopObjectType (pre)";
                    break;
                case CALLBACK_TYPE::ObDesktopHandlePost:
                    plvdi->item.pszText = (LPWSTR)L"ExDesktopObjectType (post)";
                    break;
                case CALLBACK_TYPE::MfCreatePre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_CREATE (pre)";
                    break;
                case CALLBACK_TYPE::MfCreatePost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_CREATE (post)";
                    break;
                case CALLBACK_TYPE::MfCreateNamedPipePre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_CREATE_NAMED_PIPE (pre)";
                    break;
                case CALLBACK_TYPE::MfCreateNamedPipePost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_CREATE_NAMED_PIPE (post)";
                    break;
                case CALLBACK_TYPE::MfClosePre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_CLOSE (pre)";
                    break;
                case CALLBACK_TYPE::MfClosePost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_CLOSE (post)";
                    break;
                case CALLBACK_TYPE::MfReadPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_READ (pre)";
                    break;
                case CALLBACK_TYPE::MfReadPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_READ (post)";
                    break;
                case CALLBACK_TYPE::MfWritePre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_WRITE (pre)";
                    break;
                case CALLBACK_TYPE::MfWritePost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_WRITE (post)";
                    break;
                case CALLBACK_TYPE::MfQueryInformationPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_QUERY_INFORMATION (pre)";
                    break;
                case CALLBACK_TYPE::MfQueryInformationPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_QUERY_INFORMATION (post)";
                    break;
                case CALLBACK_TYPE::MfSetInformationPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_SET_INFORMATION (pre)";
                    break;
                case CALLBACK_TYPE::MfSetInformationPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_SET_INFORMATION (post)";
                    break;
                case CALLBACK_TYPE::MfQueryEaPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_QUERY_EA (pre)";
                    break;
                case CALLBACK_TYPE::MfQueryEaPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_QUERY_EA (post)";
                    break;
                case CALLBACK_TYPE::MfSetEaPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_SET_EA (pre)";
                    break;
                case CALLBACK_TYPE::MfSetEaPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_SET_EA (post)";
                    break;
                case CALLBACK_TYPE::MfFlushBuffersPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_FLUSH_BUFFERS (pre)";
                    break;
                case CALLBACK_TYPE::MfFlushBuffersPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_FLUSH_BUFFERS (post)";
                    break;
                case CALLBACK_TYPE::MfQueryVolumeInformationPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_QUERY_VOLUME_INFORMATION (pre)";
                    break;
                case CALLBACK_TYPE::MfQueryVolumeInformationPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_QUERY_VOLUME_INFORMATION (post)";
                    break;
                case CALLBACK_TYPE::MfSetVolumeInformationPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_SET_VOLUME_INFORMATION (pre)";
                    break;
                case CALLBACK_TYPE::MfSetVolumeInformationPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_SET_VOLUME_INFORMATION (post)";
                    break;
                case CALLBACK_TYPE::MfDirectoryControlPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_DIRECTORY_CONTROL (pre)";
                    break;
                case CALLBACK_TYPE::MfDirectoryControlPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_DIRECTORY_CONTROL (post)";
                    break;
                case CALLBACK_TYPE::MfFileSystemControlPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_FILE_SYSTEM_CONTROL (pre)";
                    break;
                case CALLBACK_TYPE::MfFileSystemControlPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_FILE_SYSTEM_CONTROL (post)";
                    break;
                case CALLBACK_TYPE::MfDeviceControlPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_DEVICE_CONTROL (pre)";
                    break;
                case CALLBACK_TYPE::MfDeviceControlPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_DEVICE_CONTROL (post)";
                    break;
                case CALLBACK_TYPE::MfInternalDeviceControlPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_INTERNAL_DEVICE_CONTROL (pre)";
                    break;
                case CALLBACK_TYPE::MfInternalDeviceControlPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_INTERNAL_DEVICE_CONTROL (post)";
                    break;
                case CALLBACK_TYPE::MfShutdownPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_SHUTDOWN (pre)";
                    break;
                case CALLBACK_TYPE::MfShutdownPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_SHUTDOWN (post)";
                    break;
                case CALLBACK_TYPE::MfLockControlPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_LOCK_CONTROL (pre)";
                    break;
                case CALLBACK_TYPE::MfLockControlPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_LOCK_CONTROL (post)";
                    break;
                case CALLBACK_TYPE::MfCleanupPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_CLEANUP (pre)";
                    break;
                case CALLBACK_TYPE::MfCleanupPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_CLEANUP (post)";
                    break;
                case CALLBACK_TYPE::MfCreateMailslotPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_CREATE_MAILSLOT (pre)";
                    break;
                case CALLBACK_TYPE::MfCreateMailslotPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_CREATE_MAILSLOT (post)";
                    break;
                case CALLBACK_TYPE::MfQuerySecurityPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_QUERY_SECURITY (pre)";
                    break;
                case CALLBACK_TYPE::MfQuerySecurityPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_QUERY_SECURITY (post)";
                    break;
                case CALLBACK_TYPE::MfSetSecurityPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_SET_SECURITY (pre)";
                    break;
                case CALLBACK_TYPE::MfSetSecurityPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_SET_SECURITY (post)";
                    break;
                case CALLBACK_TYPE::MfPowerPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_POWER (pre)";
                    break;
                case CALLBACK_TYPE::MfPowerPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_POWER (post)";
                    break;
                case CALLBACK_TYPE::MfSystemControlPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_SYSTEM_CONTROL (pre)";
                    break;
                case CALLBACK_TYPE::MfSystemControlPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_SYSTEM_CONTROL (post)";
                    break;
                case CALLBACK_TYPE::MfDeviceChangePre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_DEVICE_CHANGE (pre)";
                    break;
                case CALLBACK_TYPE::MfDeviceChangePost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_DEVICE_CHANGE (post)";
                    break;
                case CALLBACK_TYPE::MfQueryQuotaPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_QUERY_QUOTA (pre)";
                    break;
                case CALLBACK_TYPE::MfQueryQuotaPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_QUERY_QUOTA (post)";
                    break;
                case CALLBACK_TYPE::MfSetQuotaPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_SET_QUOTA (pre)";
                    break;
                case CALLBACK_TYPE::MfSetQuotaPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_SET_QUOTA (post)";
                    break;
                case CALLBACK_TYPE::MfPnpPre:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_PNP (pre)";
                    break;
                case CALLBACK_TYPE::MfPnpPost:
                    plvdi->item.pszText = (LPWSTR)L"IRP_MJ_PNP (post)";
                    break;
                default:
                    plvdi->item.pszText = (LPWSTR)L"Unknown";
                    break;
                }
                break;
            }
            case 2:
            {
                plvdi->item.pszText = Module;
                break;
            }
            case 3:
            {
                plvdi->item.pszText = (Callback->Suppressed) ? (LPWSTR)L"Yes" : (LPWSTR)L"No";
                break;
            }
            default:
                plvdi->item.pszText = (LPWSTR)L"N/A";
                break;
            }
        }
        case NM_CUSTOMDRAW:
        {
            LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;

            BOOL IsNotable = FALSE;
            BOOL IsSuppressed = FALSE;
            ULONG CallbackIndex = lplvcd->nmcd.lItemlParam;

            if (wd.KmcCallbacks.size())
            {
                PCALLBACK_ENTRY Callback = wd.KmcCallbacks.at(CallbackIndex);
                
                if (Callback->Notable)
                    IsNotable = TRUE;

                if (Callback->Suppressed)
                    IsSuppressed = TRUE;
            }

            switch (lplvcd->nmcd.dwDrawStage)
            {
            case CDDS_PREPAINT:
                return CDRF_NOTIFYITEMDRAW;
            case CDDS_ITEMPREPAINT:
            {
                if (IsSuppressed)
                {
                    lplvcd->clrText = RGB(255, 255, 255);
                    lplvcd->clrTextBk = RGB(128, 128, 128);
                }
                else if (IsNotable)
                {
                    lplvcd->clrText = RGB(0, 0, 0);
                    lplvcd->clrTextBk = RGB(255, 255, 200);
                }
                else
                {
                    lplvcd->clrText = RGB(0, 0, 0);
                    lplvcd->clrTextBk = RGB(255, 255, 255);
                }
                return CDRF_NEWFONT;
            }
            case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
                return CDRF_NEWFONT;
            }
            break;
        }
        }
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}

// Function:    UmhWndProc
// Description: Processes messages for the user-mode hooks tab.
LRESULT CALLBACK UmhWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        switch (HIWORD(wParam))
        {
        case BN_CLICKED:
        {
            if (wh.UmhRefreshButton == (HWND)lParam)
                UmhLoadResults();
            else if (wh.UmhRestoreButton == (HWND)lParam)
                UmhRestoreFunction();
            else if (wh.UmhLoadButton == (HWND)lParam)
                UmhLoadDll();
            break;
        }
        }
        break;
    }
    case WM_NOTIFY:
    {
        NMLVDISPINFO* plvdi;

        switch (((LPNMHDR)lParam)->code)
        {
        case LVN_GETDISPINFO:
        {
            plvdi = (NMLVDISPINFO*)lParam;

            int ModuleIndex = HIWORD(plvdi->item.lParam);
            int FunctionIndex = LOWORD(plvdi->item.lParam);
            PLOADED_MODULE lm = wd.UmhModules.at(ModuleIndex);
            PHOOKED_FUNCTION hf = lm->HookedFunctions.at(FunctionIndex);

            LPWSTR Ordinal = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH);
            StringCbPrintfW(Ordinal, MAX_PATH, L"%d", hf->Ordinal);
            LPWSTR Address = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH);
            StringCbPrintfW(Address, MAX_PATH, L"0x%p", (ULONG64)hf->Address);

            switch (plvdi->item.iSubItem)
            {
            case 0:
                plvdi->item.pszText = (LPWSTR)lm->Path;
                break;
            case 1:
                plvdi->item.pszText = (LPWSTR)hf->Name;
                break;
            case 2:
                plvdi->item.pszText = Ordinal;
                break;
            case 3:
                plvdi->item.pszText = Address;
                break;
            default:
                plvdi->item.pszText = (LPWSTR)L"N/A";
                break;
            }
        }
        }
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}

// Function:    UmeWndProc
// Description: Processes messages for the ETW sessions tab.
LRESULT CALLBACK UmeWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        switch (HIWORD(wParam))
        {
        case BN_CLICKED:
        {
            if (wh.UmeRefreshButton == (HWND)lParam)
                UmeLoadResults();
            else if (wh.UmeDisableButton == (HWND)lParam)
                UmeDisableSelectedProvider();
            else if (wh.UmeStopButton == (HWND)lParam)
                UmeStopTracingSession();
            break;
        }
        }
        break;
    }
    case WM_NOTIFY:
    {
        NMLVDISPINFO* plvdi;

        switch (((LPNMHDR)lParam)->code)
        {
        case LVN_GETDISPINFO:
        {
            plvdi = (NMLVDISPINFO*)lParam;

            int SessionIndex = HIWORD(plvdi->item.lParam);
            int ProviderIndex = LOWORD(plvdi->item.lParam);
            PTRACING_SESSION ts = wd.UmeSessions.at(SessionIndex);
            PTRACE_PROVIDER tp = ts->EnabledProviders.at(ProviderIndex);

            switch (plvdi->item.iSubItem)
            {
            case 0:
                plvdi->item.pszText = (LPWSTR)ts->InstanceName;
                break;
            case 1:
                plvdi->item.pszText = (LPWSTR)tp->ProviderName;
                break;
            default:
                plvdi->item.pszText = (LPWSTR)L"N/A";
                break;
            }
            break;
        }
        case NM_CUSTOMDRAW:
        {
            LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;

            BOOL IsNotable = FALSE;
            int SessionIndex = HIWORD(lplvcd->nmcd.lItemlParam);
            int ProviderIndex = LOWORD(lplvcd->nmcd.lItemlParam);

            if (wd.UmeSessions.size())
            {
                PTRACING_SESSION ts = wd.UmeSessions.at(SessionIndex);

                if (ts->Notable)
                    IsNotable = TRUE;
                else if (ts->EnabledProviders.size())
                    IsNotable = ts->EnabledProviders.at(ProviderIndex)->Notable;
            }

            switch (lplvcd->nmcd.dwDrawStage)
            {
            case CDDS_PREPAINT:
                return CDRF_NOTIFYITEMDRAW;
            case CDDS_ITEMPREPAINT:
            {
                if (IsNotable)
                {
                    lplvcd->clrText = RGB(0, 0, 0);
                    lplvcd->clrTextBk = RGB(255, 255, 200);
                }
                else
                {
                    lplvcd->clrText = RGB(0, 0, 0);
                    lplvcd->clrTextBk = RGB(255, 255, 255);
                }
                return CDRF_NEWFONT;
            }
            case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
                return CDRF_NEWFONT;
            }
            break;
        }
        }
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}

// Function:    PaintWindow
// Description: Paints the main window.
// Called from: MainWndProc
VOID PaintWindow(HWND hWnd)
{
    // Get global instance.
    g_hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);

    // Get bounding box for window.
    RECT rcMain;
    GetClientRect(hWnd, &rcMain);

    // Set font to default GUI font.
    LOGFONT lf;
    GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
    HFONT hFont = CreateFont(
        lf.lfHeight, lf.lfWidth,
        lf.lfEscapement, lf.lfOrientation, lf.lfWeight,
        lf.lfItalic, lf.lfUnderline, lf.lfStrikeOut,
        lf.lfCharSet, lf.lfOutPrecision, lf.lfClipPrecision,
        lf.lfQuality, lf.lfPitchAndFamily, lf.lfFaceName);
    SendMessage(hWnd, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Begin painting.
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    // Status bar.
    wh.StatusBar = CreateWindowEx(
        NULL,                                   // no extended styles
        STATUSCLASSNAME,                        // name of status bar class
        (PCTSTR)NULL,                           // no text when first created
        WS_CHILD | WS_VISIBLE,                  // creates a visible child window
        0, 0, 0, 0,                             // ignores size and position
        hWnd,                                   // handle to parent window
        (HMENU)0,                               // child window identifier
        g_hInst,                                // handle to application instance
        NULL);                                  // no window creation data
    SendMessage(wh.StatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)L"Ready");

    // Tab control.
    wh.TabControl = CreateWindowEx(
        NULL,                                    // Extended styles
        WC_TABCONTROL,                           // Predefined class; Unicode assumed 
        L"",                                     // Control text 
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE, // Styles 
        0,                                       // X position 
        0,                                       // Y position 
        rcMain.right,                            // Control width
        rcMain.bottom - 22,                      // Control height
        hWnd,                                    // Parent window
        NULL,                                    // No menu.
        g_hInst,                                 // Global instance handle
        NULL);                                   // Pointer not needed
    SendMessage(wh.TabControl, WM_SETFONT, (WPARAM)hFont, TRUE);

    /**
     * Kernel-mode Callbacks
     */

    TCITEM tie;
    tie.mask = TCIF_TEXT;
    tie.pszText = (LPWSTR)L"Kernel-mode Callbacks";
    TabCtrl_InsertItem(wh.TabControl, 0, &tie);
    wh.KmcPage = CreateWindowEx(
        NULL,
        WC_STATIC,
        L"",
        WS_CHILD | WS_VISIBLE,
        0,
        25,
        rcMain.right - 5,
        rcMain.bottom - 55,
        wh.TabControl,
        NULL,
        g_hInst,
        NULL);
    SetWindowLongPtr(wh.KmcPage, GWLP_WNDPROC, (LONG_PTR)KmcWndProc);

    wh.KmcRefreshButton = CreateWindowEx(
        NULL,
        L"BUTTON",
        L"Refresh Results",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        5,
        5,
        100,
        30,
        wh.KmcPage,
        (HMENU)1,
        g_hInst,
        NULL);
    SendMessage(wh.KmcRefreshButton, WM_SETFONT, (WPARAM)hFont, TRUE);

    wh.KmcSuppressButton = CreateWindowEx(
        NULL,
        L"BUTTON",
        L"Suppress Callback",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        110,
        5,
        100,
        30,
        wh.KmcPage,
        NULL,
        g_hInst,
        NULL);
    SendMessage(wh.KmcSuppressButton, WM_SETFONT, (WPARAM)hFont, TRUE);

    wh.KmcRevertButton = CreateWindowEx(
        NULL,
        L"BUTTON",
        L"Revert Callback",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        215,
        5,
        100,
        30,
        wh.KmcPage,
        NULL,
        g_hInst,
        NULL);
    SendMessage(wh.KmcRevertButton, WM_SETFONT, (WPARAM)hFont, TRUE);

    wh.KmcCountLabel = CreateWindowEx(
        NULL,
        WC_STATIC,
        L"Count: 0 callbacks.",
        WS_CHILD | WS_VISIBLE,
        320,
        5,
        350,
        15,
        wh.KmcPage,
        NULL,
        g_hInst,
        NULL);
    SendMessage(wh.KmcCountLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

    wh.KmcTipLabel = CreateWindowEx(
        NULL,
        WC_STATIC,
        L"Tip: No results? Run elevated to load the driver.",
        WS_CHILD | WS_VISIBLE,
        320,
        20,
        250,
        15,
        wh.KmcPage,
        NULL,
        g_hInst,
        NULL);
    SendMessage(wh.KmcTipLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

    wh.KmcListView = CreateWindowEx(
        NULL,
        WC_LISTVIEW,
        L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
        5,
        40,
        rcMain.right - 15,
        rcMain.bottom - 100,
        wh.KmcPage,
        (HMENU)0,
        g_hInst,
        NULL);
    ListView_SetExtendedListViewStyle(wh.KmcListView, LVS_EX_FULLROWSELECT);

    LVCOLUMN lvc = { 0 };
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.iSubItem = 0;
    lvc.pszText = (LPWSTR)L"Collection Type";
    lvc.cx = 150;
    ListView_InsertColumn(wh.KmcListView, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.pszText = (LPWSTR)L"Callback Type";
    lvc.cx = 300;
    ListView_InsertColumn(wh.KmcListView, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.pszText = (LPWSTR)L"Module";
    lvc.cx = 250;
    ListView_InsertColumn(wh.KmcListView, 2, &lvc);

    lvc.iSubItem = 3;
    lvc.pszText = (LPWSTR)L"Is Suppressed?";
    lvc.cx = 100;
    ListView_InsertColumn(wh.KmcListView, 3, &lvc);

    /**
     * User-mode Hooks
     */

    tie.pszText = (LPWSTR)L"User-mode Hooks";
    TabCtrl_InsertItem(wh.TabControl, 1, &tie);
    wh.UmhPage = CreateWindowEx(
        NULL,
        WC_STATIC,
        L"",
        WS_CHILD | WS_VISIBLE,
        0,
        25,
        rcMain.right - 5,
        rcMain.bottom - 55,
        wh.TabControl,
        NULL,
        g_hInst,
        NULL);
    SetWindowLongPtr(wh.UmhPage, GWLP_WNDPROC, (LONG_PTR)UmhWndProc);

    wh.UmhRefreshButton = CreateWindowEx(
        NULL,
        L"BUTTON",
        L"Refresh Results",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        5,
        5,
        100,
        30,
        wh.UmhPage,
        NULL,
        g_hInst,
        NULL);
    SendMessage(wh.UmhRefreshButton, WM_SETFONT, (WPARAM)hFont, TRUE);

    wh.UmhRestoreButton = CreateWindowEx(
        NULL,
        L"BUTTON",
        L"Restore Function",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        110,
        5,
        100,
        30,
        wh.UmhPage,
        NULL,
        g_hInst,
        NULL);
    SendMessage(wh.UmhRestoreButton, WM_SETFONT, (WPARAM)hFont, TRUE);

    wh.UmhLoadButton = CreateWindowEx(
        NULL,
        L"BUTTON",
        L"Load Testing DLL",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        215,
        5,
        100,
        30,
        wh.UmhPage,
        NULL,
        g_hInst,
        NULL);
    SendMessage(wh.UmhLoadButton, WM_SETFONT, (WPARAM)hFont, TRUE);

    wh.UmhCountLabel = CreateWindowEx(
        NULL,
        WC_STATIC,
        L"Count: 0 hooked functions.",
        WS_CHILD | WS_VISIBLE,
        320,
        5,
        350,
        15,
        wh.UmhPage,
        NULL,
        g_hInst,
        NULL);
    SendMessage(wh.UmhCountLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

    wh.UmhTipLabel = CreateWindowEx(
        NULL,
        WC_STATIC,
        L"Tip: Validate unhooking by loading a test DLL.",
        WS_CHILD | WS_VISIBLE,
        320,
        20,
        250,
        15,
        wh.UmhPage,
        NULL,
        g_hInst,
        NULL);
    SendMessage(wh.UmhTipLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

    wh.UmhListView = CreateWindowEx(
        NULL,
        WC_LISTVIEW,
        L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
        5,
        40,
        rcMain.right - 15,
        rcMain.bottom - 100,
        wh.UmhPage,
        (HMENU)0,
        g_hInst,
        NULL);
    ListView_SetExtendedListViewStyle(wh.UmhListView, LVS_EX_FULLROWSELECT);

    lvc.iSubItem = 0;
    lvc.pszText = (LPWSTR)L"Module";
    lvc.cx = 250;
    ListView_InsertColumn(wh.UmhListView, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.pszText = (LPWSTR)L"Function Name";
    lvc.cx = 300;
    ListView_InsertColumn(wh.UmhListView, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.pszText = (LPWSTR)L"Ordinal";
    lvc.cx = 100;
    ListView_InsertColumn(wh.UmhListView, 2, &lvc);

    lvc.iSubItem = 3;
    lvc.pszText = (LPWSTR)L"Virtual Address";
    lvc.cx = 200;
    ListView_InsertColumn(wh.UmhListView, 3, &lvc);

    /**
     * ETW Trace Sessions
     */

    tie.pszText = (LPWSTR)L"ETW Trace Sessions";
    TabCtrl_InsertItem(wh.TabControl, 2, &tie);
    wh.UmePage = CreateWindowEx(
        NULL,
        WC_STATIC,
        L"",
        WS_CHILD | WS_VISIBLE,
        0,
        25,
        rcMain.right - 5,
        rcMain.bottom - 55,
        wh.TabControl,
        NULL,
        g_hInst,
        NULL);
    SetWindowLongPtr(wh.UmePage, GWLP_WNDPROC, (LONG_PTR)UmeWndProc);

    wh.UmeRefreshButton = CreateWindowEx(
        NULL,
        L"BUTTON",
        L"Refresh Results",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        5,
        5,
        100,
        30,
        wh.UmePage,
        NULL,
        g_hInst,
        NULL);
    SendMessage(wh.UmeRefreshButton, WM_SETFONT, (WPARAM)hFont, TRUE);

    wh.UmeDisableButton = CreateWindowEx(
        NULL,
        L"BUTTON",
        L"Disable Provider",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        110,
        5,
        100,
        30,
        wh.UmePage,
        NULL,
        g_hInst,
        NULL);
    SendMessage(wh.UmeDisableButton, WM_SETFONT, (WPARAM)hFont, TRUE);

    wh.UmeStopButton = CreateWindowEx(
        NULL,
        L"BUTTON",
        L"Stop Session",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        215,
        5,
        100,
        30,
        wh.UmePage,
        NULL,
        g_hInst,
        NULL);
    SendMessage(wh.UmeStopButton, WM_SETFONT, (WPARAM)hFont, TRUE);

    wh.UmeCountLabel = CreateWindowEx(
        NULL,
        WC_STATIC,
        L"Count: 0 sessions.",
        WS_CHILD | WS_VISIBLE,
        320,
        5,
        350,
        15,
        wh.UmePage,
        NULL,
        g_hInst,
        NULL);
    SendMessage(wh.UmeCountLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

    wh.UmeTipLabel = CreateWindowEx(
        NULL,
        WC_STATIC,
        L"Tip: Missing results? Run as SYSTEM to view more sessions.",
        WS_CHILD | WS_VISIBLE,
        320,
        20,
        300,
        15,
        wh.UmePage,
        NULL,
        g_hInst,
        NULL);
    SendMessage(wh.UmeTipLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

    wh.UmeListView = CreateWindowEx(
        NULL,
        WC_LISTVIEW,
        L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
        5,
        40,
        rcMain.right - 15,
        rcMain.bottom - 100,
        wh.UmePage,
        (HMENU)0,
        g_hInst,
        NULL);
    ListView_SetExtendedListViewStyle(wh.UmeListView, LVS_EX_FULLROWSELECT);

    lvc.iSubItem = 0;
    lvc.pszText = (LPWSTR)L"Session";
    lvc.cx = 300;
    ListView_InsertColumn(wh.UmeListView, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.pszText = (LPWSTR)L"Enabled Provider";
    lvc.cx = 300;
    ListView_InsertColumn(wh.UmeListView, 1, &lvc);

    /**
     * About Page
     */

    tie.pszText = (LPWSTR)L"About";
    TabCtrl_InsertItem(wh.TabControl, 3, &tie);
    wh.AbtPage = CreateWindowEx(
        NULL,
        WC_STATIC,
        L"",
        WS_CHILD | WS_VISIBLE, // | WS_BORDER,
        0,
        25,
        rcMain.right - 5,
        rcMain.bottom - 55,
        wh.TabControl,
        NULL,
        g_hInst,
        NULL);

    wh.AbtLabel = CreateWindowEx(
        NULL,
        WC_STATIC,
        L"Telemetry Sourcerer is a utility that can be used to enumerate "
        L"and disable common sources of events used by endpoint security "
        L"products (AV/EDR).\n\n"
        L"Developed by @Jackson_T (2020), leveraging code from @gentilkiwi, "
        L"@fdiskyou, and @0x00dtm. Licenced under the Apache License v2.0.\n\n"
        L"Use of this tool is intended for research purposes only and does "
        L"not attempt to be OPSEC-safe.\n\n"
        L"Code: github.com/jthuraisamy/TelemetrySourcerer\n"
        L"Methodology: jackson-t.ca/edr-reversing-evading-01.html\n",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        5,
        150,
        rcMain.right - 10,
        rcMain.bottom - 60,
        wh.AbtPage,
        NULL,
        g_hInst,
        NULL);
    SendMessage(wh.AbtLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

    // END PAINTING
    ShowWindow(wh.UmhPage, SW_HIDE);
    ShowWindow(wh.UmePage, SW_HIDE);
    ShowWindow(wh.AbtPage, SW_HIDE);
    ShowWindow(wh.KmcPage, SW_HIDE);
    EndPaint(hWnd, &ps);
}

// Function:    ResizeWindow
// Description: Handles window resizes (currently not supported).
// Called from: MainWndProc
VOID ResizeWindow(HWND hWnd)
{
    RECT rcClient;
    GetClientRect(hWnd, &rcClient);

    SetWindowPos(wh.TabControl, HWND_TOP, 0, 0, rcClient.right, rcClient.bottom - 22, SWP_SHOWWINDOW);
    SetWindowPos(wh.KmcPage, HWND_TOP, 0, 25, rcClient.right - 5, rcClient.bottom - 40, SWP_SHOWWINDOW);
    SetWindowPos(wh.StatusBar, NULL, 0, rcClient.bottom - 10, rcClient.right, 10, SWP_NOZORDER);
}

// Function:    KmcLoadResults
// Description: Loads kernel-mode callback results.
// Called from: MainWndProc after painting, and KmcWndProc when clicking the refresh button.
VOID KmcLoadResults()
{
    DWORD DriverStatus = LoadDriver();
    switch (DriverStatus)
    {
    case ERROR_FILE_NOT_FOUND:
        MessageBox(
            NULL,
            L"Could not find TelemetrySourcererDriver.sys.\n\n"
            L"Please ensure it is in the same directory as the executable.",
            TOOL_NAME,
            MB_ICONERROR);
        break;
    case ERROR_INVALID_IMAGE_HASH:
        MessageBox(
            NULL,
            L"The signature for the driver failed verification.\n\n"
            L"To load kernel-mode callback results, either enable test signing, "
            L"or sign this driver with a valid certificate.",
            TOOL_NAME,
            MB_ICONERROR);
        break;
    default:
        break;
    }

    SendMessage(wh.StatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)L"Loading kernel-mode callbacks...");

    // Delete rows and fetch latest results.
    ListView_DeleteAllItems(wh.KmcListView);
    wd.KmcCallbacks = GetCallbacks(wd.KmcCallbacks);
    DWORD ListItemCount = 0;

    for (int CallbackIndex = 0; CallbackIndex < wd.KmcCallbacks.size(); CallbackIndex++)
    {
        
        LVITEM lvi = { 0 };
        lvi.pszText = LPSTR_TEXTCALLBACK; // Sends an LVN_GETDISPINFO message.
        lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
        lvi.stateMask = 0;
        lvi.iSubItem = 0;
        lvi.state = 0;
        lvi.iItem = ListItemCount++;
        lvi.lParam = CallbackIndex;

        ListView_InsertItem(wh.KmcListView, &lvi);
    }

    LPWSTR CountText = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH);
    StringCbPrintfW(CountText, MAX_PATH, L"Count: %d callbacks.", wd.KmcCallbacks.size());
    Static_SetText(wh.KmcCountLabel, CountText);

    SendMessage(wh.StatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)L"Ready");
}

// Function:    KmcSuppressCallback
// Description: Suppresses the selected callback (if eligible).
// Called from: KmcWndProc when clicking the suppress button.
VOID KmcSuppressCallback()
{
    int SelectedIndex = ListView_GetNextItem(wh.KmcListView, -1, LVNI_SELECTED);

    if (SelectedIndex != -1)
    {
        LVITEM lvi = { 0 };
        lvi.mask = LVIF_PARAM;
        lvi.iItem = SelectedIndex;
        lvi.iSubItem = 0;
        ListView_GetItem(wh.KmcListView, &lvi);

        PCALLBACK_ENTRY Callback = wd.KmcCallbacks.at(lvi.lParam);
        WCHAR MessageTitle[MAX_PATH] = { 0 };
        StringCbPrintfW(MessageTitle, MAX_PATH, L"%ls + 0x%x", Callback->ModuleName, Callback->ModuleOffset);

        if (SuppressCallback(Callback))
        {
            Callback->Suppressed = TRUE;
            MessageBox(NULL, L"Successfully suppressed callback!", MessageTitle, MB_OK | MB_ICONINFORMATION);
            KmcLoadResults();
            ListView_EnsureVisible(wh.KmcListView, SelectedIndex, FALSE);
        }
        else
        {
            MessageBox(NULL, L"Callback could not be suppressed for unspecified reason.", MessageTitle, MB_OK | MB_ICONERROR);
        }
    }
}

// Function:    KmcRevertCallback
// Description: Reverts the selected callback (if suppressed/eligible).
// Called from: KmcWndProc when clicking the revert button.
VOID KmcRevertCallback()
{
    int SelectedIndex = ListView_GetNextItem(wh.KmcListView, -1, LVNI_SELECTED);

    if (SelectedIndex != -1)
    {
        LVITEM lvi = { 0 };
        lvi.mask = LVIF_PARAM;
        lvi.iItem = SelectedIndex;
        lvi.iSubItem = 0;
        ListView_GetItem(wh.KmcListView, &lvi);

        PCALLBACK_ENTRY Callback = wd.KmcCallbacks.at(lvi.lParam);
        WCHAR MessageTitle[MAX_PATH] = { 0 };
        StringCbPrintfW(MessageTitle, MAX_PATH, L"%ls + 0x%x", &Callback->ModuleName, Callback->ModuleOffset);

        if (Callback->Suppressed)
        {
            if (Callback->OriginalQword == GetSuppressionValue(Callback->Type))
            {
                MessageBox(NULL, L"The callback could not be reverted because the function's original first bytes are unknown.", MessageTitle, MB_OK | MB_ICONERROR);
            }
            else if (RevertCallback(Callback))
            {
                Callback->Suppressed = FALSE;
                MessageBox(NULL, L"Successfully reverted callback!", MessageTitle, MB_OK | MB_ICONINFORMATION);
                KmcLoadResults();
                ListView_EnsureVisible(wh.KmcListView, SelectedIndex, FALSE);
            }
            else
            {
                MessageBox(NULL, L"Callback could not be reverted for unspecified reason.", MessageTitle, MB_OK | MB_ICONERROR);
            }
        }
        else
        {
            MessageBox(NULL, L"The selected callback is currently not suppressed.", MessageTitle, MB_OK | MB_ICONERROR);
        }
    }
}

// Function:    UmhLoadResults
// Description: Loads user-mode inline hook results.
// Called from: MainWndProc after painting, and UmhWndProc when clicking the refresh button.
VOID UmhLoadResults()
{
    SendMessage(wh.StatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)L"Loading user-mode hooks...");

    // Delete rows and fetch latest results.
    ListView_DeleteAllItems(wh.UmhListView);
    wd.UmhModules = CheckAllModulesForHooks();
    DWORD ListItemCount = 0;

    for (int ModuleIndex = 0; ModuleIndex < wd.UmhModules.size(); ModuleIndex++)
    {
        PLOADED_MODULE lm = wd.UmhModules.at(ModuleIndex);

        for (int FunctionIndex = 0; FunctionIndex < lm->HookedFunctions.size(); FunctionIndex++)
        {
            LVITEM lvi = { 0 };
            lvi.pszText = LPSTR_TEXTCALLBACK; // Sends an LVN_GETDISPINFO message.
            lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
            lvi.stateMask = 0;
            lvi.iSubItem = 0;
            lvi.state = 0;
            lvi.iItem = ListItemCount++;
            lvi.lParam = (ModuleIndex << 16) | FunctionIndex;

            ListView_InsertItem(wh.UmhListView, &lvi);
        }
    }

    LPWSTR CountText = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH);
    StringCbPrintfW(CountText, MAX_PATH, L"Count: %d hooked functions.", ListItemCount);
    Static_SetText(wh.UmhCountLabel, CountText);

    SendMessage(wh.StatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)L"Ready");
}

// Function:    UmhRestoreFunction
// Description: Unhooks the selected function.
// Called from: UmhWndProc when clicking the restore button.
VOID UmhRestoreFunction()
{
    int SelectedIndex = ListView_GetNextItem(wh.UmhListView, -1, LVNI_SELECTED);

    if (SelectedIndex != -1)
    {
        LVITEM lvi = { 0 };
        lvi.mask = LVIF_PARAM;
        lvi.iItem = SelectedIndex;
        lvi.iSubItem = 0;
        ListView_GetItem(wh.UmhListView, &lvi);

        int ModuleIndex = HIWORD(lvi.lParam);
        int FunctionIndex = LOWORD(lvi.lParam);

        PLOADED_MODULE Module = wd.UmhModules.at(ModuleIndex);
        PHOOKED_FUNCTION Function = Module->HookedFunctions.at(FunctionIndex);

        WCHAR MessageTitle[MAX_PATH] = { 0 };
        StringCbPrintfW(MessageTitle, MAX_PATH, L"%ls: %ls", Module->Path, Function->Name);
        if (RestoreHookedFunction(Function))
            MessageBox(NULL, L"Successfully unhooked function!", MessageTitle, MB_OK | MB_ICONINFORMATION);
        else
            MessageBox(NULL, L"Function could not be unhooked for unspecified reason.", MessageTitle, MB_OK | MB_ICONERROR);

        UmhLoadResults();
        ListView_EnsureVisible(wh.UmhListView, SelectedIndex, FALSE);
    }
}

// Function:    UmhLoadDll
// Description: Loads a testing DLL to validate unhooking efficacy.
// Called from: UmhWndProc when clicking the load DLL button.
VOID UmhLoadDll()
{
    SendMessage(wh.StatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)L"Loading test DLL...");

    WCHAR DllPath[MAX_PATH] = { 0 };
    OPENFILENAMEW OpenFileName = { 0 };
    OpenFileName.lStructSize = sizeof(OpenFileName);
    OpenFileName.hwndOwner = wh.Main;
    OpenFileName.lpstrFilter = L"DLL Files (*.dll)\0*.DLL\0";
    OpenFileName.lpstrFile = DllPath;
    OpenFileName.nMaxFile = MAX_PATH;

    GetOpenFileNameW(&OpenFileName);
    LoadLibraryW(DllPath); // ToDo validation.

    SendMessage(wh.StatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)L"Ready");
}

// Function:    UmeLoadResults
// Description: Load ETW trace session results.
// Called from: MainWndProc after painting, and UmeWndProc when clicking the refresh button.
VOID UmeLoadResults()
{
    SendMessage(wh.StatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)L"Loading ETW sessions...");

    // Delete rows and fetch latest results.
    ListView_DeleteAllItems(wh.UmeListView);
    wd.UmeSessions = GetSessions();
    DWORD ListItemCount = 0;

    for (int SessionIndex = 0; SessionIndex < wd.UmeSessions.size(); SessionIndex++)
    {
        PTRACING_SESSION ts = wd.UmeSessions.at(SessionIndex);

        for (int ProviderIndex = 0; ProviderIndex < ts->EnabledProviders.size(); ProviderIndex++)
        {
            LVITEM lvi = { 0 };
            lvi.pszText = LPSTR_TEXTCALLBACK; // Sends an LVN_GETDISPINFO message.
            lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
            lvi.stateMask = 0;
            lvi.iSubItem = 0;
            lvi.state = 0;
            lvi.iItem = ListItemCount++;
            lvi.lParam = ((ULONG)SessionIndex << 16) | ProviderIndex;

            ListView_InsertItem(wh.UmeListView, &lvi);
        }
    }

    LPWSTR CountText = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH);
    StringCbPrintfW(CountText, MAX_PATH, L"Count: %d sessions.", wd.UmeSessions.size());
    Static_SetText(wh.UmeCountLabel, CountText);

    SendMessage(wh.StatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)L"Ready");
}

// Function:    UmeDisableSelectedProvider
// Description: Disables the selected provider for the session (if eligible).
// Called from: UmeWndProc when clicking the disable provider button.
VOID UmeDisableSelectedProvider()
{
    int SelectedIndex = ListView_GetNextItem(wh.UmeListView, -1, LVNI_SELECTED);

    if (SelectedIndex != -1)
    {
        LVITEM lvi = { 0 };
        lvi.mask = LVIF_PARAM;
        lvi.iItem = SelectedIndex;
        lvi.iSubItem = 0;
        ListView_GetItem(wh.UmeListView, &lvi);

        int SessionIndex = HIWORD(lvi.lParam);
        int ProviderIndex = LOWORD(lvi.lParam);

        PTRACING_SESSION Session = wd.UmeSessions.at(SessionIndex);
        PTRACE_PROVIDER Provider = Session->EnabledProviders.at(ProviderIndex);

        WCHAR MessageTitle[MAX_PATH] = { 0 };
        StringCbPrintfW(MessageTitle, MAX_PATH, L"%ls: %ls", Session->InstanceName, Provider->ProviderName);
        DWORD Status = DisableProvider(Session->LoggerId, &Provider->ProviderId);
        switch (Status)
        {
        case ERROR_SUCCESS:
            MessageBox(NULL, L"Successfully disabled provider!", MessageTitle, MB_OK | MB_ICONINFORMATION);
            break;
        case ERROR_ACCESS_DENIED:
            MessageBox(NULL, L"Denied. Administrative privileges required.", MessageTitle, MB_OK | MB_ICONERROR);
            break;
        default:
            MessageBox(NULL, L"Provider could not be disabled for unspecified reason.", MessageTitle, MB_OK | MB_ICONERROR);
            break;
        }

        UmeLoadResults();
        ListView_EnsureVisible(wh.UmeListView, SelectedIndex, FALSE);
    }
}

// Function:    UmeDisableSelectedProvider
// Description: Stops the selected session (if eligible).
// Called from: UmeWndProc when clicking the stop session button.
VOID UmeStopTracingSession()
{
    int SelectedIndex = ListView_GetNextItem(wh.UmeListView, -1, LVNI_SELECTED);

    if (SelectedIndex != -1)
    {
        LVITEM lvi = { 0 };
        lvi.mask = LVIF_PARAM;
        lvi.iItem = SelectedIndex;
        lvi.iSubItem = 0;
        ListView_GetItem(wh.UmeListView, &lvi);

        int SessionIndex = HIWORD(lvi.lParam);
        int ProviderIndex = LOWORD(lvi.lParam);

        PTRACING_SESSION Session = wd.UmeSessions.at(SessionIndex);
        PTRACE_PROVIDER Provider = Session->EnabledProviders.at(ProviderIndex);

        DWORD Status = StopTracingSession(Session->LoggerId);
        switch (Status)
        {
        case ERROR_SUCCESS:
            MessageBox(NULL, L"Successfully stopped tracing session!", Session->InstanceName, MB_OK | MB_ICONINFORMATION);
            break;
        case ERROR_ACCESS_DENIED:
            MessageBox(NULL, L"Denied. Administrative privileges required.", Session->InstanceName, MB_OK | MB_ICONERROR);
            break;
        default:
            MessageBox(NULL, L"Session could not be stopped for unspecified reason.", Session->InstanceName, MB_OK | MB_ICONERROR);
            break;
        }

        UmeLoadResults();
        ListView_EnsureVisible(wh.UmeListView, SelectedIndex, FALSE);
    }
}