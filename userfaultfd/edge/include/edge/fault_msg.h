#pragma once

#include <linux/userfaultfd.h>
#include <cstddef>
#include <algorithm>
#include "fault_registration.h"

#define PAGE_ALIGN(X, SIZE) ((X) & ~(SIZE - 1))

namespace edge {

class fault_msg
{
public:
    explicit fault_msg(const uffd_msg &raw);

    [[nodiscard]] size_t faulting_address() const {
        return raw_.arg.pagefault.address;
    }

    [[nodiscard]] void* faulting_ptr() const {
        return reinterpret_cast<void *>(faulting_address());
    }

    [[nodiscard]] size_t faulting_page_address() const
    {
        return PAGE_ALIGN(faulting_address(), page_size_);
    }

    [[nodiscard]] void* faulting_page_ptr() const
    {
        return reinterpret_cast<void *>(faulting_page_address());
    }

private:
    const size_t page_size_{};
    struct uffd_msg raw_;
};

}