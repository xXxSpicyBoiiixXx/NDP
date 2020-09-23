#pragma once

#include <cstdint>
#include <edge/mapped_buf.h>

namespace edge::internal::uffd {

int32_t create();
void setup(int32_t uffd);
void register_mem(int32_t uffd, edge::mapped_buf &buf);
void copy(int32_t uffd, void *dest, void *src, size_t len);

}
