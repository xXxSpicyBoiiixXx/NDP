#include <unistd.h>
#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

#include <edge/panic.h>
#include "edge/internal/uffd.h"

namespace edge::internal::uffd {

int32_t create()
{
    int32_t uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
    if (uffd < 0) {
        PANIC("syscall(userfaultdf): failed with %d", uffd);
    }
    return uffd;
}

void setup(int32_t uffd)
{
    struct uffdio_api userfault_api = {
            .api = UFFD_API,
            .features = 0,
    };

    int result = ioctl(uffd, UFFDIO_API, &userfault_api);
    if (result < 0) {
        PANIC("ioctl(uffd, UFFDIO_API, ...): failed failed with %d", result);
    }
}

void register_mem(int32_t uffd, mapped_buf &buf)
{
    // Register memory mapping to be handed with the userfaultfd.
    // We will request to track missing pages (that has not bene loaded in yet)
    struct uffdio_register uff_register = {
            .range = {
                    .start = (uint64_t) buf.ptr(),
                    .len = buf.size(),
            },
            .mode = UFFDIO_REGISTER_MODE_MISSING,
    };

    int result = ioctl(uffd, UFFDIO_REGISTER, &uff_register) < 0;
    if (result < 0) {
        PANIC("ioctl(uffd, UFFDIO_REGISTER, ...): failed with %d", result);
    }
}

void copy(int32_t uffd, void *dest, void *src, size_t len)
{
    struct uffdio_copy copy_cmd = {
            // Tell userfaultfd to copy our result page
            .dst = (uint64_t) dest,
            .src = (uint64_t) src,
            .len = len,
            .mode = 0,
    };
    int32_t result = ioctl(uffd, UFFDIO_COPY, &copy_cmd);
    if (result < 0) {
        PANIC("ioctl(uffd, UFFDIO_COPY, ...): failed with %d", result);
    }
}

}
