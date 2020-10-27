#pragma once

#include <cstdint>
#include <edge/mapped_buf.h>
#include <edge/internal/uffd.h>
#include <cstring>
#include "fault_msg.h"

namespace edge {

class fault_msg;

class fault_registration
{
public:
    explicit fault_registration(edge::mapped_buf &buf);

    [[nodiscard]] struct pollfd make_pollfd() const;
    [[nodiscard]] edge::fault_msg read() const;
    void respond_with(edge::fault_msg &dest, edge::mapped_buf &src) const;

    [[nodiscard]] size_t page_size() const { return buf_.page_size(); }
    [[nodiscard]] int32_t fd() const { return fd_; }

private:
    static int32_t create(edge::mapped_buf &buf);

    int32_t fd_;
    edge::mapped_buf &buf_;
};

}