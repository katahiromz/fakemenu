/*
 * PROJECT:     ReactOS FakeMenu Library
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Menu-like control without window activation
 * COPYRIGHT:   Copyright 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
#pragma once

#define FAKEMENU_CLASSNAMEA  "katahiromz's FakeMenu"
#define FAKEMENU_CLASSNAMEW L"katahiromz's FakeMenu"
#ifdef UNICODE
    #define FAKEMENU_CLASSNAME FAKEMENU_CLASSNAMEW
#else
    #define FAKEMENU_CLASSNAME FAKEMENU_CLASSNAMEA
#endif

#if defined(NDEBUG) || !defined(__cplusplus)
    DECLARE_HANDLE(HFAKEMENU);
#else
    class FakeMenu;
    typedef class FakeMenu* HFAKEMENU;
#endif

#ifdef __cplusplus
extern "C" {
#endif

BOOL APIENTRY FakeMenu_InitInstance(VOID);
VOID APIENTRY FakeMenu_ExitInstance(VOID);

HFAKEMENU APIENTRY FakeMenu_Create(VOID);
HFAKEMENU APIENTRY FakeMenu_FromHMENU(HMENU hMenu);
INT APIENTRY FakeMenu_TrackPopup(HFAKEMENU hFakeMenu, POINT pt);
VOID APIENTRY FakeMenu_Destroy(HFAKEMENU hFakeMenu);

BOOL APIENTRY FakeMenu_AddString(HFAKEMENU hFakeMenu, UINT nID, LPCWSTR text, UINT fState);
INT APIENTRY FakeMenu_AppendItem(HFAKEMENU hFakeMenu, const MENUITEMINFO* pmii);
VOID APIENTRY FakeMenu_DeleteItems(HFAKEMENU hFakeMenu);

BOOL APIENTRY FakeMenu_EnableItem(HFAKEMENU hFakeMenu, INT iItem, UINT uEnable);
BOOL APIENTRY FakeMenu_CheckItem(HFAKEMENU hFakeMenu, INT iItem, UINT uCheck);
BOOL APIENTRY FakeMenu_CheckRadioItem(HFAKEMENU hFakeMenu, INT iFirst, INT iLast, INT iCheck, BOOL bByPosition);

VOID APIENTRY FakeMenu_SetLogFont(HFAKEMENU hFakeMenu, LPLOGFONT plf OPTIONAL);
BOOL APIENTRY FakeMenu_GetItemText(HFAKEMENU hFakeMenu, INT iItem, LPWSTR pszText, INT cchText, BOOL bByPosition);

#ifdef __cplusplus
} // extern "C"
#endif
