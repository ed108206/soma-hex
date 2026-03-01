#ifndef HEADER_ADAFB5087A86027D
#define HEADER_ADAFB5087A86027D

#include <windows.h>
#include <commdlg.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <string_view>
#include "BaseView.h"

#include "mapped_file.h"
#include "csv_index.h"
#include "row_cache_buffer.h"

class CFrameProc;

class CEView : public BaseView
{
public:
    CEView();
    ~CEView() override;

    CFrameProc* cfr{nullptr};

    HWND m_hwnd{nullptr};

    void Initialize() override;
    HWND hwnd() const override { return m_hwnd; }
    void setParent(HWND parent) override { m_parent = parent; }
    void setFont(HFONT font) override { m_font = font; }

    LRESULT CallMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void ShowText(const char* b);
    void ShowText(HWND hwnd, const char* b);
    void SetNumberToStatic(long long unsigned int number);
    void SetNumberToCaption(long long unsigned int number);

    void PreMain(HWND hwnd);
    void FreeAll(HWND hwnd);
    void MakeRow0Header();
    CEView* GetLocalPtr(HWND hw);

    bool PreSelectMain(HWND hwnd, char* szFileName, char* fname);
    bool isDialogExist(std::string& ss);

    CSVIndex g_index;
    RowCacheBuffer* g_cache;
    MappedFile g_mf;

    std::string GetCSVName();

private:
    HFONT m_font;
    HWND m_parent;

    bool m_dlg{false};
    int m_mela{0};
    int m_ur{0};

    std::vector<std::string> m_header;

    char m_fname[MAX_PATH];

    HWND CreateListView(HINSTANCE hInstance, HWND hwndParent);
    void ResizeListView(HWND hwndParent);
    LRESULT ListViewNotify(HWND hWnd, LPARAM lParam);

    static INT_PTR CALLBACK EaDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ListProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void mostrar_fila(size_t idx);
    void DoFileOpenEx(HWND hwnd);
    void SetText(const char* b);
    void SimulateClick(HWND hw);
    void roFind(const char* str);
    void GetRow0Text();

};

#endif // header guard

