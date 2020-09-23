#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <edge/panic.h>

#include "edge/mapped_buf.h"
#include "edge/interop.h"

namespace edge {

mapped_buf mapped_buf::create_from_num_pages(uint32_t num_pages)
{
    size_t page_size = edge::interop::page_size();
    return mapped_buf(num_pages * page_size, page_size);
}

mapped_buf mapped_buf::create_from_num_pages(uint32_t num_pages, size_t page_size)
{
    return mapped_buf(num_pages * page_size, page_size);
}

mapped_buf::mapped_buf(size_t mem_size, size_t page_size)
        : ptr_(nullptr)
        , size_(mem_size)
        , page_size_(page_size)
{
    ptr_ = mmap(
            nullptr,
            mem_size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0);
}

mapped_buf::~mapped_buf()
{
    if (ptr_ == nullptr || size_ == 0) {
        // nothing to do, this is the "ghost" of a move
        return;
    }

    printf("mapped_buf: dtor\n");
    int32_t result = munmap(ptr_, size_);
    if (result != 0) {
        PANIC("munmap: failed to unmap buffer");
    }
}

mapped_buf::mapped_buf(mapped_buf &&other) noexcept
        : ptr_(other.ptr_)
        , size_(other.size_)
        , page_size_(other.page_size_)
{
    other.ptr_ = nullptr;
    other.size_ = 0;
    other.page_size_ = 0;
}

}