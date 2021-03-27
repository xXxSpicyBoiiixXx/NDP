#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

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
    size_t *source_dirty_bitmap_len;

    uint8_t *target_buf;
    size_t target_len;

    // ???: Not sure if we want to have something like this
    uint8_t *target_dirty_bitmap;
    size_t *target_dirty_bitmap_len;

    ndp_event_handler_status_t status;

    uint32_t user_callback_vector;
};

typedef void (*ndp_compute_fp)(struct npd_event_t *fault);

struct ndp_register_t {
    uint8_t *source_buf;
    size_t source_len;

    uint8_t *target_buf;
    size_t target_len;

    ndp_compute_fp compute_fp;
    atomic<boolean> did_cancel;
};


//        if (fault->did_cancel) {
//            fault->status = NDP_FAULT_STATUS_CANCELLED;
//            return;
//        }


void sum_i32_compute(struct npd_event_t *fault) {
    // TODO: Use dirty bitmap to recompute without running over the entire
    // buffer

    ENSURE_OR_PANIC(fault->target_len >= sizeof(int32_t));

    int32_t sum = 0;

    const uint8_t *start = fault->source_buf;
    const uint8_t *end = start + source_len;
    for (int32_t *p = (int32_t *) start; p < end; p++) {
        sum += *p;
    }

    int32_t *result = (int32_t *) fault->target_buf;
    *result = sum;

    if (sum > 9000) {
        fault->user_callback_vector = USER_INTESTING_STUFF_CB;
        fault->user_callback_args = &args_struct;
    }

    ASSERT(fault->target_dirty_bitmap_len == (fault->target_len + 7) / 8);
    fault->target_dirty_bitmap[0] |= 0xf0;

    fault->status = NDP_FAULT_STATUS_TARGET_CHANGED;
}

void ndp_register(struct ndp_register_t *reg) {
    // TODO
}

int main() {

#define N (1024)
    int32_t *input = calloc(N, sizeof(int32_t));
    int32_t *result = malloc(sizeof(int32_t));

    for (int32_t *p = input; p < input + N; p++) {
        *p = random_i32();
    }

    struct ndp_register_t reg = {
            .source_buf = (uint8_t *) input,
            .source_len = N * sizeof(int32_t),
            .target_buf = (uint8_t *) result,
            .target_len = sizeof(int32_t),
            .compute_fp = &sum_i32_compute
    };

    ndp_register(&reg);

    // Perform the initial calculation
    int32_t sum0 = *result; // --> trigger the initial computation in NDP

    // Change random values
    input[77] += 7; // -> trigger computation
    input[12] += 7; // -> again...
    input[1] += 7;  // ->   ,,
    input[42] += 7; // ->   ,,

    // Recompute the sum
    int32_t sum1 = *result; // directly reads result since it's not dirty

    ENSURE(sum0 + (7 * 4) == sum1);
}

void async_example() {

    // ... registration


    // Recompute the sum
    int32_t sum_1 = ndp_fetch_async(result).wait();

    //
    ndp_buf_hash_t token1 = ndp_hash(input, N * sizeof(int32));
    ndp_bitmap_t input_dirty = ndp_compute_dirty_bitmap(token0, token1);

    //
    Future<int32_t> sum_buf_1 = npd_fetch_async(&sum_1_result);
    Future<int32_t> sum_buf_2 = npd_fetch_async(&sum_2_result);

    int32_t sums[2] = {};
    wait(sum, &sums);

    ENSURE(sum0 + 7 == sum1);
}

//
//

void more_random_ideas() {

    // Perform the initial calculation
    int32_t sum0 = *result; // --> trigger the computation in NDP
    ndp_buf_hash_t token0 = ndp_hash(input, N * sizeof(int32));

    /*
     *             [H(ABCD) ** = H(H(AB)||H(CD)) ]
     *                 /                 \
     *  [H(AB) = H(H(A)||H(B))]  [H(CD) ** = H( H(C)||H(D) )]
     *        /    \              /         \
     *  [ H(a)]   [H(B) ]     [H(C) ** ]   [H(D)   ]
     *     ,,         ,,         ,,         ,,
     *  [   A   ]  [   B    ] [   ** C   ] [    D     ]
     */

    // Change a value
    thread stream_prices()
    {
        something_t *input;
        while (read(input)) {
            stock_prices[get_ticker('GME')] = input.price;
        }
    }

    remote thread ndp(uint64_t *target_epoch) {
        while (true) {
            // do thing to achieve target_epoch
            wait()
        }
    }

    thread user()
    {
        while (true) {
            ndp_wait_until()
        }
    }

    //

    thread gui()
    {
        while (true) {
            wait(16ms); // 60 fps

            ndp_advance_epoch();
            for (int i = 0; i < NUM_STOCKS; i++) {
                gui_control->text = do_something_fancy_with_prices(stock_prices[i]);
            }
        }
    }

}
