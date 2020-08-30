#include "paint.h"

HBITMAP g_hbm = NULL;
static BOOL s_bDragging = FALSE;
static POINT s_ptOld;
MODE g_nMode = MODE_PENCIL;
static POINT s_pt;

void DoNormalizeRect(RECT *prc)
{
    if (prc->left > prc->right)
        std::swap(prc->left, prc->right);
    if (prc->top > prc->bottom)
        std::swap(prc->top, prc->bottom);
}

static void OnDelete(HWND hwnd)
{
    if (g_nMode != MODE_SELECT)
        return;

    RECT rc;
    SetRect(&rc, s_ptOld.x, s_ptOld.y, s_pt.x, s_pt.y);
    DoNormalizeRect(&rc);

    DoPutSubImage(g_hbm, &rc, NULL);
    InvalidateRect(hwnd, NULL, TRUE);
}

static void OnCopy(HWND hwnd)
{
    RECT rc;
    if (g_nMode == MODE_SELECT)
    {
        SetRect(&rc, s_ptOld.x, s_ptOld.y, s_pt.x, s_pt.y);
        DoNormalizeRect(&rc);
    }
    else
    {
        BITMAP bm;
        GetObject(g_hbm, sizeof(bm), &bm);
        SetRect(&rc, 0, 0, bm.bmWidth, bm.bmHeight);
    }

    HBITMAP hbmNew = DoGetSubImage(g_hbm, &rc);
    if (!hbmNew)
        return;

    HGLOBAL hDIB = DIBFromBitmap(hbmNew);

    if (OpenClipboard(hwnd))
    {
        EmptyClipboard();
        SetClipboardData(CF_DIB, hDIB);
        CloseClipboard();
    }

    DeleteBitmap(hbmNew);
}

static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case ID_CUT:
        OnCopy(hwnd);
        OnDelete(hwnd);
        break;
    case ID_COPY:
        OnCopy(hwnd);
        break;
    case ID_PASTE:
        // TODO:
        break;
    case ID_DELETE:
        OnDelete(hwnd);
        break;
    case ID_SELECT_ALL:
        // TODO:
        break;
    case ID_SELECT:
        g_nMode = MODE_SELECT;
        break;
    case ID_PENCIL:
        g_nMode = MODE_PENCIL;
        break;
    }
}

static BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    g_hbm = DoCreate24BppBitmap(320, 120);
    return TRUE;
}

static void OnDestroy(HWND hwnd)
{
    DeleteObject(g_hbm);
    g_hbm = NULL;
}

static void OnPaint(HWND hwnd)
{
    BITMAP bm;
    GetObject(g_hbm, sizeof(bm), &bm);

    PAINTSTRUCT ps;
    if (HDC hDC = BeginPaint(hwnd, &ps))
    {
        if (HDC hMemDC = CreateCompatibleDC(NULL))
        {
            HBITMAP hbmOld = SelectBitmap(hMemDC, g_hbm);
            BitBlt(hDC, 0, 0, bm.bmWidth, bm.bmHeight,
                   hMemDC, 0, 0, SRCCOPY);
            SelectBitmap(hMemDC, hbmOld);

            if (g_nMode == MODE_SELECT)
            {
                RECT rc = { s_ptOld.x, s_ptOld.y, s_pt.x, s_pt.y };
                DoNormalizeRect(&rc);
                DrawFocusRect(hDC, &rc);
            }

            DeleteDC(hMemDC);
        }
        EndPaint(hwnd, &ps);
    }
}

static void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    if (fDoubleClick)
        return;

    s_bDragging = TRUE;
    SetCapture(hwnd);

    s_ptOld.x = x;
    s_ptOld.y = y;
}

static void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
    if (!s_bDragging)
        return;

    POINT pt = { x, y };
    if (g_nMode == MODE_PENCIL)
    {
        if (HDC hMemDC = CreateCompatibleDC(NULL))
        {
            HPEN hPenOld = SelectPen(hMemDC, GetStockPen(WHITE_PEN));
            HBITMAP hbmOld = SelectBitmap(hMemDC, g_hbm);
            MoveToEx(hMemDC, s_ptOld.x, s_ptOld.y, NULL);
            LineTo(hMemDC, x, y);
            SelectBitmap(hMemDC, hbmOld);
            SelectPen(hMemDC, hPenOld);

            DeleteDC(hMemDC);

            InvalidateRect(hwnd, NULL, TRUE);
        }

        s_ptOld = pt;
    }
    else
    {
        s_pt = pt;
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

static void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
    if (!s_bDragging)
        return;

    POINT pt = { x, y };
    if (g_nMode == MODE_PENCIL)
    {
        if (HDC hMemDC = CreateCompatibleDC(NULL))
        {
            HPEN hPenOld = SelectPen(hMemDC, GetStockPen(WHITE_PEN));
            HBITMAP hbmOld = SelectBitmap(hMemDC, g_hbm);
            MoveToEx(hMemDC, s_ptOld.x, s_ptOld.y, NULL);
            LineTo(hMemDC, x, y);
            SelectBitmap(hMemDC, hbmOld);
            SelectPen(hMemDC, hPenOld);

            DeleteDC(hMemDC);

            InvalidateRect(hwnd, NULL, TRUE);
        }
    }
    else
    {
        s_pt = pt;
        InvalidateRect(hwnd, NULL, TRUE);
    }

    ReleaseCapture();
    s_bDragging = FALSE;
}

LRESULT CALLBACK
CanvasWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
        HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);
        HANDLE_MSG(hwnd, WM_LBUTTONUP, OnLButtonUp);
        case WM_CAPTURECHANGED:
            s_bDragging = FALSE;
            break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
