#pragma once

namespace edge {

class mapped_buf
{
public:
    mapped_buf(mapped_buf &&other) noexcept;
    ~mapped_buf();

    static mapped_buf create_from_num_pages(uint32_t num_pages);
    static mapped_buf create_from_num_pages(uint32_t num_pages, size_t page_size);

    [[nodiscard]] void *ptr() const { return ptr_; }
    [[nodiscard]] size_t size() const { return size_; }
    [[nodiscard]] size_t page_size() const { return page_size_; }
    [[nodiscard]] size_t num_pages() const { return size_ / page_size_; }

private:
    mapped_buf(size_t mem_size, size_t page_sizes);

    void *ptr_;
    size_t size_;
    size_t page_size_;
};

}