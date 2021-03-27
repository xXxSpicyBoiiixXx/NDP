#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "ndp/core.h"
#include "ndp/mapped_buf.h"
#include "panic.h"

//        if (fault->did_cancel) {
//            fault->status = NDP_FAULT_STATUS_CANCELLED;
//            return;
//        }


void sum_i32_compute(struct npd_event_t *fault) {
    // TODO: Use dirty bitmap to recompute without running over the entire
    // buffer

    ENSURE_OR_PANIC(fault->target_len >= sizeof(int32_t));

    int32_t sum = 0;

    const int32_t *p = (int32_t *) fault->source_buf;
    const int32_t *end = (int32_t *) (p + fault->source_len);
    for (; p < end; p++) {
        sum += *p;
    }

    int32_t *result = (int32_t *) fault->target_buf;
    *result = sum;

//    if (sum > 9000) {
//        fault->user_callback_vector = USER_INTESTING_STUFF_CB;
//        fault->user_callback_args = &args_struct;
//    }

    ENSURE_OR_PANIC(
            fault->target_dirty_bitmap_len >= ((fault->target_len + 7) / 8));
    fault->target_dirty_bitmap[0] |= 0xf0;

    fault->status = NDP_FAULT_STATUS_TARGET_CHANGED;
}

void ndp_register(struct ndp_register_t *reg) {
    // TODO
}

int main() {

#define N (4096 / sizeof(int32_t))
    ENSURE_OR_PANIC(ndp_get_page_size() >= 4096);

    struct ndp_mapped_buf_t input_buf;
    ndp_mapped_buf_create_num_pages(&input_buf, 1);

    struct ndp_mapped_buf_t result_buf;
    ndp_mapped_buf_create_num_pages(&result_buf, 1);

    srandom((uint32_t) time(NULL));

    int32_t *p = input_buf.ptr;
    int32_t *p_end = p + N;
    for (; p < p_end; p++) {
        *p = random() % 1000;
    }

    struct ndp_register_t reg = {
            .source_buf = input_buf,
            .target_buf = result_buf,
            .compute_fp = &sum_i32_compute
    };

    ndp_register(&reg);

    // Perform the initial calculation
    int32_t *result = &((int32_t *) result_buf.ptr)[0];
    int32_t sum0 = *result; // --> trigger the initial computation in NDP

    // Change random values
    int32_t *input = (int32_t *) input_buf.ptr;
    input[77] += 7; // -> trigger computation
    input[12] += 7; // -> again...
    input[1] += 7;  // ->   ,,
    input[42] += 7; // ->   ,,

    // Recompute the sum
    int32_t sum1 = *result; // directly reads result since it's not dirty

    ENSURE_OR_PANIC(sum0 + (7 * 4) == sum1);
}
