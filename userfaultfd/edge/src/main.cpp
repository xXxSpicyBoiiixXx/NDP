#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <chrono>

#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>

#include <edge/panic.h>
#include <edge/fault_handler_memory.h>
#include <edge/fault_registration.h>
#include <edge/mapped_buf.h>


#define DEBUG_USERFAULT 0

uint32_t fib_kernel(uint32_t n)
{
    uint32_t n1 = -1;
    uint32_t n2 = 1;
    for (uint32_t i = 0; i <= n; ++i) {
        uint32_t sum = n1 + n2;
        n1 = n2;
        n2 = sum;
    }
    return n2;
}


static void *
fault_handler_thread(void *arg)
{
    static std::optional<edge::mapped_buf> page;
    static uint64_t num_faults = 0;

    //

    PANIC_IF_NULL(arg);
    auto &ctx = *(edge::fault_registration *) arg;

    // Create page (if not already) that will be copied later into the requested page
    if (!page.has_value()) {
        page.emplace(edge::mapped_buf::create_from_num_pages(1));
    }

    // Loop handling any events from the userfaultfd file descriptor
    while (true) {
        auto pollfd = ctx.make_pollfd();
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
        auto msg = ctx.read();

        // Get faulting address
#define PAGE_ALIGN(X, SIZE) ((X) & ~(SIZE - 1))
        size_t req_addr = msg.faulting_address();
        size_t req_addr_page_start = msg.faulting_page_address();

        // Print debug information
#if DEBUG_USERFAULT
        printf("    UFFD_EVENT_PAGEFAULT event: ");
        printf("flags = %llx; ", msg.arg.pagefault.flags);
        printf("address = %llx\n", msg.arg.pagefault.address);
#endif

        // Write the result into our copy of the result page
        memset(page->ptr(), (int32_t)('A' + (num_faults % 20)), ctx.page_size());
        num_faults++;
        ctx.respond_with(msg, *page);

#if DEBUG_USERFAULT
        printf("uffdio_copy: copied %lld bytes\n", copy_cmd.copy);
#endif
    }
}

int main()
{
    // Create and setup `userfaultfd` object
    auto mem = edge::fault_handler_memory::create({
        .num_invoke_pages = 1,
        .num_request_pages = 1,
        .num_response_pages = 1,
    });
    static auto ctx = edge::fault_registration(mem.invoke_buf());

    // Create thread to handle userfaultfd events
    pthread_t handler_thread;
    int32_t result = pthread_create(
            &handler_thread,
            nullptr,
            fault_handler_thread,
            (void *) &ctx);
    if (result != 0) {
        PANIC("pthread_create: failed with %d", result);
    }

    // Trigger page faults by reading from 1024 bytes apart
    int l = 0x0; // avoid faulting on page boundary to make things clear
    char *buf = static_cast<char *>(mem.invoke_buf().ptr());
    for (; l < mem.invoke_buf().size(); l += 0x100) {
        using namespace std::chrono;

        char *addr = buf + l;
        auto start = high_resolution_clock::now();

        char c = *addr;

        auto end = high_resolution_clock::now();
        auto dur = duration_cast<nanoseconds>(end - start);

        printf("read %lx = %c, fault took %ld nanos\n",
               addr - buf,
               c,
               dur.count());
        usleep(10000);
    }

    return 0;
}
