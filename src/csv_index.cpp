#include "base.h"
#include "csv_index.h"
#include <cstring>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include "EView.h"


CSVIndex::CSVIndex() : m_stop(false), m_ready(false)
{
}

CSVIndex::~CSVIndex() {
    stopAsyncBuild();
}

void CSVIndex::buildInitial(const MappedFile& mf, size_t initialLines)
{
    m_offsets.clear();
    size_t pos = 0;
    m_offsets.push_back(pos);

    const size_t totalSize = mf.size();
    size_t lines = 0;

    while (pos < totalSize && lines < initialLines)
    {
        const char* data = mf.data() + pos;
        const char* end = (const char*)memchr(data, '\n', totalSize - pos);
        size_t next = end ? (end - mf.data() + 1) : totalSize;
        pos = next;
        m_offsets.push_back(pos);
        ++lines;
    }

    m_ready = !m_offsets.empty();
}

void CSVIndex::startBuildAsync(const MappedFile& mf, HWND hNotifyWnd)
{
    m_stop = false;

    size_t startPos;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        startPos = m_offsets.empty() ? 0 : m_offsets.back();
    }

    m_thread = std::thread([this, &mf, startPos, hNotifyWnd]()
    {
        auto t0 = std::chrono::steady_clock::now();

        size_t pos = startPos;
        const size_t totalSize = mf.size();
        //size_t batchCounter = 0;

        while (pos < totalSize && !m_stop)
        {
            const char* data = mf.data() + pos;
            const char* end = (const char*)memchr(data, '\n', totalSize - pos);
            size_t next = end ? (end - mf.data() + 1) : totalSize;
            pos = next;

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_offsets.push_back(pos);
            }

            if(pos % 10'000 == 0){
                cev->SetNumberToCaption(pos);
            }
        }

        PostMessage(hNotifyWnd, WM_INDEX_UPDATED, 0, 0);

        auto t1 = std::chrono::steady_clock::now();
        std::chrono::duration<double> t_read  = t1 - t0;

        char bef[256] = {'\0'};
        std::stringstream ss;
        ss.imbue(std::locale(std::locale::classic(), new custom_numpunct));
        ss << totalLines();

        snprintf(bef, sizeof(bef), "Indexing time: %.6f sec | Number of lines: %s", t_read.count(), ss.str().c_str());

        cev->ShowText(bef);
        cev->SetNumberToCaption(0);
    });
}

void CSVIndex::stopAsyncBuild()
{
    m_stop = true;
    if (m_thread.joinable()){
        m_thread.join();
    }
}

bool CSVIndex::ready() const {
     return m_ready;
}

std::string_view CSVIndex::getLine(MappedFile& mf, size_t line)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (line >= m_offsets.size() - 1)
        return {};

    size_t start = m_offsets[line];
    size_t end   = m_offsets[line + 1];

    if (end > 0 && mf.data()[end - 1] == '\n'){
        --end;
    }

    return std::string_view(mf.data() + start, end - start);
}

size_t CSVIndex::totalLines() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_offsets.size()-1;
}

size_t CSVIndex::estimateTotalLines(const MappedFile& mf, size_t sampleSize)
{
    const size_t fileSize = mf.size();
    if (fileSize == 0)
        return 0;

    size_t actualSample = std::min(sampleSize, fileSize);
    const char* data = mf.data();

    size_t lines = 0;

    for (size_t i = 0; i < actualSample; ++i){
        if (data[i] == '\n'){
            ++lines;
        }
    }

    if (lines == 0){
        return fileSize / 80; // fb
    }

    double bytesPerLine = (double)actualSample / lines;
    return (size_t)(fileSize / bytesPerLine);
}

void CSVIndex::posOffsets()
{
    m_offsets.erase(m_offsets.begin()); // delete row 0
}
