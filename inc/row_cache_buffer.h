#ifndef HEADER_AECFCB426ABF301A
#define HEADER_AECFCB426ABF301A

#pragma once
#include <vector>
#include <string>
#include <deque>
#include <mutex>
#include <string_view>
#include "csv_index.h"
#include "mapped_file.h"


struct CachedRow {
    //std::string line;
    std::string_view line;
    std::vector<size_t> commaOffsets;
};

struct Chunk {
    size_t baseLine;
    std::vector<CachedRow> rows;
};

class RowCacheBuffer {
public:
    RowCacheBuffer(CSVIndex* index, size_t chunkSize, size_t maxChunks = 3);

    void preload(size_t baseLine, const MappedFile& mf);
    std::string_view getCell(size_t row, int col);

    size_t getNumCols();
    int preHeader(std::vector<std::string>& row_cero);
    void posHeader();

    void clear();

private:
    void parseLine(const std::string_view& line, CachedRow& row);

    CSVIndex* m_index;
    size_t m_chunkSize;
    size_t m_maxChunks;
    std::deque<Chunk> m_chunks;
    std::mutex m_mutex;
};
#endif // header guard

