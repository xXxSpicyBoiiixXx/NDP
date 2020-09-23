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


#define EDGE_FIB_NUM (42)
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

class fn_registration {
public:
    explicit fn_registration(uint32_t max_registrations)
        : max_registrations_(max_registrations)
        , fns_(max_registrations)
    { }

    void set_handler(uint32_t slot, void *handler)
    {
        if (slot > max_registrations_) {
            PANIC("handler slot out of range");
        }
        fns_[slot] = handler;
    }

    void *handler_at(uint32_t slot)
    {
        void *fn = fns_[slot];
        PANIC_IF_NULL(fn);
        return fn;
    }

private:
    uint32_t max_registrations_;
    std::vector<void *> fns_;
};

class edge_rpc {
public:
    explicit edge_rpc(edge::fault_handler_memory &memory)
        : memory_(memory)
    { }

    uint32_t fib(uint32_t n)
    {
        ((uint32_t *) memory_.request_buf().ptr())[0] = EDGE_FIB_NUM;
        ((uint32_t *) memory_.request_buf().ptr())[1] = n;

        uint8_t poke = *(volatile uint8_t *) memory_.invoke_buf().ptr();

        uint32_t result = *(uint32_t *) memory_.response_buf().ptr();
        return result;
    }

private:
    edge::fault_handler_memory &memory_;
};


struct fault_ctx {
    edge::fault_handler_memory &mem;
    edge::fault_registration &reg;
    fn_registration &fns;
};

static void *
fault_handler_thread(void *arg)
{
    static std::optional<edge::mapped_buf> page;
    static uint64_t num_faults = 0;

    //

    PANIC_IF_NULL(arg);
    auto &ctx = *(fault_ctx *) arg;

    // Create page (if not already) that will be copied later into the requested page
    if (!page.has_value()) {
        page.emplace(edge::mapped_buf::create_from_num_pages(1));
    }

    // Loop handling any events from the userfaultfd file descriptor
    while (true) {
        auto pollfd = ctx.reg.make_pollfd();
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
        auto msg = ctx.reg.read();

        // Print debug information
#if DEBUG_USERFAULT
        printf("    UFFD_EVENT_PAGEFAULT event: ");
        printf("flags = %llx; ", msg.arg.pagefault.flags);
        printf("address = %llx\n", msg.arg.pagefault.address);
#endif

        // Invoke function
        // TODO: Handle arbitrary function signature
        auto *input = (uint32_t *) ctx.mem.request_buf().ptr();

        typedef uint32_t fn_sig(uint32_t);
        auto *fn = (fn_sig *) ctx.fns.handler_at(input[0]);

        uint32_t arg0 = input[1];
        uint32_t result = fn(arg0);

        ((uint32_t *) ctx.mem.response_buf().ptr())[0] = result;

        num_faults++;
        ctx.reg.respond_with(msg, *page);

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

    static auto reg = edge::fault_registration(mem.invoke_buf());

    static auto fns = fn_registration(1000);
    fns.set_handler(EDGE_FIB_NUM, (void *) &fib_kernel);

    static fault_ctx ctx = {
            .mem = mem,
            .reg = reg,
            .fns = fns,
    };

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

    auto rpc = edge_rpc(mem);
    uint32_t fib_n = 10;

    using namespace std::chrono;
    auto start = high_resolution_clock::now();

    uint32_t answer = rpc.fib(fib_n);

    auto end = high_resolution_clock::now();
    auto dur = duration_cast<nanoseconds>(end - start);

    printf("fib(%d) = %d | call took %ld nanos\n", fib_n, answer, dur.count());
    return 0;
}
