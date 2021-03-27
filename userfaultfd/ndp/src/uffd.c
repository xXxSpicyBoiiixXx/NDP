#include <stdint.h>

#include <unistd.h>
#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

#include <ndp/core.h>
#include <ndp/uffd.h>
#include <panic.h>

ndp_uffd_t ndp_uffd_create()
{
    int32_t uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
    if (uffd < 0) {
        PANIC("syscall(userfaultdf): failed with %d", uffd);
    }
    return uffd;
}

void ndp_uffd_setup(ndp_uffd_t uffd)
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

void ndp_uffd_register_mem(ndp_uffd_t uffd, struct ndp_mapped_buf_t buf)
{
    // Register memory mapping to be handed with the userfaultfd.
    // We will request to track missing pages (that has not bene loaded in yet)
    struct uffdio_register uff_register = {
            .range = {
                    .start = (uint64_t) buf.ptr,
                    .len = buf.len,
            },
            .mode = UFFDIO_REGISTER_MODE_MISSING,
    };

    int result = ioctl(uffd, UFFDIO_REGISTER, &uff_register);
    if (result < 0) {
        PANIC("ioctl(uffd, UFFDIO_REGISTER, ...): failed with %d", result);
    }
}

void ndp_uffd_copy(ndp_uffd_t uffd, void *dest, void *src, size_t len)
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
