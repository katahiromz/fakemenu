/*
 * PROJECT:     ReactOS FakeMenu Library
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     A test program for FakeMenu
 * COPYRIGHT:   Copyright 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "fakemenu.h"

#define MYWM_NOTIFY_ICON (WM_USER + 100)

static HINSTANCE s_hInst = NULL;
static WCHAR s_szText[MAX_PATH] = L"(Right-Click me!)";

BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    NOTIFYICONDATA data = { NOTIFYICONDATA_V1_SIZE, hwnd, 1, MYWM_NOTIFY_ICON };
    data.hIcon = LoadIcon(s_hInst, MAKEINTRESOURCE(1));
    data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_STATE;
    data.uCallbackMessage = MYWM_NOTIFY_ICON;
    lstrcpynW(data.szTip, L"FakeMenu Test", _countof(data.szTip));
    data.dwState = 0;
    data.dwStateMask = NIS_HIDDEN;
    Shell_NotifyIcon(NIM_ADD, &data);

    return TRUE;
}

void OnPaint(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    PAINTSTRUCT ps;
    if (HDC hdc = BeginPaint(hwnd, &ps))
    {
        DrawText(hdc, s_szText, -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        EndPaint(hwnd, &ps);
    }
}

VOID OnNotifyMenu(HWND hwnd, POINT pt, INT nMenuID)
{
    HMENU hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(nMenuID));
    HFAKEMENU hFakeMenu = FakeMenu_FromHMENU(hMenu);
    DestroyMenu(hMenu);

    LOGFONT lf;
    ZeroMemory(&lf, sizeof(lf));
    lf.lfHeight = -24;
    lstrcpynW(lf.lfFaceName, L"Tahoma", _countof(lf.lfFaceName));
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfQuality = ANTIALIASED_QUALITY;
    FakeMenu_SetLogFont(hFakeMenu, &lf);

    INT id = FakeMenu_TrackPopup(hFakeMenu, pt);
    if (id != 0)
    {
        FakeMenu_GetItemText(hFakeMenu, id, s_szText, _countof(s_szText), FALSE);
        InvalidateRect(hwnd, NULL, TRUE);
    }

    FakeMenu_Destroy(hFakeMenu);
}

void OnRButtonUp(HWND hwnd, int x, int y, UINT flags)
{
    POINT pt = { x, y };
    ClientToScreen(hwnd, &pt);

    OnNotifyMenu(hwnd, pt, 1);
}

LRESULT OnNotifyIcon(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    INT nID = (INT)wParam;
    UINT uMsg = (INT)lParam;

    POINT pt;
    GetCursorPos(&pt);
    switch (uMsg)
    {
        case WM_LBUTTONUP:
            OnNotifyMenu(hwnd, pt, 2);
            break;

        case WM_RBUTTONUP:
            OnNotifyMenu(hwnd, pt, 3);
            break;
    }

    return 0;
}

void OnDestroy(HWND hwnd)
{
    NOTIFYICONDATA data = { NOTIFYICONDATA_V1_SIZE, hwnd, 1 };
    Shell_NotifyIcon(NIM_DELETE, &data);

    PostQuitMessage(0);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
        HANDLE_MSG(hwnd, WM_RBUTTONUP, OnRButtonUp);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE;
        case MYWM_NOTIFY_ICON:
            return OnNotifyIcon(hwnd, wParam, lParam);
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

INT WINAPI
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        INT nCmdShow)
{
    s_hInst = hInstance;
    InitCommonControls();

    FakeMenu_InitInstance();

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = s_hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    wc.hbrBackground = GetStockBrush(WHITE_BRUSH);
    wc.lpszClassName = L"FakeMenu Test";
    RegisterClassExW(&wc);

    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exstyle = WS_EX_TOPMOST | WS_EX_NOACTIVATE;
    HWND hwnd = CreateWindowExW(exstyle, L"FakeMenu Test", L"FakeMenu Test", style,
                                CW_USEDEFAULT, CW_USEDEFAULT, 300, 200,
                                NULL, NULL, s_hInst, NULL);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    FakeMenu_ExitInstance();
    return 0;
}
