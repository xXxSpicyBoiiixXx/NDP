#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <optional>
#include <chrono>
#include <thread>
#include <memory>
#include <atomic>

#include <linux/mman.h>
#include <unistd.h>
#include <poll.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/socket.h>

#include <edge/fault_handler_memory.h>
#include <edge/fault_registration.h>
#include <edge/mapped_buf.h>
#include <edge/panic.h>

#define DEBUG_USERFAULT 0


#define EDGE_FIB_NUM (42)
uint64_t fib_kernel(uint32_t n)
{
    uint64_t n1 = -1;
    uint64_t n2 = 1;
    for (uint32_t i = 0; i <= n; ++i) {
        uint64_t sum = n1 + n2;
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

    uint64_t fib(uint32_t n)
    {
        ((uint32_t *) memory_.request_buf().ptr())[0] = EDGE_FIB_NUM;
        ((uint32_t *) memory_.request_buf().ptr())[1] = n;

        uint8_t poke = *(volatile uint8_t *) memory_.invoke_buf().ptr();

        uint64_t result = *(uint64_t *) memory_.response_buf().ptr();
        return result;
    }

private:
    edge::fault_handler_memory &memory_;
};

typedef struct {
    unsigned long size,resident,share,text,lib,data,dt;
} statm_t;

statm_t read_off_memory_status()
{
    statm_t result;

    unsigned long dummy;
    const char* statm_path = "/proc/self/statm";

    FILE *f = fopen(statm_path,"r");
    if(!f){
        perror(statm_path);
        abort();
    }
    if(7 != fscanf(f,"%ld %ld %ld %ld %ld %ld %ld",
                   &result.size,&result.resident,&result.share,&result.text,&result.lib,&result.data,&result.dt))
    {
        perror(statm_path);
        abort();
    }
    fclose(f);

    return result;
}

class edge_server_context {
public:
    using handler_fn_t = void *(*)(void *);

    edge_server_context(
            edge::fault_handler_memory_spec &memory_spec,
            uint32_t num_function_reg,
            handler_fn_t handler_fn)
    : mem_(create_memory_default(memory_spec))
    , reg_(create_registration(mem_))
    , handler_thread_(create_handler_thread(this, handler_fn))
    , fns_(fn_registration(num_function_reg))
    , swap_buf_(create_swap_page(mem_))
    {
    }

    void invalidate()
    {
        void *remapped = mremap(
                mem_.invoke_buf().ptr(),
                mem_.invoke_buf().size(),
                mem_.invoke_buf().size(),
                MREMAP_MAYMOVE | MREMAP_DONTUNMAP | MREMAP_FIXED,
                swap_buf_.ptr());
        if (remapped == MAP_FAILED) {
            static char errstrbuf[512] = {};
            char *errstr = strerror_r(errno, errstrbuf, sizeof(errstrbuf));
            PANIC("mremap: failed to invalidate page: %s", errstr);
        }

        munmap(remapped, mem_.invoke_buf().size());
    }

    [[nodiscard]] edge::fault_handler_memory &memory()
    {
        return mem_;
    }

    [[nodiscard]] edge::fault_registration &registration()
    {
        return reg_;
    }

    [[nodiscard]] fn_registration &functions()
    {
        return fns_;
    }

private:
    edge::fault_handler_memory mem_;
    edge::fault_registration reg_;
    fn_registration fns_;
    pthread_t handler_thread_;
    edge::mapped_buf swap_buf_;

    static edge::fault_handler_memory create_memory_default(edge::fault_handler_memory_spec &spec)
    {
        printf("mark 1: RSS = %ld kb\n", read_off_memory_status().resident);

        // Create and setup `userfaultfd` object
        auto mem = edge::fault_handler_memory::create(spec);

        return mem;
    }

    static edge::fault_registration create_registration(edge::fault_handler_memory &mem) {
        auto reg = edge::fault_registration(mem.invoke_buf());
        return reg;
    }

    static pthread_t create_handler_thread(edge_server_context *ctx, handler_fn_t handle_fn)
    {
        pthread_t handler_thread;
        int32_t result = pthread_create(
                &handler_thread,
                nullptr,
                handle_fn,
                (void *) ctx);
        if (result != 0) {
            PANIC("pthread_create: failed with %d", result);
        }

        return handler_thread;
    }

    static edge::mapped_buf create_swap_page(edge::fault_handler_memory &mem)
    {
        auto swap_page = edge::mapped_buf::create_from_num_pages(mem.invoke_buf().num_pages());
        return swap_page;
    }
};

static std::atomic<uint32_t> num_faults(0);

static void *
fault_handler_thread(void *arg)
{
    static std::optional<edge::mapped_buf> page;

    //

    PANIC_IF_NULL(arg);
    auto &ctx = *(edge_server_context *) arg;

    // Create page (if not already) that will be copied later into the requested page
    if (!page.has_value()) {
        page.emplace(edge::mapped_buf::create_from_num_pages(1));
    }

    // Loop handling any events from the userfaultfd file descriptor
    while (true) {
        auto pollfd = ctx.registration().make_pollfd();
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
        auto msg = ctx.registration().read();

        // Print debug information
#if DEBUG_USERFAULT
        printf("    UFFD_EVENT_PAGEFAULT event: ");
        printf("flags = %llx; ", msg.arg.pagefault.flags);
        printf("address = %llx\n", msg.arg.pagefault.address);
#endif

        // Invoke function
        // TODO: Handle arbitrary function signature
        auto *input = (uint32_t *) ctx.memory().request_buf().ptr();

        typedef uint64_t fn_sig(uint32_t);
        auto *fn = (fn_sig *) ctx.functions().handler_at(input[0]);

        uint32_t arg0 = input[1];
        uint64_t result = fn(arg0);

        ((uint64_t *) ctx.memory().response_buf().ptr())[0] = result;

//        num_faults++;
        ctx.registration().respond_with(msg, *page);

#if DEBUG_USERFAULT
        printf("uffdio_copy: copied %lld bytes\n", copy_cmd.copy);
#endif
    }
}

int start_listening(uint16_t portno)
{
    int result;

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    PANIC_IF_NEG_WITH_ERRNO(socket_fd, "failed to open socket: socket");

    int flag = 1;
    result = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const void *) &flag, sizeof(flag));
    PANIC_IF_NEG_WITH_ERRNO(result, "setsockopt");

    struct sockaddr_in serveraddr = {};
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(portno);
    result = bind(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    PANIC_IF_NEG_WITH_ERRNO(result, "bind");

    return socket_fd;
}

class edge_client_context {
public:
    edge_client_context(edge_server_context &parent, const sockaddr_in client_addr, socklen_t client_addr_len)
        : parent_(parent)
        , client_addr_(client_addr)
        , client_addr_len_(client_addr_len)
    {}

    edge_server_context &parent()
    {
        return parent_;
    }
    const sockaddr_in &client_addr()
    {
        return client_addr_;
    }
    socklen_t client_addr_len()
    {
        return client_addr_len_;
    }

private:
    edge_server_context &parent_;
    struct sockaddr_in client_addr_;
    socklen_t client_addr_len_;
};

void client_handler_thread(std::unique_ptr<edge_client_context> ctx)
{
    using namespace std::chrono;
    auto start = high_resolution_clock::now();

    edge_rpc rpc(ctx->parent().memory());
    auto answer = rpc.fib(1);

    auto end = high_resolution_clock::now();
    auto dur = duration_cast<nanoseconds>(end - start);

    printf("%ld\n", dur.count());
//        printf("[%d faults | RSS = %ld kb] fib(%d) = %ld | (dirty) took %ld nanos\n",
//               num_faults.load(),
//               read_off_memory_status().resident,
//               fib_n,
//               answer,
//               dur
//        .count());


}

int main()
{
    int result = 0;

    edge::fault_handler_memory_spec memory_spec = {
            .num_invoke_pages = 1,
            .num_request_pages = 1,
            .num_response_pages = 1,
    };
    edge_server_context ctx(memory_spec, 100, fault_handler_thread);

    std::vector<std::thread> client_threads;

    //
    // start listening on the port
    //

    int socket_fd = start_listening(9000);
    result = listen(socket_fd, 128);
    PANIC_IF_NEG_WITH_ERRNO(result, "listen");

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = 0;
    while (true) {
        bzero(&client_addr, sizeof(client_addr));
        client_addr_len = 0;
        int client_fd = accept(socket_fd, reinterpret_cast<sockaddr *>(&client_addr), &client_addr_len);
        if (client_fd < 0) {
            WARN_WITH_ERRNO("accept");
            continue;
        }

        auto client_ctx = std::make_unique<edge_client_context>(
                ctx,
                client_addr,
                client_addr_len);

        client_threads.emplace_back(client_handler_thread, std::move(client_ctx));
    }

    return 0;
}
