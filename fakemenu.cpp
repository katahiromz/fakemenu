/* FakeMenu --- Menu-like control without window activation. */
#include <windows.h>
#include <windowsx.h>
#include <vssym32.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <cassert>
#include "fakemenu.h"

// Constants
#define FAKEMENU_MARGIN 8
#define FAKEMENU_CX_RIGHT_SPACE 24
#define FAKEMENU_CX_SEP 6
#define FAKEMENU_CY_SEP 6
#define FAKEMENU_REFRESH_TIMER 999
#define FAKEMENU_REFRESH_INTERVAL 150
#define FAKEMENU_ANIMATION_TIMER 888
#define FAKEMENU_ANIMATION_DELAY 150

//////////////////////////////////////////////////////////////////////////////////////////////
// FakeMenuItem and FakeMenu classes

class FakeMenu;

// FakeMenu item class
class FakeMenuItem
{
public:
    INT m_nID;          // The item ID
    UINT m_fType;       // Same as MENUITEMINFO.fType
    UINT m_fState;      // Same as MENUITEMINFO.fState
    LPWSTR m_pszText;   // malloc'ed
    FakeMenu* m_pSubMenu;

    // FakeMenuItem is a cyclic linked list
    class FakeMenuItem* m_pNext;
    class FakeMenuItem* m_pPrev;

    RECT m_rcItem;

    FakeMenuItem(const MENUITEMINFO* pmii);
    virtual ~FakeMenuItem();

    BOOL IsSep() const
    {
        return (m_fType & MFT_SEPARATOR);
    }

    BOOL IsGrayed() const
    {
        return (m_fState & (MFS_GRAYED | MFS_DISABLED));
    }
};

// The FakeMenu
class FakeMenu
{
protected:
    HWND m_hwnd;                // The window handle
    HWND m_hwndNotify;          // The window to be notified
    BOOL m_fKeyboardUsing;      // Using Keyboard?
    HTHEME m_hTheme;            // The window theme
    INT m_cItems;               // The # of items
    FakeMenuItem* m_pItems;     // The fake menu items
    FakeMenu* m_pParent;        // The parent
    HFONT m_hFont;              // The font
    INT m_iParentItem;          // The index from the parent
    MARGINS m_marginsItem;      // The margins

    BOOL m_fDone;               // The task is done?
    BOOL m_fDestroying;         // Is it destroying the window?
    BOOL m_fDelayed;            // Delayed for animation?
    INT m_idResult;             // The ID to return
    INT m_iOpenSubMenu;         // The index to the sub menu that is open
    INT m_iSelected;            // The selected index

    VOID InitStatus();
    BOOL DoMeasureItem(INT iItem, FakeMenuItem* pItem, LPMEASUREITEMSTRUCT pMeasure);
    BOOL DoDrawItem(INT iItem, FakeMenuItem* pItem, LPDRAWITEMSTRUCT pDraw);
    FakeMenu(HMENU hMenu, FakeMenu* pParent = NULL);
    void MeasureItems(SIZE& size);
    void UpdateVisuals(HWND hwnd);
    void ChooseLocation(POINT& pt, INT cx, INT cy, LPCRECT prcExclude = NULL);
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT GetNextSelectable(INT iItem, BOOL bNext);
    INT GetNextIndex(INT iItem, BOOL bNext);
    BOOL IsFamilyHWND(HWND hwnd);
    static BOOL CALLBACK EnumCloseProc(HWND hwnd, LPARAM lParam);
    static BOOL CALLBACK EnumFindStdMenuProc(HWND hwnd, LPARAM lParam);

public:
    static BOOL DoRegisterClass(VOID);

    FakeMenu();
    static FakeMenu* FromHWND(HWND hwnd);
    static FakeMenu* FromHMENU(HMENU hMenu, FakeMenu* pParent = NULL);
    virtual ~FakeMenu();

    void SetLogFont(LPLOGFONT plf = NULL);

    FakeMenu* GetRoot();

    INT IdFromIndex(INT iItem);
    INT IndexFromId(INT nID, FakeMenu** ppOwner = NULL);
    FakeMenuItem* GetItem(INT iItem, BOOL bByPosition = TRUE, FakeMenu** ppOwner = NULL);
    BOOL GetItemRect(INT iItem, LPRECT prc, BOOL bByPosition = TRUE);
    BOOL GetItemText(INT iItem, LPWSTR pszText, INT cchText, BOOL bByPosition = TRUE);
    FakeMenu* GetSubMenu(INT iItem, BOOL bByPosition = TRUE);

    BOOL CheckItem(INT iItem, UINT uCheck = MF_BYPOSITION | MF_CHECKED);
    BOOL CheckRadioItem(INT iFirst, INT iLast, INT iCheck, BOOL bByPosition = TRUE);
    BOOL EnableItem(INT iItem, UINT uEnable = MF_BYPOSITION | MF_ENABLED);

    BOOL AddString(UINT nID, LPCWSTR text, UINT fState = MFS_ENABLED);
    INT AppendItem(const MENUITEMINFO* pmii);
    void DeleteItems();

    INT GetCurSel();
    void SetCurSel(HWND hwnd, INT iSelected);

    INT HitTest(INT x, INT y);

    INT TrackPopup(HWND hwndNotify, POINT pt, BOOL fKeyboard = FALSE, LPCRECT prcExclude = NULL);
    VOID HideTree(INT idResult);
    VOID HideTreeDelay(INT idResult, HWND hwndDelay);
    void DestroyTree(INT idResult);

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
    void OnShowWindow(HWND hwnd, BOOL fShow, UINT status);
    void OnPaint(HWND hwnd);
    void OnMouseMove(HWND hwnd, INT x, INT y, UINT keyFlags);
    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
    void OnLButtonUp(HWND hwnd, INT x, INT y, UINT keyFlags);
    void OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
    void OnRButtonUp(HWND hwnd, int x, int y, UINT flags);
    void OnButtonDown(HWND hwnd, INT x, INT y, BOOL fDoubleClick);
    void OnButtonUp(HWND hwnd, INT x, INT y);
    void OnKey(HWND hwnd, UINT vk, BOOL fDown, INT cRepeat, UINT flags);
    void OnChar(HWND hwnd, TCHAR ch, int cRepeat);
    void OnSysChar(HWND hwnd, TCHAR ch, int cRepeat);
    void OnTimer(HWND hwnd, UINT id);
    void OnDestroy(HWND hwnd);
    void OnReturn();
    void OnLeft();
    void OnRight();
    void OnEscape();

    BOOL IsAlive();
    INT FindItemByAccessChar(TCHAR ch);
    void DoMessageLoop(MSG& msg);
};

// static variables
static FakeMenu* s_pActiveMenu = NULL;
static HWND s_hwndOldActive = NULL;
static HWND s_hwndOldForeground = NULL;

//////////////////////////////////////////////////////////////////////////////////////////////
// FakeMenuItem impl

FakeMenuItem::FakeMenuItem(const MENUITEMINFO* pmii)
{
    m_nID = 0;
    m_fType = pmii->fType;
    m_fState = pmii->fState;
    m_pszText = NULL;
    m_pSubMenu = NULL;
    m_pNext = m_pPrev = NULL;

    if (!(pmii->fType & MFT_SEPARATOR))
    {
        m_nID = pmii->wID;
        m_pszText = _wcsdup((LPCTSTR)pmii->dwTypeData);
    }

    SetRectEmpty(&m_rcItem);
}

FakeMenuItem::~FakeMenuItem()
{
    free(m_pszText);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// FakeMenu impl

/*static*/ BOOL FakeMenu::DoRegisterClass(VOID)
{
    // register the window class
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = FakeMenu::WindowProc;
    wc.hInstance = ::GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = NULL;
    wc.hbrBackground = ::GetSysColorBrush(COLOR_MENU);
    wc.lpszClassName = FAKEMENU_CLASSNAME;
    return !!::RegisterClassExW(&wc);
}

BOOL FakeMenu::DoMeasureItem(INT iItem, FakeMenuItem* pItem, LPMEASUREITEMSTRUCT pMeasure)
{
    if (pItem->IsSep()) // Separator?
    {
        pMeasure->itemHeight = FAKEMENU_CY_SEP;
        return TRUE;
    }

    if (HDC hdc = ::GetDC(m_hwnd))
    {
        HGDIOBJ hFontOld = ::SelectObject(hdc, m_hFont);

        // Get text height
        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);

        // Get text extent
        SIZE size;
        ::GetTextExtentPoint32W(hdc, pItem->m_pszText, lstrlenW(pItem->m_pszText), &size);

        // Calculate width and height of item
        INT itemWidth = GetSystemMetrics(SM_CXMENUCHECK) + size.cx +
                        (2 * FAKEMENU_MARGIN) + FAKEMENU_CX_RIGHT_SPACE;
        INT itemHeight = tm.tmHeight + 2 * FAKEMENU_MARGIN;

        // Adjust the height
        INT cyMenuCheck = ::GetSystemMetrics(SM_CYMENUCHECK);
        if (itemHeight < cyMenuCheck + (2 * FAKEMENU_MARGIN))
            itemHeight = cyMenuCheck + (2 * FAKEMENU_MARGIN);

        pMeasure->itemWidth = itemWidth;
        pMeasure->itemHeight = itemHeight;

        ::SelectObject(hdc, hFontOld);
        ::ReleaseDC(m_hwnd, hdc);
    }

    return TRUE;
}

BOOL FakeMenu::DoDrawItem(INT iItem, FakeMenuItem* pItem, LPDRAWITEMSTRUCT pDraw)
{
    HDC hdc = pDraw->hDC;
    RECT rcItem = pDraw->rcItem;

    // The flags
    BOOL bSelected = (pDraw->itemState & ODS_SELECTED);
    BOOL bGrayed = (pDraw->itemState & (ODS_GRAYED | ODS_DISABLED));
    BOOL bSep = pItem->IsSep();
    BOOL bChecked = (pDraw->itemState & ODS_CHECKED);
    BOOL bSubMenu = !!pItem->m_pSubMenu;

    if (bSep) // Separator?
    {
        bSelected = bGrayed = bChecked = FALSE;
    }

    // Get the Part ID and state for window theme
    INT partid = MENU_POPUPITEM, state;
    if (bSep)
    {
        partid = MENU_POPUPSEPARATOR;
        state = MPI_NORMAL;
    }
    else if (bSelected)
    {
        if (bGrayed)
            state = MPI_DISABLEDHOT;
        else
            state = MPI_HOT;
    }
    else
    {
        if (bGrayed)
            state = MPI_DISABLED;
        else
            state = MPI_NORMAL;
    }

    // Draw background using theme
    ::DrawThemeBackground(m_hTheme, hdc, partid, state, &rcItem, NULL);

    if (bSep)
        return TRUE; // The separator is drawn. Done.

    // Trim the margin
    rcItem.left += m_marginsItem.cxLeftWidth;
    rcItem.right -= m_marginsItem.cxRightWidth;
    rcItem.top += m_marginsItem.cyTopHeight;
    rcItem.bottom -= m_marginsItem.cyBottomHeight;

    if (bChecked) // Draw checkmark or radio bullet?
    {
        RECT rcCheck = rcItem;
        rcCheck.right = rcCheck.left + ::GetSystemMetrics(SM_CXMENUCHECK) + 2 * FAKEMENU_CX_SEP;
        if (pItem->m_fType & MFT_RADIOCHECK)
            ::DrawThemeBackground(m_hTheme, hdc, MENU_POPUPCHECK, MC_BULLETNORMAL, &rcCheck, &rcCheck);
        else
            ::DrawThemeBackground(m_hTheme, hdc, MENU_POPUPCHECK, MC_CHECKMARKNORMAL, &rcCheck, &rcCheck);
    }

    if (bSubMenu) // Draw sub-menu?
    {
        RECT rcArrow = rcItem;
        rcArrow.left = rcArrow.right - FAKEMENU_CX_RIGHT_SPACE + 2 * FAKEMENU_CX_SEP;
        state = (bGrayed ? MSM_DISABLED : MSM_NORMAL);
        ::DrawThemeBackground(m_hTheme, hdc, MENU_POPUPSUBMENU, state, &rcArrow, &rcArrow);
    }

    if (pItem->m_pszText) // Draw text?
    {
        RECT rcText = rcItem;
        rcText.left += ::GetSystemMetrics(SM_CXMENUCHECK) + FAKEMENU_CX_SEP;
        ::InflateRect(&rcText, -FAKEMENU_MARGIN, -FAKEMENU_MARGIN);

        UINT dwFlags = DT_SINGLELINE | DT_LEFT | DT_VCENTER;
        ::SelectObject(hdc, m_hFont);
        ::DrawThemeText(m_hTheme, hdc, MENU_POPUPITEM, state, pItem->m_pszText, -1, dwFlags, 0, &rcText);
    }

    return TRUE;
}

VOID FakeMenu::InitStatus()
{
    m_fDone = FALSE;
    m_fDestroying = FALSE;
    m_fDelayed = FALSE;
    m_idResult = 0;
    m_iOpenSubMenu = -1;
    m_iSelected = -1;
}

FakeMenu::FakeMenu()
    : m_hwnd(NULL)
    , m_hwndNotify(NULL)
    , m_fKeyboardUsing(FALSE)
    , m_hTheme(NULL)
    , m_cItems(0)
    , m_pItems(NULL)
    , m_pParent(NULL)
    , m_hFont(GetStockFont(DEFAULT_GUI_FONT))
    , m_iOpenSubMenu(0)
    , m_iParentItem(-1)
{
    ZeroMemory(&m_marginsItem, sizeof(m_marginsItem));

    InitStatus();
}

FakeMenu::FakeMenu(HMENU hMenu, FakeMenu* pParent/* = NULL*/)
    : m_hwnd(NULL)
    , m_hwndNotify(NULL)
    , m_fKeyboardUsing(FALSE)
    , m_hTheme(NULL)
    , m_cItems(0)
    , m_pItems(NULL)
    , m_pParent(pParent)
    , m_hFont(GetStockFont(DEFAULT_GUI_FONT))
    , m_iOpenSubMenu(0)
    , m_iParentItem(-1)
{
    ZeroMemory(&m_marginsItem, sizeof(m_marginsItem));

    InitStatus();

    INT cItems = ::GetMenuItemCount(hMenu);
    if (cItems == -1)
    {
        assert(0);
        return;
    }

    // Populate the items
    for (INT iItem = 0; iItem < cItems; ++iItem)
    {
        TCHAR szText[128];
        MENUITEMINFO mii =
        {
            sizeof(mii),
            MIIM_TYPE | MIIM_ID | MIIM_DATA | MIIM_STATE | MIIM_SUBMENU
        };

        szText[0] = 0;

        mii.cch = _countof(szText);
        mii.dwTypeData = szText;
        ::GetMenuItemInfo(hMenu, iItem, TRUE, &mii);

        AppendItem(&mii);

        if (mii.hSubMenu) // Sub-menu?
        {
            auto pItem = GetItem(iItem);
            if (pItem)
            {
                auto pSubMenu = FakeMenu::FromHMENU(mii.hSubMenu, this);
                pSubMenu->m_iParentItem = iItem;

                LOGFONT lf;
                ::GetObject(m_hFont, sizeof(lf), &lf);
                pSubMenu->SetLogFont(&lf);

                pItem->m_pSubMenu = pSubMenu;
            }
        }
    }
}

void FakeMenu::SetLogFont(LPLOGFONT plf)
{
    if (m_hFont)
        ::DeleteObject(m_hFont);

    if (plf)
        m_hFont = ::CreateFontIndirect(plf);
    else
        m_hFont = GetStockFont(DEFAULT_GUI_FONT);

    for (INT i = 0; i < m_cItems; ++i)
    {
        auto pSubMenu = GetSubMenu(i);
        if (pSubMenu)
            pSubMenu->SetLogFont(plf);
    }
}

/*static*/ FakeMenu* FakeMenu::FromHWND(HWND hwnd)
{
    if (!::IsWindow(hwnd))
        return NULL;

    DWORD dwPID;
    ::GetWindowThreadProcessId(hwnd, &dwPID);
    if (dwPID != ::GetCurrentProcessId()) // same process?
        return NULL;

    return (FakeMenu*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

/*static*/ FakeMenu* FakeMenu::FromHMENU(HMENU hMenu, FakeMenu* pParent/* = NULL*/)
{
    return new FakeMenu(hMenu, pParent);
}

FakeMenu::~FakeMenu()
{
    DeleteItems();
    ::DeleteObject(m_hFont);
}

FakeMenu* FakeMenu::GetRoot()
{
    auto pMenu = this;
    while (pMenu->m_pParent)
    {
        pMenu = pMenu->m_pParent;
    }
    return pMenu;
}

FakeMenuItem*
FakeMenu::GetItem(INT iItem, BOOL bByPosition/* = TRUE*/, FakeMenu** ppOwner/* = NULL*/)
{
    if (ppOwner)
        *ppOwner = this;

    if (bByPosition)
    {
        if (iItem < 0 || m_cItems <= iItem)
            return NULL;

        auto pItem = m_pItems;
        for (INT i = 0; i < m_cItems; ++i)
        {
            if (iItem == i)
                return pItem;

            pItem = pItem->m_pNext;
        }
    }
    else
    {
        FakeMenu* pOwner = NULL;
        iItem = IndexFromId(iItem, &pOwner);
        if (iItem >= 0)
        {
            if (ppOwner)
                *ppOwner = pOwner;
            return pOwner->GetItem(iItem, TRUE);
        }
    }

    return NULL;
}

FakeMenu* FakeMenu::GetSubMenu(INT iItem, BOOL bByPosition/* = TRUE*/)
{
    auto pItem = GetItem(iItem, bByPosition);
    if (pItem && pItem->m_pSubMenu)
        return pItem->m_pSubMenu;
    return NULL;
}

BOOL FakeMenu::EnableItem(INT iItem, UINT uEnable/* = MF_BYPOSITION | MF_ENABLED*/)
{
    BOOL bByPosition = (uEnable & MF_BYPOSITION);
    auto pItem = GetItem(iItem, bByPosition);
    if (!pItem)
        return FALSE;

    if (uEnable & (MF_GRAYED | MFS_DISABLED))
        pItem->m_fState |= (MFS_GRAYED | MFS_DISABLED);
    else
        pItem->m_fState &= ~(MFS_GRAYED | MFS_DISABLED);

    return TRUE;
}

BOOL FakeMenu::CheckItem(INT iItem, UINT uCheck/* = MF_BYPOSITION | MF_CHECKED*/)
{
    BOOL bByPosition = (uCheck & MF_BYPOSITION);

    auto pItem = GetItem(iItem, bByPosition);
    if (!pItem)
        return FALSE;

    if (uCheck & MF_CHECKED)
        pItem->m_fState |= MFS_CHECKED;
    else
        pItem->m_fState &= ~MFS_CHECKED;

    pItem->m_fType &= ~MFT_RADIOCHECK;
    return TRUE;
}

BOOL FakeMenu::CheckRadioItem(INT iFirst, INT iLast, INT iCheck, BOOL bByPosition/* = TRUE*/)
{
    auto pOwner = this;

    if (!bByPosition)
    {
        FakeMenu *pOwner1 = NULL;
        FakeMenu *pOwner2 = NULL;
        FakeMenu *pOwner3 = NULL;
        iFirst = IndexFromId(iFirst, &pOwner1);
        iLast = IndexFromId(iLast, &pOwner2);
        iCheck = IndexFromId(iCheck, &pOwner3);
        if (pOwner1 == NULL || pOwner1 != pOwner2 || pOwner2 != pOwner3)
            return FALSE;
        pOwner = pOwner1;
    }

    for (INT i = iFirst; i <= iLast; ++i)
    {
        auto pItem = pOwner->GetItem(i, TRUE);
        if (pItem)
        {
            pItem->m_fType |= MFT_RADIOCHECK;
            if (i == iCheck)
                pItem->m_fState |= MFS_CHECKED;
            else
                pItem->m_fState &= ~MFS_CHECKED;
        }
    }

    return TRUE;
}

INT FakeMenu::IdFromIndex(INT iItem)
{
    auto pItem = GetItem(iItem);
    if (pItem)
        return pItem->m_nID;
    return 0;
}

INT FakeMenu::IndexFromId(INT nID, FakeMenu** ppOwner/* = NULL*/)
{
    if (ppOwner)
        *ppOwner = NULL;

    if (nID == 0) // Invalid ID?
        return -1;

    // Search the items
    auto pItem = m_pItems;
    for (INT iItem = 0; iItem < m_cItems; iItem++)
    {
        if (pItem->m_nID == nID) // Matched?
        {
            if (ppOwner)
            {
                *ppOwner = this;
                return iItem;
            }
        }
        pItem = pItem->m_pNext;
    }

    // Search sub-menus
    for (INT iItem = 0; iItem < m_cItems; iItem++)
    {
        auto pSubMenu = GetSubMenu(iItem, TRUE);
        if (pSubMenu)
        {
            INT i = pSubMenu->IndexFromId(nID, ppOwner);
            if (i >= 0)
                return i;
        }
    }

    return -1;
}

BOOL FakeMenu::GetItemRect(INT iItem, LPRECT prc, BOOL bByPosition/* = TRUE*/)
{
    auto pItem = GetItem(iItem, bByPosition);
    if (pItem)
    {
        *prc = pItem->m_rcItem;
        return TRUE;
    }
    SetRectEmpty(prc);
    return FALSE;
}

BOOL FakeMenu::GetItemText(INT iItem, LPWSTR pszText, INT cchText, BOOL bByPosition/* = TRUE*/)
{
    auto pItem = GetItem(iItem, bByPosition);
    if (pItem)
    {
        lstrcpynW(pszText, pItem->m_pszText, cchText);
        return TRUE;
    }
    return FALSE;
}

INT FakeMenu::AppendItem(const MENUITEMINFO* pmii)
{
    auto pItem = new FakeMenuItem(pmii);

    // m_pItems is a cyclic linked list
    if (m_pItems)
    {
        auto pLast = m_pItems->m_pPrev;
        pItem->m_pNext = m_pItems;
        pItem->m_pPrev = pLast;
        m_pItems->m_pPrev = pItem;
        pLast->m_pNext = pItem;
    }
    else
    {
        pItem->m_pNext = pItem->m_pPrev = pItem;
        m_pItems = pItem;
    }

    ++m_cItems;
    return TRUE;
}

void FakeMenu::UpdateVisuals(HWND hwnd)
{
    if (m_hTheme)
    {
        ::CloseThemeData(m_hTheme);
        m_hTheme = NULL;
    }

    m_hTheme = ::OpenThemeData(hwnd, VSCLASS_MENU);
    assert(m_hTheme != NULL);
    ::GetThemeMargins(m_hTheme, NULL, MENU_POPUPITEM, 0, TMT_CONTENTMARGINS, NULL, &m_marginsItem);

    // Force to repaint
    ::InvalidateRect(hwnd, NULL, TRUE);
}

BOOL FakeMenu::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    UpdateVisuals(hwnd);
    return TRUE;
}

void FakeMenu::OnShowWindow(HWND hwnd, BOOL fShow, UINT status)
{
    if (fShow)
        ::SetTimer(hwnd, FAKEMENU_REFRESH_TIMER, FAKEMENU_REFRESH_INTERVAL, NULL);
    else
        ::KillTimer(hwnd, FAKEMENU_REFRESH_TIMER);
}

void FakeMenu::OnTimer(HWND hwnd, UINT id)
{
    if (id == FAKEMENU_ANIMATION_TIMER) // Animation?
    {
        KillTimer(hwnd, FAKEMENU_ANIMATION_TIMER);
        ::ShowWindow(m_hwnd, SW_HIDE);

        s_pActiveMenu = NULL;

        m_fDelayed = FALSE;
        m_fDone = TRUE;
        ::SetWindowRgn(m_hwnd, NULL, TRUE);
        return;
    }

    if (m_fDelayed)
        return;

    if (id == FAKEMENU_REFRESH_TIMER) // Refresh?
    {
        if (!m_fKeyboardUsing) // Mouse moving?
        {
            POINT pt;
            ::GetCursorPos(&pt);
            ::ScreenToClient(hwnd, &pt); // Convert to client coordinates

            INT iSelected = HitTest(pt.x, pt.y);
            SetCurSel(hwnd, iSelected); // Select it
        }

        // Check foreground window
        HWND hwndForeground = ::GetForegroundWindow();
        if (hwndForeground && hwndForeground != s_hwndOldForeground)
        {
            auto pRoot = GetRoot();
            if (pRoot && !pRoot->m_fDestroying)
                pRoot->HideTree(pRoot->m_idResult); // Hide it
        }
    }
}

void FakeMenu::DestroyTree(INT idResult)
{
    if (m_fDestroying) // Destroying the window?
        return;

    m_fDestroying = TRUE;
    m_idResult = idResult;

    // Destroy the sub-menu windows
    for (INT iItem = 0; iItem < m_cItems; ++iItem)
    {
        auto pSubMenu = GetSubMenu(iItem, TRUE);
        if (pSubMenu)
        {
            pSubMenu->DestroyTree(idResult);
        }
    }

    ::DestroyWindow(m_hwnd);
    m_fDone = TRUE;
}

void FakeMenu::OnDestroy(HWND hwnd)
{
    KillTimer(hwnd, FAKEMENU_REFRESH_TIMER);

    auto pRoot = GetRoot();
    if (pRoot)
    {
        pRoot->HideTree(m_idResult);
        pRoot->DestroyTree(m_idResult);
    }

    if (m_pParent)
        m_pParent->m_iOpenSubMenu = -1;

    if (m_hTheme)
    {
        CloseThemeData(m_hTheme);
        m_hTheme = NULL;
    }
}

void FakeMenu::OnPaint(HWND hwnd)
{
    // Start the painting
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    if (!hdc)
    {
        assert(0);
        return;
    }

    // For all items...
    auto pItem = m_pItems;
    for (INT iItem = 0; iItem < m_cItems; iItem++)
    {
        if (RectVisible(hdc, &pItem->m_rcItem))
        {
            DRAWITEMSTRUCT DrawItem = { ODT_MENU };
            DrawItem.itemAction = ODA_DRAWENTIRE;

            // Calculate the item state
            DrawItem.itemState = 0;
            if (iItem == m_iSelected)
                DrawItem.itemState |= ODS_SELECTED;
            if (pItem->m_fState & MFS_CHECKED)
                DrawItem.itemState |= ODS_CHECKED;
            if (pItem->m_fState & MFS_GRAYED)
                DrawItem.itemState |= ODS_DISABLED;

            DrawItem.hwndItem = m_hwnd;
            DrawItem.hDC = hdc;
            DrawItem.rcItem = pItem->m_rcItem;
            DrawItem.itemData = (DWORD_PTR)pItem;

            // Draw the item
            DoDrawItem(iItem, pItem, &DrawItem);
        }

        pItem = pItem->m_pNext;
    }

    // End the painting
    EndPaint(hwnd, &ps);
}

INT FakeMenu::GetCurSel()
{
    return m_iSelected;
}

void FakeMenu::SetCurSel(HWND hwnd, INT iSelected)
{
    RECT rc;

    if (m_iSelected == iSelected)
        return;

    if (m_iSelected >= 0) // Old value?
    {
        GetItemRect(m_iSelected, &rc);
        ::InvalidateRect(hwnd, &rc, TRUE); // Repainting
    }

    m_iSelected = iSelected; // Update selection

    if (m_iSelected >= 0) // New value
    {
        GetItemRect(m_iSelected, &rc);
        ::InvalidateRect(hwnd, &rc, TRUE); // Repainting
    }
}

INT FakeMenu::HitTest(INT x, INT y)
{
    POINT pt = { x, y };

    auto pItem = m_pItems;
    for (INT iItem = 0; iItem < m_cItems; ++iItem)
    {
        if (!pItem->IsSep() && !pItem->IsGrayed())
        {
            if (PtInRect(&pItem->m_rcItem, pt))
                return iItem; // Found!
        }

        pItem = pItem->m_pNext;
    }

    return -1; // Not found
}

void FakeMenu::OnMouseMove(HWND hwnd, INT x, INT y, UINT keyFlags)
{
    m_fKeyboardUsing = FALSE;

    INT iSelected = HitTest(x, y);
    SetCurSel(hwnd, iSelected);
}

void FakeMenu::OnButtonDown(HWND hwnd, INT x, INT y, BOOL fDoubleClick)
{
    if (fDoubleClick)
        return;

    POINT pt = { x, y };
    INT iItem = HitTest(x, y);
    auto pItem = GetItem(iItem, TRUE);
    if (!pItem || pItem->IsSep() || pItem->IsGrayed())
        return; // The action is disabled

    SetCurSel(hwnd, iItem); // Select it now

    auto pSubMenu = pItem->m_pSubMenu;
    if (!pSubMenu)
        return; // No sub-menu

    // Get the item rect in screen coordinates
    RECT rcItem = pItem->m_rcItem;
    pt.x = rcItem.right;
    pt.y = rcItem.top;
    ::ClientToScreen(m_hwnd, &pt);
    MapWindowRect(m_hwnd, NULL, &rcItem);

    // Open the sub-menu
    pSubMenu->TrackPopup(m_hwndNotify, pt, FALSE, &rcItem);
}

void FakeMenu::OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    OnButtonDown(hwnd, x, y, fDoubleClick);
}

void FakeMenu::OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    OnButtonDown(hwnd, x, y, fDoubleClick);
}

// Is menu animation disabled?
static BOOL IsMenuAnimationDisabled(VOID)
{
    DWORD dwValue = FALSE;
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
                      0,
                      KEY_READ,
                      &hKey) == ERROR_SUCCESS)
    {
        DWORD cbValue = sizeof(dwValue);
        RegQueryValueExW(hKey, L"NoChangeAnimation", NULL, NULL,
                         (LPBYTE)&dwValue, &cbValue);
        RegCloseKey(hKey);
    }
    return dwValue;
}

void FakeMenu::OnButtonUp(HWND hwnd, INT x, INT y)
{
    INT iSelected = HitTest(x, y);
    if (iSelected == -1)
        return;

    SetCurSel(hwnd, iSelected); // Select it

    auto pItem = GetItem(iSelected);
    if (!pItem || pItem->IsSep() || pItem->m_pSubMenu)
        return; // The action is disabled

    RECT rc;
    GetWindowRect(hwnd, &rc);

    BOOL bDelay = !IsMenuAnimationDisabled();
    if (bDelay) // Animation?
    {
        // Get the item rect in window coordinates
        RECT rcItem = pItem->m_rcItem;
        MapWindowRect(hwnd, NULL, &rcItem);
        OffsetRect(&rcItem, -rc.left, -rc.top);

        // Create the window region of the item
        HRGN hRgn = CreateRectRgnIndirect(&rcItem);
        bDelay = SetWindowRgn(hwnd, hRgn, TRUE);
        if (!bDelay)
            DeleteObject(hRgn);
    }

    // Hide the tree from thr root
    INT idResult = IdFromIndex(iSelected);
    auto pRoot = GetRoot();
    if (pRoot)
    {
        if (bDelay)
            pRoot->HideTreeDelay(idResult, hwnd);
        else
            pRoot->HideTree(idResult);
    }
}

void FakeMenu::OnLButtonUp(HWND hwnd, INT x, INT y, UINT keyFlags)
{
    OnButtonUp(hwnd, x, y);
}

void FakeMenu::OnRButtonUp(HWND hwnd, int x, int y, UINT flags)
{
    OnButtonUp(hwnd, x, y);
}

void FakeMenu::OnReturn()
{
    if (m_iSelected < 0)
        return; // Not selected

    auto pItem = GetItem(m_iSelected);
    if (!pItem || pItem->IsSep() || pItem->IsGrayed())
        return; // The action is disabled

    auto pSubMenu = pItem->m_pSubMenu;
    if (pSubMenu) // Open sub-menu?
    {
        RECT rcItem = pItem->m_rcItem;
        POINT pt = { rcItem.right, rcItem.top };
        ::ClientToScreen(m_hwnd, &pt);
        ::MapWindowRect(m_hwnd, NULL, &rcItem);

        // Open the sub-menu
        pSubMenu->TrackPopup(m_hwndNotify, pt, TRUE, &rcItem);
    }
    else
    {
        // Choose the menu item
        INT idResult = IdFromIndex(m_iSelected);
        auto pRoot = GetRoot();
        if (pRoot)
        {
            pRoot->HideTree(idResult);
        }
    }
}

void FakeMenu::OnEscape()
{
    auto pRoot = GetRoot();
    if (pRoot)
        pRoot->HideTree(m_idResult);
}

void FakeMenu::OnLeft()
{
    ::ShowWindow(m_hwnd, SW_HIDE);
    if (s_pActiveMenu)
        s_pActiveMenu = s_pActiveMenu->m_pParent;
}

void FakeMenu::OnRight()
{
    if (m_iSelected < 0) // Not selected
        return;

    auto pItem = GetItem(m_iSelected);
    if (!pItem || pItem->IsSep() || pItem->IsGrayed())
        return; // The action is disabled

    auto pSubMenu = pItem->m_pSubMenu;
    if (!pSubMenu) // No sub-menu?
        return;

    // Get the item rectangle in screen coordinates
    RECT rcItem = pItem->m_rcItem;
    POINT pt = { rcItem.right, rcItem.top };
    ::ClientToScreen(m_hwnd, &pt);
    ::MapWindowRect(m_hwnd, NULL, &rcItem);

    // Open the sub-menu
    pSubMenu->TrackPopup(m_hwndNotify, pt, TRUE, &rcItem);
}

INT FakeMenu::FindItemByAccessChar(TCHAR ch)
{
    WCHAR szUpper[3] = { '&', ch, 0 };
    WCHAR szLower[3] = { '&', ch, 0 };
    ::CharUpperW(szUpper);
    ::CharLowerW(szLower);

    // Search the menu item by the access key
    auto pItem = m_pItems;
    for (INT iItem = 0; iItem < m_cItems; ++iItem)
    {
        if (pItem->m_pszText)
        {
            if (wcsstr(pItem->m_pszText, szUpper) != NULL ||
                wcsstr(pItem->m_pszText, szLower) != NULL)
            {
                return iItem; // Found
            }
        }
        pItem = pItem->m_pNext;
    }

    return -1;
}

void FakeMenu::OnSysChar(HWND hwnd, TCHAR ch, int cRepeat)
{
    INT iItem = FindItemByAccessChar(ch);
    SetCurSel(hwnd, iItem);
    OnReturn();
}

void FakeMenu::OnChar(HWND hwnd, TCHAR ch, int cRepeat)
{
    INT iItem = FindItemByAccessChar(ch);
    SetCurSel(hwnd, iItem);
    OnReturn();
}

void FakeMenu::OnKey(HWND hwnd, UINT vk, BOOL fDown, INT cRepeat, UINT flags)
{
    if (!fDown)
        return;

    m_fKeyboardUsing = TRUE;

    INT iItem;
    switch (vk)
    {
        case VK_ESCAPE:
            OnEscape();
            break;

        case VK_RETURN:
            OnReturn();
            break;

        case VK_LEFT:
            OnLeft();
            break;

        case VK_RIGHT:
            OnRight();
            break;

        case VK_UP:
            iItem = GetNextSelectable(m_iSelected, FALSE);
            SetCurSel(hwnd, iItem);
            break;

        case VK_DOWN:
            iItem = GetNextSelectable(m_iSelected, TRUE);
            SetCurSel(hwnd, iItem);
            break;
    }
}

LRESULT CALLBACK FakeMenu::WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_SHOWWINDOW, OnShowWindow);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);
        HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
        HANDLE_MSG(hwnd, WM_LBUTTONUP, OnLButtonUp);
        HANDLE_MSG(hwnd, WM_RBUTTONDOWN, OnRButtonDown);
        HANDLE_MSG(hwnd, WM_RBUTTONUP, OnRButtonUp);
        HANDLE_MSG(hwnd, WM_SYSKEYDOWN, OnKey);
        HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
        HANDLE_MSG(hwnd, WM_CHAR, OnChar);
        HANDLE_MSG(hwnd, WM_SYSCHAR, OnSysChar);
        HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
        HANDLE_MSG(hwnd, WM_TIMER, OnTimer);

        case WM_MOUSEACTIVATE:
            return MA_NOACTIVATE; // Don't activate the window!

        case WM_THEMECHANGED:
        case WM_SETTINGCHANGE:
            UpdateVisuals(hwnd);
            break;

        default:
            return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

/*static*/ LRESULT CALLBACK
FakeMenu::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto pThis = (FakeMenu*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (uMsg == WM_CREATE)
    {
        auto pCS = (CREATESTRUCT*)lParam;
        pThis = (FakeMenu*)pCS->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hwnd = hwnd;
    }
    else
    {
        if (pThis == NULL)
            return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    LRESULT ret = pThis->WindowProcDx(hwnd, uMsg, wParam, lParam);
    if (uMsg == WM_NCDESTROY)
    {
        pThis->m_hwnd = NULL;
    }
    return ret;
}

void FakeMenu::ChooseLocation(POINT& pt, INT cx, INT cy, LPCRECT prcExclude)
{
    HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMon, &mi);
    INT x = pt.x, y = pt.y;

    if (prcExclude)
    {
        if (mi.rcMonitor.right < prcExclude->right + cx)
            x = prcExclude->left - cx;
        else
            x = prcExclude->right;

        if (mi.rcMonitor.bottom < prcExclude->top + cy)
            y = prcExclude->bottom - cy;
        else
            y = prcExclude->top;

        if (x < mi.rcMonitor.left)
            x = mi.rcMonitor.left;

        if (y < mi.rcMonitor.top)
            y = mi.rcMonitor.top;
    }
    else
    {
        if (mi.rcWork.right < x + cx)
            x -= cx;

        if (mi.rcWork.bottom < y + cy)
            y -= cy;

        if (x < mi.rcWork.left)
            x = mi.rcWork.left;

        if (y < mi.rcWork.top)
            y = mi.rcWork.top;
    }

    pt.x = x;
    pt.y = y;
}

BOOL FakeMenu::AddString(UINT nID, LPCWSTR text, UINT fState/* = MFS_ENABLED*/)
{
    MENUITEMINFO mii = { sizeof(mii), MIIM_ID | MIIM_STATE | MIIM_TYPE | MIIM_DATA };
    if (text && text[0])
    {
        mii.fType = MFT_STRING;
        mii.dwTypeData = const_cast<LPWSTR>(text);
    }
    else
    {
        mii.fType = MFT_SEPARATOR;
    }
    mii.fState = fState;
    mii.wID = nID;
    return AppendItem(&mii);
}

void FakeMenu::DeleteItems()
{
    if (m_pItems && m_cItems)
    {
        auto pItem = m_pItems;
        for (INT iItem = 0; iItem < m_cItems; ++iItem)
        {
            auto pNext = pItem->m_pNext;
            delete pItem->m_pSubMenu;
            delete pItem;
            pItem = pNext;
        }
    }

    m_pItems = NULL;
    m_cItems = 0;
}

void FakeMenu::MeasureItems(SIZE& size)
{
    size.cx = size.cy = 0;

    auto pItem = m_pItems;
    for (INT iItem = 0; iItem < m_cItems; ++iItem)
    {
        // Measure the item
        MEASUREITEMSTRUCT MeasureItem = { ODT_MENU };
        MeasureItem.itemID = iItem;
        MeasureItem.itemData = (DWORD_PTR)pItem;
        MeasureItem.itemWidth = size.cx;
        MeasureItem.itemHeight = GetSystemMetrics(SM_CYMENU);
        DoMeasureItem(iItem, pItem, &MeasureItem);

        // Update the width
        if (size.cx < (LONG)MeasureItem.itemWidth)
            size.cx = (LONG)MeasureItem.itemWidth;

        // Set the rectangle
        pItem->m_rcItem.left = 0;
        pItem->m_rcItem.right = size.cx;
        pItem->m_rcItem.top = size.cy;
        pItem->m_rcItem.bottom = size.cy + MeasureItem.itemHeight;

        // Update height of the contents
        size.cy += MeasureItem.itemHeight;

        pItem = pItem->m_pNext;
    }

    // Update the right of the item rectangles
    pItem = m_pItems;
    for (INT iItem = 0; iItem < m_cItems; ++iItem)
    {
        pItem->m_rcItem.right = size.cx;

        pItem = pItem->m_pNext;
    }
}

VOID FakeMenu::HideTree(INT idResult)
{
    m_idResult = idResult; // Set the result ID

    // Hide the self
    ::ShowWindow(m_hwnd, SW_HIDE);

    // Hide the sub-menu
    for (INT iItem = 0; iItem < m_cItems; ++iItem)
    {
        auto pSubMenu = GetSubMenu(iItem);
        if (pSubMenu)
        {
            pSubMenu->HideTree(idResult);
        }
    }

    // Update s_pActiveMenu if necessary
    if (s_pActiveMenu == this)
    {
        s_pActiveMenu = m_pParent;
    }
}

VOID FakeMenu::HideTreeDelay(INT idResult, HWND hwndDelay)
{
    m_idResult = idResult;
    m_fDelayed = TRUE;

    // Hide the sub-menus with delay
    for (INT iItem = 0; iItem < m_cItems; ++iItem)
    {
        auto pSubMenu = GetSubMenu(iItem);
        if (pSubMenu)
        {
            pSubMenu->HideTreeDelay(idResult, hwndDelay);
        }
    }

    if (m_hwnd == hwndDelay) // The self is to be animated?
    {
        ::KillTimer(m_hwnd, FAKEMENU_REFRESH_TIMER);
        ::SetTimer(m_hwnd, FAKEMENU_ANIMATION_TIMER, FAKEMENU_ANIMATION_DELAY, NULL);
    }
    else
    {
        ::ShowWindow(m_hwnd, SW_HIDE); // Hide immediately
    }
}

// Helper structure for FakeMenu::EnumCloseProc
struct PRE_POPUP
{
    FakeMenu* pThis;
    BOOL bFound;
};

/*static*/ BOOL CALLBACK FakeMenu::EnumCloseProc(HWND hwnd, LPARAM lParam)
{
    auto pPrePopup = (PRE_POPUP*)lParam;

    // Get the class name of hwnd
    WCHAR szClass[64];
    if (!::GetClassNameW(hwnd, szClass, _countof(szClass)))
        return TRUE;

    if (lstrcmpiW(szClass, L"#32768") == 0 && ::IsWindowVisible(hwnd)) // standard visible menu
    {
        ::SendMessageW(hwnd, WM_CLOSE, 0, 0);
        pPrePopup->bFound = TRUE;
        return TRUE;
    }

    if (lstrcmpiW(szClass, FAKEMENU_CLASSNAME) == 0)
    {
        if (pPrePopup->pThis->IsFamilyHWND(hwnd))
            return TRUE; // Our family

        ::SendMessageW(hwnd, WM_CLOSE, 0, 0); // Close it
        pPrePopup->bFound = TRUE;
    }

    return TRUE;
}

/*static*/ BOOL CALLBACK FakeMenu::EnumFindStdMenuProc(HWND hwnd, LPARAM lParam)
{
    auto pbFound = (BOOL*)lParam;

    // Get the class name of hwnd
    WCHAR szClass[64];
    if (!::GetClassNameW(hwnd, szClass, _countof(szClass)))
        return TRUE;

    if (lstrcmpiW(szClass, L"#32768") == 0) // standard menu
    {
        if (::IsWindowVisible(hwnd))
        {
            *pbFound = TRUE; // We found a standard visible menu
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL PtInTaskNotify(POINT pt)
{
    RECT rc;

    HWND hwndTrayWnd = ::FindWindowW(L"Shell_TrayWnd", NULL);
    HWND hNotifyWnd = ::FindWindowExW(hwndTrayWnd, NULL, L"TrayNotifyWnd", NULL);
    ::GetWindowRect(hNotifyWnd, &rc);
    if (::PtInRect(&rc, pt))
        return TRUE;

    HWND hwndOverflow = ::FindWindowW(L"NotifyIconOverflowWindow", NULL);
    ::GetWindowRect(hwndOverflow, &rc);
    if (::PtInRect(&rc, pt))
        return TRUE;

    return FALSE;
}

// Keep tracking or not?
BOOL FakeMenu::IsAlive()
{
    if (m_fDone || !s_pActiveMenu)
        return FALSE;

    if (::GetActiveWindow() != s_hwndOldActive)
        return FALSE;

    BOOL bFound = FALSE;
    ::EnumWindows(EnumFindStdMenuProc, (LPARAM)&bFound);
    if (bFound)
        return FALSE;

    auto pRoot = GetRoot();
    if (!pRoot)
        return FALSE;

    // Mouse action?
    if (::GetKeyState(VK_LBUTTON) < 0 ||
        ::GetKeyState(VK_MBUTTON) < 0 ||
        ::GetKeyState(VK_RBUTTON) < 0)
    {
        DWORD dwPos = ::GetMessagePos();
        POINT pt = { LOWORD(dwPos), HIWORD(dwPos) };
        HWND hwndPt = ::WindowFromPoint(pt);
        if (!IsFamilyHWND(hwndPt) && !PtInTaskNotify(pt))
            return FALSE; // Not our family
    }

    // Keyboard action?
    if (::GetKeyState(VK_ESCAPE) < 0)
        return FALSE;

    if (pRoot->m_fDelayed || ::IsWindowVisible(pRoot->m_hwnd))
        return TRUE;

    return FALSE;
}

void FakeMenu::DoMessageLoop(MSG& msg)
{
    for (;;)
    {
        while (!::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
        {
            if (!IsAlive())
                return;
            ::Sleep(80);
        }

        if (!::GetMessage(&msg, NULL, 0, 0))
            return;

        switch (msg.message)
        {
        case WM_LBUTTONDBLCLK:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_NCLBUTTONDBLCLK:
        case WM_NCLBUTTONDOWN:
        case WM_NCLBUTTONUP:
        case WM_NCMBUTTONDBLCLK:
        case WM_NCMBUTTONDOWN:
        case WM_NCMBUTTONUP:
        case WM_NCRBUTTONDBLCLK:
        case WM_NCRBUTTONDOWN:
        case WM_NCRBUTTONUP:
            // Mouse action!
            if (!IsFamilyHWND(msg.hwnd))
            {
                ::SendMessageW(m_hwnd, WM_CLOSE, 0, 0);
                if (s_hwndOldActive == msg.hwnd)
                {
                    ::DispatchMessage(&msg);
                }
                continue;
            }
            break;

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
        case WM_DEADCHAR:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_SYSCHAR:
        case WM_SYSDEADCHAR:
            // Keyboard action!
            if (s_pActiveMenu)
                msg.hwnd = s_pActiveMenu->m_hwnd;
            else
                msg.hwnd = m_hwnd;
            break;
        }

        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);

        if (!IsAlive())
            return;
    }
}

INT FakeMenu::TrackPopup(HWND hwndNotify, POINT pt, BOOL fKeyboard, LPCRECT prcExclude)
{
    s_pActiveMenu = this;

    // Close the other menus if necessary
    PRE_POPUP PrePopup = { this, FALSE };
    ::EnumWindows(EnumCloseProc, (LPARAM)&PrePopup);
    if (PrePopup.bFound)
        ::Sleep(50);

    // Save the active window and the foreground window To detect mouse actions
    s_hwndOldActive = ::GetActiveWindow();
    s_hwndOldForeground = ::GetForegroundWindow();

    InitStatus();

    m_hwndNotify = hwndNotify;
    m_fKeyboardUsing = fKeyboard;

    DWORD style = WS_POPUP | WS_BORDER; // Popup with border
    DWORD exstyle =
        WS_EX_TOPMOST |         // Always show on top
        WS_EX_NOACTIVATE |      // Always don't activate
        WS_EX_TOOLWINDOW |      // Don't show the taskbar pane
        WS_EX_DLGMODALFRAME |   // With dialog frame
        WS_EX_WINDOWEDGE;       // With window edge

    /* Measure items */
    SIZE size;
    MeasureItems(size);

    RECT rc = { 0, 0, size.cx, size.cy };
    ::AdjustWindowRectEx(&rc, style, FALSE, exstyle);

    size.cx = rc.right - rc.left;
    size.cy = rc.bottom - rc.top;
    ChooseLocation(pt, size.cx, size.cy, prcExclude);

    // Create or move?
    if (!m_hwnd)
    {
        HWND hwnd = ::CreateWindowExW(exstyle, FAKEMENU_CLASSNAME, FAKEMENU_CLASSNAME, style,
                                      pt.x, pt.y, size.cx, size.cy,
                                      NULL, NULL, GetModuleHandle(NULL), this);
        assert(m_hwnd == hwnd);
    }
    else
    {
        ::SetWindowPos(m_hwnd, HWND_TOPMOST, pt.x, pt.y, size.cx, size.cy,
                       SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    }

    if (m_pParent) // Sub-menu?
    {
        auto iOpenSubMenu = m_pParent->m_iOpenSubMenu;
        if (iOpenSubMenu != -1)
        {
            auto pSubMenu = m_pParent->GetSubMenu(iOpenSubMenu);
            if (pSubMenu && pSubMenu != this)
                pSubMenu->HideTree(0); // Another sub-menu to be hidden
        }
        m_pParent->m_iOpenSubMenu = m_iParentItem; // Update parent's m_iOpenSubMenu
    }

    // Show the window but don't activate it!
    ::ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);

    if (m_fKeyboardUsing)
        SetCurSel(m_hwnd, 0);

    if (!m_pParent) // Root?
    {
        MSG msg;
        DoMessageLoop(msg);

        HideTree(m_idResult); // Done. Hide the tree

        if (msg.message == WM_QUIT)
        {
            ::PostQuitMessage((INT)msg.wParam);
        }
        else if (m_idResult != 0 && m_hwndNotify)
        {
            ::PostMessageW(m_hwndNotify, WM_COMMAND, m_idResult, 0);
        }
    }

    return m_idResult;
}

INT FakeMenu::GetNextIndex(INT iItem, BOOL bNext)
{
    if (bNext)
    {
        ++iItem;
        if (iItem >= m_cItems)
            iItem = 0;
    }
    else
    {
        --iItem;
        if (iItem < 0)
            iItem = m_cItems - 1;
    }
    return iItem;
}

INT FakeMenu::GetNextSelectable(INT iItem, BOOL bNext)
{
    iItem = GetNextIndex(iItem, bNext);

    for (INT iTry = 0; iTry < m_cItems; ++iTry)
    {
        auto pItem = GetItem(iItem);
        if (pItem == NULL)
            return -1;

        if (!pItem->IsSep())
            return iItem;

        iItem = GetNextIndex(iItem, bNext);
    }

    return -1;
}

BOOL FakeMenu::IsFamilyHWND(HWND hwnd)
{
    auto pMenu = FakeMenu::FromHWND(hwnd);
    if (!pMenu)
        return FALSE;
    return GetRoot() == pMenu->GetRoot(); // The root is same?
}

//////////////////////////////////////////////////////////////////////////////////////////////
// helper functions

inline FakeMenu* HandleToFakeMenu(HFAKEMENU hFakeMenu)
{
    return reinterpret_cast<FakeMenu*>(hFakeMenu);
}

inline HFAKEMENU FakeMenuToHandle(FakeMenu* pFakeMenu)
{
    return reinterpret_cast<HFAKEMENU>(pFakeMenu);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// C interface

extern "C"
{

BOOL APIENTRY FakeMenu_InitInstance(VOID)
{
    return FakeMenu::DoRegisterClass();
}

VOID APIENTRY FakeMenu_ExitInstance(VOID)
{
}

HFAKEMENU APIENTRY FakeMenu_Create(VOID)
{
    return FakeMenuToHandle(new FakeMenu());
}

HFAKEMENU APIENTRY FakeMenu_FromHMENU(HMENU hMenu)
{
    return FakeMenuToHandle(FakeMenu::FromHMENU(hMenu));
}

VOID APIENTRY FakeMenu_Destroy(HFAKEMENU hFakeMenu)
{
    auto pFakeMenu = HandleToFakeMenu(hFakeMenu);
    pFakeMenu->DestroyTree(0);
    delete pFakeMenu;
}

VOID APIENTRY FakeMenu_SetLogFont(HFAKEMENU hFakeMenu, LPLOGFONT plf OPTIONAL)
{
    HandleToFakeMenu(hFakeMenu)->SetLogFont(plf);
}

BOOL APIENTRY FakeMenu_GetItemText(HFAKEMENU hFakeMenu, INT iItem, LPWSTR pszText, INT cchText, BOOL bByPosition)
{
    return HandleToFakeMenu(hFakeMenu)->GetItemText(iItem, pszText, cchText, bByPosition);
}

BOOL APIENTRY FakeMenu_EnableItem(HFAKEMENU hFakeMenu, INT iItem, UINT uEnable)
{
    return HandleToFakeMenu(hFakeMenu)->EnableItem(iItem, uEnable);
}

BOOL APIENTRY FakeMenu_CheckItem(HFAKEMENU hFakeMenu, INT iItem, UINT uCheck)
{
    return HandleToFakeMenu(hFakeMenu)->CheckItem(iItem, uCheck);
}

BOOL APIENTRY FakeMenu_CheckRadioItem(HFAKEMENU hFakeMenu, INT iFirst, INT iLast, INT iCheck, BOOL bByPosition)
{
    return HandleToFakeMenu(hFakeMenu)->CheckRadioItem(iFirst, iLast, iCheck, bByPosition);
}

BOOL APIENTRY FakeMenu_AddString(HFAKEMENU hFakeMenu, UINT nID, LPCWSTR text, UINT fState)
{
    return HandleToFakeMenu(hFakeMenu)->AddString(nID, text, fState);
}

INT APIENTRY FakeMenu_AppendItem(HFAKEMENU hFakeMenu, const MENUITEMINFO* pmii)
{
    return HandleToFakeMenu(hFakeMenu)->AppendItem(pmii);
}

VOID APIENTRY FakeMenu_DeleteItems(HFAKEMENU hFakeMenu)
{
    return HandleToFakeMenu(hFakeMenu)->DeleteItems();
}

INT APIENTRY FakeMenu_TrackPopup(HFAKEMENU hFakeMenu, HWND hwndNotify, POINT pt)
{
    return HandleToFakeMenu(hFakeMenu)->TrackPopup(hwndNotify, pt);
}

} // extern "C"

//////////////////////////////////////////////////////////////////////////////////////////////
