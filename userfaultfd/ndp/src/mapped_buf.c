#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

#include <ndp/mapped_buf.h>
#include <panic.h>

size_t ndp_get_page_size()
{
    static size_t memo = 0;
    if (memo != 0) {
        return memo;
    }

    memo = sysconf(_SC_PAGE_SIZE);
    return memo;
}

bool ndp_mapped_buf_create(struct ndp_mapped_buf_t *self, size_t len)
{
    self->ptr = mmap(
            NULL,
            len,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0);

    self->len = len;

    return true;
}

bool ndp_mapped_buf_create_num_pages(struct ndp_mapped_buf_t *self, uint32_t num_pages)
{
    return ndp_mapped_buf_create(self, ndp_get_page_size() * num_pages);
}

void ndp_mapped_buf_destroy(struct ndp_mapped_buf_t *self)
{
    if (!(self && self->ptr && self->len > 0)) {
        return;
    }

    int32_t result = munmap(self->ptr, self->len);
    if (result != 0) {
        PANIC_WITH_ERRNO("munmap");
    }

    memset(self, 0, sizeof(*self));
}
