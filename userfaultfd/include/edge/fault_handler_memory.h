#pragma once

#include "mapped_buf.h"
#include "panic.h"

namespace edge {

struct fault_handler_memory_spec
{
    uint32_t num_invoke_pages;
    uint32_t num_request_pages;
    uint32_t num_response_pages;

    void ensure_or_panic() const;
};

class fault_handler_memory
{
public:
    static fault_handler_memory create(const fault_handler_memory_spec &spec);

    edge::mapped_buf &invoke_buf() { return invoke_buf_; }
    edge::mapped_buf &request_buf() { return request_buf_; }
    edge::mapped_buf &response_buf() { return response_buf_; }

private:
    fault_handler_memory(
            size_t pageSize,
            mapped_buf pokeBuf,
            mapped_buf requestBuf,
            mapped_buf responseBuf);

    size_t page_size_;
    edge::mapped_buf invoke_buf_;
    edge::mapped_buf request_buf_;
    edge::mapped_buf response_buf_;
};

}