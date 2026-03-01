#ifndef HEADER_E81F9271DA77E5CB
#define HEADER_E81F9271DA77E5CB

#pragma once
#include <cstddef>

class MappedFile {
public:
    MappedFile();
    ~MappedFile();

    bool open(const char* filename);
    void close();

    const char* data() const;
    size_t size() const;

private:
    void* m_handle;
    void* m_view;
    size_t m_size;
};
#endif // header guard 

