#include "row_cache_buffer.h"

RowCacheBuffer::RowCacheBuffer(CSVIndex* index, size_t chunkSize, size_t maxChunks) : m_index(index), m_chunkSize(chunkSize), m_maxChunks(maxChunks)
{
}

void RowCacheBuffer::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_chunks.clear();
}

void RowCacheBuffer::preload(size_t baseLine, const MappedFile& mf)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& c : m_chunks){
        if (c.baseLine == baseLine){
            return;
        }
    }

    Chunk chunk;
    chunk.baseLine = baseLine;

    for (size_t i = baseLine; i < baseLine + m_chunkSize && i < m_index->totalLines(); ++i)
    {
        std::string_view line = m_index->getLine(const_cast<MappedFile&>(mf), i);
        CachedRow r;
        //r.line.assign(line.data(), line.size());
        r.line = line;
        parseLine(line, r);
        chunk.rows.push_back(std::move(r));
    }

    m_chunks.push_front(std::move(chunk));
    while (m_chunks.size() > m_maxChunks){
        m_chunks.pop_back();
    }
}

std::string_view RowCacheBuffer::getCell(size_t row, int col)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& chunk : m_chunks)
    {
        if (row >= chunk.baseLine && row < chunk.baseLine + chunk.rows.size()) {
            const CachedRow& r = chunk.rows[row - chunk.baseLine];
            if (col == 0){
                return std::string_view(r.line.data(), r.commaOffsets.empty() ? r.line.size() : r.commaOffsets[0]);
            }

            if (col <= 0 || static_cast<size_t>(col - 1) >= r.commaOffsets.size()){
                return std::string_view(r.line.data() + (r.commaOffsets.empty() ? 0 : r.commaOffsets.back() + 1), r.line.size() - (r.commaOffsets.empty() ? 0 : r.commaOffsets.back() + 1));
            }
            size_t ucol = static_cast<size_t>(col);
            size_t start = r.commaOffsets[ucol - 1] + 1;
            size_t end   = (ucol < r.commaOffsets.size()) ? r.commaOffsets[ucol] : r.line.size();
            return std::string_view(r.line.data() + start, end - start);
        }
    }

    return {};
}

void RowCacheBuffer::parseLine(const std::string_view& line, CachedRow& row)
{
    size_t pos = 0;
    while (true)
    {
        size_t next = line.find(',', pos);
        if (next == std::string_view::npos){
            break;
        }
        row.commaOffsets.push_back(next);
        pos = next + 1;
    }
}

size_t RowCacheBuffer::getNumCols()
{
    std::string_view r = m_chunks[0].rows[0].line;

    size_t pos = 0;
    size_t cols = 0;

    while (true)  {
        size_t next = r.find(',', pos);
        if (next == std::string_view::npos){
            cols++;
            break;
        }
        pos = next + 1;
        cols++;
    }

    return cols;
}

int RowCacheBuffer::preHeader(std::vector<std::string>& row_cero)
{
    int cols = getNumCols();
    row_cero.reserve(cols);

    for(int i = 0; i < cols; i++){
        std::string_view ss = getCell(0,i);
        std::string tmp;
        tmp.assign(ss.data(), ss.size());
        row_cero.push_back(tmp);
    }
    return cols;
}

void RowCacheBuffer::posHeader()
{
    m_chunks.erase(m_chunks.begin()+1);
}

