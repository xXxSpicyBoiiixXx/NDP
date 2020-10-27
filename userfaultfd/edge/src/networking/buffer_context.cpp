#include <cstddef>
#include <cstdint>
#include <climits>
#include <iostream>
#include <optional>
#include <sstream>
#include <vector>
#include <unistd.h>

namespace edge::util {

template <typename UInt>
UInt ceil_pow2(UInt v) {
    static_assert(std::is_unsigned<UInt>::value, "Only works for unsigned types");
    v--;
    for (size_t i = 1; i < sizeof(v) * CHAR_BIT; i *= 2) //Prefer size_t "Warning comparison between signed and unsigned integer"
    {
        v |= v >> i;
    }
    return ++v;
}

}

namespace edge::networking {

class seekbuf {
public:
    explicit seekbuf(size_t capacity)
            : capacity_target_(capacity)
            , buf_(capacity)
            , pos_(0)
    {}

    void regrow()
    {
        buf_.reserve(edge::util::ceil_pow2(capacity_target_ + pos_));
    }

    template<typename T>
    ssize_t read_into(T &output)
    {
        if (sizeof(T) > remaining()) {
            return -1;
        }

        auto initial_ptr = data();
        output = *((T *) initial_ptr);
        return pos_ + sizeof(T);
    }

    template<typename T>
    ssize_t write(T &input)
    {
        if (sizeof(T) > remaining()) {
            return -1;
        }

        auto initial_ptr = data();
        *((T *) initial_ptr) = input;
        return pos_ + sizeof(T);
    }

    [[nodiscard]] const uint8_t *data() const {
        return reinterpret_cast<const uint8_t *>(buf_.data()) + pos_;
    }

    [[nodiscard]] void *ptr() const {
        return (void *) data();
    }

    [[nodiscard]] size_t size() const
    {
        return buf_.size() - pos_;
    }

    [[nodiscard]] size_t remaining() const
    {
        return buf_.capacity() - size();
    }

private:
    size_t capacity_target_;
    std::vector<std::byte> buf_;
    size_t pos_;
};

namespace messages {

enum class message_kind_t : uint32_t
{
    INVALID,
    REQ_ALLOC,
    RES_ALLOC
};

// TODO: These do not set the new buffer position correctly on success
// or failure!

#define SEEKBUF_BEGIN ssize_t __result

#define SEEKBUF_READ(BUF, VAR) \
    __result = (BUF).read_into(VAR); \
    if (__result < 0) return false

#define SEEKBUF_WRITE(BUF, VAR) \
    __result = (BUF).write(VAR); \
    if (__result < 0) return false

#define SEEKBUF_END return true

struct header_t
{
    uint32_t len;
    message_kind_t message_kind;

    bool read_from(seekbuf &buf)
    {
        SEEKBUF_BEGIN;
        SEEKBUF_READ(buf, len);
        SEEKBUF_READ(buf, message_kind);
        SEEKBUF_END;
    }

    bool write_to(seekbuf &buf)
    {
        SEEKBUF_BEGIN;
        SEEKBUF_WRITE(buf, len);
        SEEKBUF_WRITE(buf, message_kind);
        SEEKBUF_END;
    }
};


constexpr uint32_t header_len = sizeof(header_t);

struct alloc_request {
    size_t size;

    bool read_from(seekbuf &buf)
    {
        SEEKBUF_BEGIN;
        SEEKBUF_READ(buf, size);
        SEEKBUF_END;
    }

    bool write_to(seekbuf &buf)
    {
        SEEKBUF_BEGIN;
        SEEKBUF_READ(buf, size);
        SEEKBUF_END;
    }
};

struct alloc_response {
    uint32_t handle;

    bool read_from(seekbuf &buf)
    {
        SEEKBUF_BEGIN;
        SEEKBUF_READ(buf, handle);
        SEEKBUF_END;
    }

    bool write_to(seekbuf &buf)
    {
        SEEKBUF_BEGIN;
        SEEKBUF_READ(buf, handle);
        SEEKBUF_END;
    }
};

}

enum class buffer_state_t
{
    EMPTY,
    WAIT_HEADER,
    WAIT_DATA,
    DONE
};

class buffer_context
{
public:
    explicit buffer_context(size_t capacity)
            : buf_(capacity)
            , header_(std::nullopt)
    {}

    bool try_read_header()
    {
        messages::header_t header = {};
        ssize_t next_pos = header.read_from(buf_);
        if (next_pos < 0) { return false; }

        header_.emplace(header);
        return true;
    }

    [[nodiscard]] buffer_state_t state() const
    {
        size_t actual_size = size();

        if (actual_size == 0 && !header_.has_value()) {
            return buffer_state_t::EMPTY;
        } else if (!header_.has_value()) {
            return buffer_state_t::WAIT_HEADER;
        } else if (header_.has_value() && actual_size >= header_.value().len) {
            return buffer_state_t::DONE;
        } else {
            return buffer_state_t::WAIT_DATA;
        }
    }

    [[nodiscard]] size_t size() const
    {
        return buf_.size();
    }

    seekbuf &buffer()
    {
        return buf_;
    }

    void set_header(messages::header_t header)
    {
        header_.emplace(header);
    }

    [[nodiscard]] const std::optional<messages::header_t> &header() const
    {
        return header_;
    }

private:
    seekbuf buf_;
    std::optional<messages::header_t> header_;
};

template <typename R>
std::optional<R> read_next(int socket_fd, buffer_context &ctx)
{
    while (true) {
        auto cur_state = ctx.state();
        if (cur_state != buffer_state_t::DONE) {
            ctx.buffer().regrow();
            ssize_t num_ready = read(
                    socket_fd,
                    ctx.buffer().ptr(),
                    ctx.buffer().size());

            if (num_ready == 0) {
                printf("peer disconnected\n");
                break;
            }

            if (cur_state == buffer_state_t::EMPTY
                || cur_state == buffer_state_t::WAIT_HEADER)
            {
                ctx.try_read_header();
            }

            cur_state = ctx.state();
        }

        // Check if we can read the body at this point
        if (cur_state == buffer_state_t::DONE) {
            auto header = ctx.header().value();
            auto kind = header.message_kind;

            R message = {};
            ssize_t result = message.read_from(ctx.buffer());
            if (result < 0) {
                continue;
            }

            ctx.buffer().regrow();
            return std::make_optional(message);
        }
    }
}

}
