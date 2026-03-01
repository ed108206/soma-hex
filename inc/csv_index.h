#ifndef HEADER_77DDE8C4C68A36EA
#define HEADER_77DDE8C4C68A36EA

#pragma once
#include <vector>
#include <string_view>
#include <thread>
#include <atomic>
#include <mutex>
#include <windows.h>
#include "mapped_file.h"


#define WM_INDEX_UPDATED        (WM_APP + 1)

class CEView;

/*
struct custom_numpunct : std::numpunct<char> {
protected:
    //char do_thousands_sep() const override { return '\''; } // thousands separator
    char do_thousands_sep() const override { return '.'; } // thousands separator
    std::string do_grouping() const override { return "\3"; } // groups of 3 digits
};
*/

class CSVIndex {
public:
    CSVIndex();
    ~CSVIndex();

    CEView* cev;

    void buildInitial(const MappedFile& mf, size_t initialLines = 1000);
    void startBuildAsync(const MappedFile& mf, HWND hNotifyWnd);
    void stopAsyncBuild();

    bool ready() const;
    size_t totalLines() const;

    std::string_view getLine(MappedFile& mf, size_t line);
    size_t estimateTotalLines(const MappedFile& mf, size_t sampleSize = 32 * 1024 * 1024);
    void posOffsets();


private:
    std::vector<size_t> m_offsets;
    std::thread m_thread;
    std::atomic<bool> m_stop;
    std::atomic<bool> m_ready;

    mutable std::mutex m_mutex;
};

#endif


