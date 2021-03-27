#pragma once

#include "mapped_buf.h"
typedef uint8_t ndp_event_handler_status_t;

#define NDP_FAULT_STATUS_FAILED          (0);
#define NDP_FAULT_STATUS_TARGET_CHANGED  (1);
#define NDP_FAULT_STATUS_TARGET_NOCHANGE (2);
#define NDP_FAULT_STATUS_CANCELLED       (3);

struct npd_event_t {
    uint8_t *source_buf;
    size_t source_len;

    // ???: Not sure if we want to have something like this
    uint8_t *source_dirty_bitmap;
    size_t source_dirty_bitmap_len;

    uint8_t *target_buf;
    size_t target_len;

    // ???: Not sure if we want to have something like this
    uint8_t *target_dirty_bitmap;
    size_t target_dirty_bitmap_len;

    ndp_event_handler_status_t status;

    uint32_t user_callback_vector;
};

typedef void (*ndp_compute_fp)(struct npd_event_t *fault);

struct ndp_register_t {
    struct ndp_mapped_buf_t source_buf;
    struct ndp_mapped_buf_t target_buf;
    ndp_compute_fp compute_fp;
};
