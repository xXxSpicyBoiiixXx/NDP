#include <cstdint>

#include <unistd.h>
#include <fcntl.h>
#include <syscall.h>
#include <linux/userfaultfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <poll.h>

#include <edge/panic.h>
#include <edge/mapped_buf.h>
#include <edge/interop.h>
#include "edge/fault_registration.h"
#include "edge/internal/uffd.h"

namespace edge {

fault_registration::fault_registration(mapped_buf &buf)
        : fd_(create(buf))
        , buf_(buf)
{ }

struct pollfd fault_registration::make_pollfd() const
{
    return {
            .fd = fd_,
            .events = POLLIN,
    };
}

int32_t fault_registration::create(mapped_buf &buf)
{
    using namespace edge::internal;

    int32_t fd = uffd::create();
    uffd::setup(fd);
    uffd::register_mem(fd, buf);

    return fd;
}

edge::fault_msg fault_registration::read() const
{
    struct uffd_msg msg = {};
    ssize_t num_read = ::read(fd_, &msg, sizeof(msg));
    if (num_read == 0) {
        PANIC("read_into: got unexpected EOF");
    }
    if (num_read < 0) {
        PANIC("read_into: invalid result");
    }

    // Verify event received
    if (msg.event != UFFD_EVENT_PAGEFAULT) {
        PANIC("read_into: bad uffd_msg.event type");
    }

    return fault_msg(msg);
}

void fault_registration::respond_with(fault_msg &dest, mapped_buf &src) const
{
    edge::internal::uffd::copy(fd_, dest.faulting_page_ptr(), src.ptr(), src.size());
}


}