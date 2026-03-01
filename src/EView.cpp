#include "base.h"
#include "EView.h"
#include "resource.h"
#include "FrameProc.h"
#include <stdexcept>
#include <thread>
#include <iomanip>

static WNDPROC lvproc = nullptr;
static HINSTANCE cev_hInst = nullptr;
const int CHUNK_SIZE = 2000;


CEView::CEView()
    : cfr(nullptr),
	  m_hwnd(nullptr),
      m_font(nullptr),
      m_parent(nullptr),
	  m_dlg(false),
      m_mela(0),
      m_ur(0)
{
    g_index.cev = this;
}

CEView::~CEView()
{
}

void CEView::Initialize()
{
    cev_hInst = GetModuleHandle(nullptr);
    if (cev_hInst) {
        CreateDialogParamA(cev_hInst, MAKEINTRESOURCE(IDD_DIALOG2), m_parent, EaDialogProc, reinterpret_cast<LPARAM>(this));
    } else {
        MessageBox(nullptr, "ERROR: create CEView dlg.", SNOM, MB_OK);
    }
}


void CEView::FreeAll(HWND hwnd)
{
    HWND hL = GetDlgItem(hwnd, IDC_LIST1);
    if (hL) {
        SendMessage(hL, LVM_SETITEMCOUNT, (WPARAM)0, (LPARAM)LVSICF_NOINVALIDATEALL);
    }

    if (lvproc != nullptr && hL) {
        SetWindowLongPtr(hL, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(lvproc));
    }

    if (g_cache){
        g_cache->clear();
        delete g_cache;
        g_cache = nullptr;
    }

    g_index.stopAsyncBuild();
    g_mf.close();
}

CEView* CEView::GetLocalPtr(HWND hw)
{
    return cfr->GetCurrentPtr(hw);
}


void CEView::ShowText(const char* b)
{
    HWND hL = GetDlgItem(m_hwnd, IDC_STATIC1);
    SetWindowText(hL, b);
}


void CEView::ShowText(HWND hwnd, const char* b)
{
    HWND hL = GetDlgItem(hwnd, IDC_STATIC1);
    SetWindowText(hL, b);
}


void CEView::SetNumberToStatic(long long unsigned int number)
{
    HWND hL = GetDlgItem(m_hwnd, IDC_STATIC1);
    if(!number){
        SetWindowText(hL, "");
        return;
    }

    std::stringstream ss;
    ss.imbue(std::locale(std::locale::classic(), new custom_numpunct));
    ss << number;

    std::string formatted = "Indexing CSV file, please wait.. ";
    formatted += ss.str();

    SetWindowText(hL, formatted.c_str());
}


void CEView::SetNumberToCaption(long long unsigned int number)
{
    if(!number){
        cfr->TextCaption("");
        return;
    }
    std::stringstream ss;
    ss.imbue(std::locale(std::locale::classic(), new custom_numpunct));
    ss << number;

    std::string formatted = "Indexing CSV: ";
    formatted += ss.str();

    cfr->TextCaption(formatted);
}


void CEView::ResizeListView(HWND hwndParent)
{
    RECT rc{};
    GetClientRect(hwndParent, &rc);

    HWND hL = GetDlgItem(hwndParent, IDC_LIST1);
    const int ex = 40;
    MoveWindow(hL, rc.left + 1, rc.top + ex, rc.right - (rc.left + 2), rc.bottom - (rc.top + ex + 1), TRUE);

    hL = GetDlgItem(hwndParent, IDC_STATIC2);
    MoveWindow(hL, rc.left + 1, rc.top + 1, rc.left + 49, (rc.top + ex - 6), TRUE);

    hL = GetDlgItem(hwndParent, IDC_STATIC1);
    MoveWindow(hL, rc.left + 55, rc.top + 1, rc.right - (rc.left + 56), (rc.top + ex - 6), TRUE);
}


void CEView::GetRow0Text()
{
	char buf[256] ={'\0'};

	LVITEM lvItem;
	lvItem.mask = LVIF_TEXT;
	lvItem.pszText = buf;
	lvItem.cchTextMax = 256;

	int ncol = (int)g_cache->getNumCols();
    HWND hL = GetDlgItem(m_hwnd, IDC_LIST1);

	for(int i = 0; i < ncol; i++){
        lvItem.iSubItem = i;
		SendMessage(hL, LVM_GETITEMTEXT, (WPARAM)0, (LPARAM)&lvItem);
		std::string ss(lvItem.pszText);
        m_header.push_back(ss);
	}
}


void CEView::MakeRow0Header()
{
    if(m_ur) {
        MessageBox(m_hwnd, "It appears that row 0 is now in the header.", SNOM, MB_OK);
        return;
    }

    int ncol = (int)m_header.size();

    char buf[256] ={'\0'};
    HWND hL = GetDlgItem(m_hwnd, IDC_LIST1);

    LVCOLUMN lvcol{};
    lvcol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvcol.cx = 120;

    for (int i = 0; i < ncol; ++i) {
        ListView_GetColumn(hL, i, &lvcol);
        snprintf(buf, sizeof(buf), "%s", m_header[i].c_str());
        lvcol.pszText = buf;
        ListView_SetColumn(hL, i, &lvcol);
    }

    g_index.posOffsets();
    g_cache->posHeader();

    ListView_SetItemCountEx(hL, (int)g_index.totalLines(), 0);
    m_ur = 1;
}


INT_PTR CALLBACK CEView::EaDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CEView* pcev = reinterpret_cast<CEView*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (msg) {
    case WM_INITDIALOG: {
            pcev = reinterpret_cast<CEView*>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pcev);

            pcev->m_hwnd = hWnd;

            // select csv file
            char szFileName[MAX_PATH] = {'\0'};

            if(!pcev->PreSelectMain(hWnd, szFileName, pcev->m_fname)){
                EndDialog(hWnd, 0);
                pcev->m_hwnd = 0;
                return 0;
            }

            // font
            HWND hL = GetDlgItem(hWnd, IDC_STATIC1);
            SendMessage(hL, WM_SETFONT, (WPARAM)pcev->m_font, MAKELPARAM(1, 0));

            hL = GetDlgItem(hWnd, IDC_STATIC2);
            SendMessage(hL, WM_SETFONT, (WPARAM)pcev->m_font, MAKELPARAM(1, 0));

            // icon
            HICON ICON = LoadIcon(cev_hInst, MAKEINTRESOURCE(IDI_ICON1));
            SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)ICON);

            // listview
            HWND hList = GetDlgItem(hWnd, IDC_LIST1);

            // subclass
            lvproc = (WNDPROC)SetWindowLongPtr(hList, GWLP_WNDPROC, (LONG_PTR)ListProc);

            // font
            SendMessage(hList, WM_SETFONT, (WPARAM)pcev->m_font, MAKELPARAM(1, 0));

            // style
            DWORD style = LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER;
            ListView_SetExtendedListViewStyle(hList, style);
            ListView_SetBkColor(hList, GetSysColor(COLOR_WINDOW));
            ListView_SetTextBkColor(hList, GetSysColor(COLOR_WINDOW));
            ListView_SetTextColor(hList, RGB(0,0,0));

            // focus main window
            pcev->SimulateClick(pcev->cfr->m_fhwnd);

            // open csv file
            if (pcev->g_mf.open(szFileName)){

                // estimated lines
                size_t estimatedLines = pcev->g_index.estimateTotalLines(pcev->g_mf);
                ListView_SetItemCountEx(hList, (int)estimatedLines, LVSICF_NOINVALIDATEALL);

                pcev->g_index.buildInitial(pcev->g_mf, CHUNK_SIZE);

                // build cache
                pcev->g_cache = new RowCacheBuffer(&pcev->g_index, CHUNK_SIZE);
                pcev->g_cache->preload(0, pcev->g_mf);

                // create initial columns
                int cols = pcev->g_cache->getNumCols();
                char buf[64] = {'\0'};
                sprintf(buf, "%i Columns, indexing file, wait...", cols);
                pcev->ShowText(buf);

                LVCOLUMN col{};
                col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
                col.cx = 120;

                for (int i = 0; i < cols; ++i) {
                    snprintf(buf, sizeof(buf), "Col %d", i + 1);
                    col.pszText = buf;
                    ListView_InsertColumn(hList, i, &col);
                }

                // indexing file in the background
                pcev->g_index.startBuildAsync(pcev->g_mf, hWnd);

                // final window style
                LONG_PTR dw = GetWindowLongPtr(hWnd, GWL_STYLE);
                dw &= ~(WS_BORDER | WS_DLGFRAME | WS_THICKFRAME | WS_POPUP);
                dw |= WS_CHILD;
                SetWindowLongPtr(hWnd, GWL_STYLE, dw);

                // resize listview
                pcev->ResizeListView(hWnd);

                // set the window parent
                if (pcev->m_parent) {
                    SetParent(hWnd, pcev->m_parent);
                }
                pcev->m_dlg = true;

                pcev->GetRow0Text();
            }
            else {
                MessageBox(hWnd, "There was an error in the parser,\nor there is no data in the file.", SNOM, MB_OK);
                EndDialog(hWnd, 0);
            }
        }
        break;

    case WM_NOTIFY:
        if (pcev) {
            pcev->ListViewNotify(pcev->m_hwnd, lParam);
        }
        return 0;


    case WM_SIZE:
        if (pcev && !pcev->m_mela && pcev->m_parent) {
            RECT rc{};
            GetClientRect(pcev->m_parent, &rc);
            const int ru = 26;
            MoveWindow(hWnd, rc.left, rc.top + ru, rc.right - rc.left, rc.bottom - ru, TRUE);
        }
        if (pcev) {
            pcev->ResizeListView(hWnd);
        }
        break;

    case WM_ERASEBKGND:
        return TRUE;

    case WM_CTLCOLORDLG:
        return (INT_PTR)WBRUSH;
        break;

    case WM_CTLCOLORSTATIC: {
            HDC hdc     = reinterpret_cast<HDC>(wParam);
            HWND target = reinterpret_cast<HWND>(lParam);

            int ctrlId = GetDlgCtrlID(target);
            SetBkMode(hdc, TRANSPARENT);

            switch (ctrlId) {
                case IDC_STATIC1:
                    SetTextColor(hdc, RGB(0, 0, 0));
                    return reinterpret_cast<INT_PTR>(WHITE);

                case IDC_STATIC2:
                    SetTextColor(hdc, RGB(0, 0, 255));
                    return reinterpret_cast<INT_PTR>(WBRUSH);

                default:
                    SetTextColor(hdc, RGB(0, 0, 0));
                    return reinterpret_cast<INT_PTR>(WHITE);
            }
        }
        break;

    case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rc;
            GetClientRect(hWnd, &rc);
            FillRect(hdc, &rc, WHITE);

            const HWND controls[] = {
                GetDlgItem(hWnd, IDC_LIST1),
                GetDlgItem(hWnd, IDC_STATIC1),
                GetDlgItem(hWnd, IDC_STATIC2)
            };

            for (HWND ctrl : controls) {
                if (!ctrl) {
                    continue;
                }

                RECT cr;
                GetWindowRect(ctrl, &cr);

                int w = cr.right - cr.left;
                int h = cr.bottom - cr.top;

                POINT at{ cr.left, cr.top };
                ScreenToClient(hWnd, &at);

                cr.left   = at.x - 1;
                cr.top    = at.y - 1;
                cr.right  = cr.left + w + 2;
                cr.bottom = cr.top + h + 2;
                FrameRect(hdc, &cr, DBORD);
            }
            EndPaint(hWnd, &ps);
        }
        break;

    case WM_COMMAND: {
            switch(LOWORD(wParam)) {
            case IDC_STATIC1:
                //PostMessage(hWnd, WM_CLOSE, 0,0);
                break;

            case IDC_STATIC2: {
                    HWND hL = GetDlgItem(hWnd, IDC_STATIC2);
                    if(!pcev->m_mela) {
                        pcev->m_mela = 1;
                        SetWindowText(hL, "Attach");
                        pcev->cfr->ReParent(hWnd, "Attach", 1);
                    } else {
                        pcev->m_mela = 0;
                        SetWindowText(hL, "Detach");
                        pcev->cfr->ReParent(hWnd, "Detach", 0);
                    }
                }
                break;

            default:
                return false;
            }
        }
        break;

    case WM_INDEX_UPDATED:
        {
            HWND hList = GetDlgItem(hWnd, IDC_LIST1);
            ListView_SetItemCountEx(hList,(int)pcev->g_index.totalLines(), LVSICF_NOINVALIDATEALL);
            return TRUE;
        }

    case WM_CLOSE:
        {
            if (pcev) {
                pcev->cfr->ProxyMoCursor(4);
                pcev->FreeAll(hWnd);
                DestroyWindow(hWnd);
                if (pcev->cfr) {
                    pcev->cfr->DeleteCurTab(hWnd);
                }
                pcev->cfr->ProxyMoCursor(0);
            }
        }
        break;

    default:
        return false;
    }

    return 0;
}

bool CEView::isDialogExist(std::string& ss)
{
    ss = m_fname;
    return m_dlg;
}


std::string CEView::GetCSVName()
{
    std::string ss(m_fname);
    return ss;
}


void CEView::SimulateClick(HWND hwnd)
{
    POINT pt;
    GetCursorPos(&pt);

    int cx = GetSystemMetrics(SM_CXSCREEN);
    int cy = GetSystemMetrics(SM_CYSCREEN);

	WINDOWPLACEMENT wp;
	GetWindowPlacement(hwnd, &wp);
	RECT zc = wp.rcNormalPosition;

	// corner left-top
	int dx = (65535 * zc.left) / cx;
    int dy = (65535 * zc.top) / cy;
    dx += 3000;
	dy += 1000;

    mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, dx, dy, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, dx, dy, 0, 0);

    SetCursorPos(pt.x, pt.y);
}


bool CEView::PreSelectMain(HWND hwnd, char* szFileName, char* fname)
{
    OPENFILENAME ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "CSV(*.csv)\0*.csv\0";
    ofn.lpstrFile = szFileName;
    ofn.lpstrFileTitle = fname;
    ofn.nMaxFile = MAX_PATH;
    ofn.nMaxFileTitle = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "csv";

    if (GetOpenFileName(&ofn)) {
        return true;
    }
    return false;
}


LRESULT CEView::ListViewNotify(HWND hWnd, LPARAM lParam)
{
    LPNMHDR lpnmh = (LPNMHDR)lParam;

    switch (lpnmh->code)
    {
        case LVN_GETDISPINFO:
        {
            NMLVDISPINFO* di = (NMLVDISPINFO*)lParam;
            int row = di->item.iItem;
            int col = di->item.iSubItem;

            if (di->item.mask & LVIF_TEXT) {
                std::string_view text = g_cache->getCell(row, col);

                if (!text.empty())
                {
                    static std::string tmp;
                    tmp.assign(text.data(), text.size());
                    lstrcpynA(di->item.pszText, tmp.c_str(), di->item.cchTextMax);
                }
            }
        }
        break;

        case LVN_ODCACHEHINT:
        {
            auto* hint = (LPNMLVCACHEHINT)lParam;
            int base = (hint->iFrom / CHUNK_SIZE) * CHUNK_SIZE;
            g_cache->preload(base, g_mf);

            int nextBase = base + CHUNK_SIZE;
            if ((size_t)nextBase < g_index.totalLines()){
                g_cache->preload(nextBase, g_mf);
            }
        }
        break;
    }

    return 0;
}

LRESULT CALLBACK CEView::ListProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_ERASEBKGND) {
        return TRUE;//FALSE;
    }
    return CallWindowProc(lvproc, hwnd, msg, wParam, lParam);
}

