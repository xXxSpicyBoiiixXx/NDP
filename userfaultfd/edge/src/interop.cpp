#include <cstdint>
#include <cstddef>
#include <unistd.h>
#include <sys/mman.h>

#include "edge/panic.h"
#include "edge/interop.h"
#include "edge/mapped_buf.h"

namespace edge::interop {

size_t page_size()
{
    static size_t memo = -1;
    if (memo != -1) {
        return memo;
    }

    memo = sysconf(_SC_PAGE_SIZE);
    return memo;
}

}