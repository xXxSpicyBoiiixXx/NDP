#pragma once

#include <stdbool.h>

struct ndp_buf_t {
    void *ptr;
    size_t len;
};

struct ndp_mapped_buf_t {
    void *ptr;
    size_t len;
};

size_t ndp_get_page_size();

bool ndp_mapped_buf_create(
        struct ndp_mapped_buf_t *self,
        size_t len);

bool ndp_mapped_buf_create_num_pages(
        struct ndp_mapped_buf_t *self,
        uint32_t num_pages);

void ndp_mapped_buf_destroy(
        struct ndp_mapped_buf_t *self);

