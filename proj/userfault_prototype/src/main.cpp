#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <chrono>

#include <unistd.h>
#include <fcntl.h>
#include <syscall.h>
#include <linux/userfaultfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <poll.h>

#include <edge/panic.h>
#include <cstring>

struct edge_fault_handler_ctx_t
{
    size_t page_size;
    int32_t uffd;
};

static void *
fault_handler_thread(void *arg)
{
    static void *page = nullptr;
    static uint64_t num_faults = 0;

    //

    PANIC_IF_NULL(arg);
    auto &ctx = *(edge_fault_handler_ctx_t *) arg;

    // Create page (if not already) that will be copied later into the requested page
    if (page == nullptr) {
        page = mmap(
                nullptr,
                ctx.page_size,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS,
                -1,
                0);
        if (page == MAP_FAILED) {
            PANIC("mmap");
        }
    }

    // Loop handling any events from the userfaultfd file descriptor
    while (true) {
        struct pollfd pollfd = {
                .fd = ctx.uffd,
                .events = POLLIN,
        };
        int32_t num_ready = poll(&pollfd, 1, -1);
        if (num_ready < 0) {
            PANIC("poll");
        }

#if DEBUG_USERFAULT
        printf("\nfault_handler_thread():\n");
        printf("    poll() returns: nready = %d; "
               "POLLIN = %d; POLLERR = %d\n", num_ready,
               (pollfd.revents & POLLIN) != 0,
               (pollfd.revents & POLLERR) != 0);
#endif

        // Read event from file descriptor
        struct uffd_msg msg = {};
        ssize_t num_read = read(ctx.uffd, &msg, sizeof(msg));
        if (num_read == 0) {
            PANIC("read(uffd, ...): got unexpected EOF");
        }
        if (num_read < 0) {
            PANIC("read(uffd, ...): invalid result");
        }

        // Verify event received
        if (msg.event != UFFD_EVENT_PAGEFAULT) {
            PANIC("bad uffd_msg.event type");
        }

        // Get faulting address
#define PAGE_ALIGN(X, SIZE) ((X) & ~(SIZE - 1))
        size_t req_addr = msg.arg.pagefault.address;
        size_t req_addr_page_start = PAGE_ALIGN(req_addr, ctx.page_size);

        // Print debug information
#if DEBUG_USERFAULT
        printf("    UFFD_EVENT_PAGEFAULT event: ");
        printf("flags = %llx; ", msg.arg.pagefault.flags);
        printf("address = %llx\n", msg.arg.pagefault.address);
#endif

        // Write the result into our copy of the result page
        memset(page, (int32_t)('A' + (num_faults % 20)), ctx.page_size);
        num_faults++;

        struct uffdio_copy copy_cmd = {
                // Tell userfaultfd to copy our result page
                .dst = (uint64_t) req_addr_page_start,
                .src = (uint64_t) page,
                .len = ctx.page_size,
                .mode = 0,
        };
        int32_t result = ioctl(ctx.uffd, UFFDIO_COPY, &copy_cmd);
        if (result < 0) {
            PANIC("ioctl(uffd, UFFDIO_COPY, ...): failed with %d", result);
        }

#if DEBUG_USERFAULT
        printf("uffdio_copy: copied %lld bytes\n", copy_cmd.copy);
#endif
    }
}

edge_fault_handler_ctx_t g_ctx = {};

int main()
{
    const uint32_t num_pages = 1000;

    const size_t page_size = sysconf(_SC_PAGE_SIZE);
    const size_t mapped_mem_size = num_pages * page_size;

    // Create and setup `userfaultfd` object
    int32_t uffd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
    if (uffd < 0) {
        PANIC("syscall(userfaultdf): failed with %d", uffd);
    }

    struct uffdio_api userfault_api = {
            .api = UFFD_API,
            .features = 0,
    };

    int result = ioctl(uffd, UFFDIO_API, &userfault_api);
    if (result < 0) {
        PANIC("ioctl(uffd, UFFDIO_API, ...): failed failed with %d", result);
    }

    // Create private anonymous mapping. This will be demand paged and
    // allocated when requested by our handler
    char *buf = (char *) mmap(
            nullptr,
            mapped_mem_size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0);
    if (buf == MAP_FAILED) {
        PANIC("mmap: failed to map private anonymous virtual memory");
    }

    printf("mmap: start at %p\n", buf);

    // Register memory mapping to be handed with the userfaultfd.
    // We will request to track missing pages (that has not bene loaded in yet)
    struct uffdio_register uff_register = {
            .range = {
                    .start = (uint64_t) buf,
                    .len = mapped_mem_size,
            },
            .mode = UFFDIO_REGISTER_MODE_MISSING,
    };

    result = ioctl(uffd, UFFDIO_REGISTER, &uff_register) < 0;
    if (result < 0) {
        PANIC("ioctl(uffd, UFFDIO_REGISTER, ...): failed with %d", result);
    }

    // Create thread to handle userfaultfd events
    g_ctx.page_size = page_size;
    g_ctx.uffd = uffd;

    pthread_t handler_thread;
    result = pthread_create(
            &handler_thread,
            nullptr,
            fault_handler_thread,
            (void *) &g_ctx);
    if (result != 0) {
        PANIC("pthread_create: failed with %d", result);
    }

    // Trigger page faults by reading from 1024 bytes apart
    int l = 0xf; // avoid faulting on page boundary to make things clear
    for (; l < mapped_mem_size; l += 0x1000) {
        using namespace std::chrono;

        char *addr = buf + l;
        auto start = high_resolution_clock::now();

        char c = *addr;

        auto end = high_resolution_clock::now();
        auto dur = duration_cast<nanoseconds>(end - start);

        printf("read %p = %c, fault took %ld nanos\n", addr, c, dur.count());
    }

    exit(EXIT_SUCCESS);
}