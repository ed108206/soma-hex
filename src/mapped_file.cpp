#include "mapped_file.h"
#include <windows.h>

MappedFile::MappedFile() : m_handle(nullptr), m_view(nullptr), m_size(0)
{
}

MappedFile::~MappedFile()
{
    close();
}

bool MappedFile::open(const char* filename)
{
    close();

    HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) { CloseHandle(hFile); return false; }
    m_size = static_cast<size_t>(fileSize.QuadPart);

    m_handle = CreateFileMapping(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    CloseHandle(hFile);
    if (!m_handle) { m_size = 0; return false; }

    m_view = MapViewOfFile(m_handle, FILE_MAP_READ, 0, 0, 0);
    if (!m_view) { CloseHandle((HANDLE)m_handle); m_handle = nullptr; m_size = 0; return false; }

    return true;
}

void MappedFile::close()
{
    if (m_view) {
        UnmapViewOfFile(m_view); m_view = nullptr;
    }

    if (m_handle) {
        CloseHandle((HANDLE)m_handle);
        m_handle = nullptr;
    }
    m_size = 0;
}

const char* MappedFile::data() const {
    return static_cast<const char*>(m_view);
}

size_t MappedFile::size() const {
    return m_size;
}
