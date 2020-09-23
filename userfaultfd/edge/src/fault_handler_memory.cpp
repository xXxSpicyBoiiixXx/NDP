#include <cstddef>
#include <cstdint>
#include <algorithm>

#include "edge/fault_handler_memory.h"
#include "edge/mapped_buf.h"
#include "edge/interop.h"

namespace edge {

fault_handler_memory fault_handler_memory::create(
        const fault_handler_memory_spec &spec)
{
    spec.ensure_or_panic();

    auto page_size = edge::interop::page_size();
    auto request_buf = edge::mapped_buf::create_from_num_pages(spec.num_request_pages, page_size);
    auto response_buf = edge::mapped_buf::create_from_num_pages(spec.num_response_pages, page_size);
    auto poke_buf = edge::mapped_buf::create_from_num_pages(spec.num_invoke_pages, page_size);

    return fault_handler_memory(page_size,
                                std::move(poke_buf),
                                std::move(request_buf),
                                std::move(response_buf));
}

fault_handler_memory::fault_handler_memory(
        size_t pageSize,
        mapped_buf pokeBuf,
        mapped_buf requestBuf,
        mapped_buf responseBuf)
        : page_size_(pageSize)
        , invoke_buf_(std::move(pokeBuf))
        , request_buf_(std::move(requestBuf))
        , response_buf_(std::move(responseBuf)) {}

void fault_handler_memory_spec::ensure_or_panic() const
{
    PANIC_IF_ZERO(num_request_pages);
    PANIC_IF_ZERO(num_response_pages);
    PANIC_IF_ZERO(num_invoke_pages);
}

}