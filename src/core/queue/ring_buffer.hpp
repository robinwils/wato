#pragma once

#include <bx/ringbuffer.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>

/**
 *
 *  ┌─────────────────────────────────────────────────────────────────────────┐
 *  │             bx::RingBufferControl Overview (ASCII Diagram)              │
 *  └─────────────────────────────────────────────────────────────────────────┘
 *
 *  RingBufferControl tracks read and write offsets within a fixed-size buffer
 *  in a circular fashion. We typically have an array or memory block of size
 *  'm_size' bytes (or "slots"). The ring "wraps around" when it hits the end:
 *
 *      +----------+----------+----------+----------+----------+-----+
 *      |   idx 0  |   idx 1  |   idx 2  |   idx 3  |   idx 4  | ... |
 *      +----------+----------+----------+----------+----------+-----+
 *       ^          ^                                         ^
 *       |          |                                         |
 *       m_read     (possible data in between)               m_write
 *
 *      • m_size   : The total capacity of the ring (e.g., 1024).
 *      • m_read   : Latest read offset (where oldest data is consumed).
 *      • m_write  : Latest write offset (where new data is written).
 *      • m_current: Latest commited offset.
 *
 *  ┌─────────────────────────────────────────────────────────────────────────┐
 *  │  Producer Flow: reserve(...) -> copy data -> commit(...)                │
 *  └─────────────────────────────────────────────────────────────────────────┘
 *
 *   1) reserve(N):
 *      - Asks the ring buffer to "reserve" N bytes/slots.
 *      - If enough space is available, it returns N (or partial if using mustSucceed=false).
 *        0 otherwise
 *      - Internally sets aside that space from m_write forward.
 *
 *   3) commit(N):
 *      - Confirms that you finished writing N bytes, advancing m_current by N
 *        (wrapping around if it goes past the end).
 *
 *  ┌─────────────────────────────────────────────────────────────────────────┐
 *  │  Consumer Flow: available() -> read data -> consume(...)                │
 *  └─────────────────────────────────────────────────────────────────────────┘
 *
 *   1) available():
 *      - Returns how many bytes/slots are currently unconsumed (m_current - m_read,
 *        wrapped for the circular boundary).
 *
 *   2) (You) read from the underlying buffer, again taking care if the read
 *      wraps around the end back to index 0.
 *
 *   3) consume(N):
 *      - Moves the m_read offset forward by N (wrapping as needed),
 *        effectively discarding that portion of the ring.
 *
 *  Essentially, RingBufferControl is a “pointer arithmetic” manager for a
 *  circular buffer. You provide the memory, it provides the indexing.
 *
 */

/**
 * @brief RingBufferControl tracks read and write offsets within a fixed-size buffer
 * in a circular fashion. We typically have an array or memory block of size
 * 'm_size' bytes (or "slots"). The ring "wraps around" when it hits the end
 *
 */
template <typename Type, std::size_t Size>
struct RingBuffer {
   public:
    static constexpr std::size_t CAPACITY = Size;

    using value_type     = Type;
    using element_type   = std::optional<value_type>;
    using container_type = std::array<element_type, CAPACITY>;

    RingBuffer() : mPrevious(CAPACITY - 1), mCtrl(CAPACITY) {}

    RingBuffer(const RingBuffer&)            = delete;
    RingBuffer(RingBuffer&&)                 = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;
    RingBuffer& operator=(RingBuffer&&)      = delete;
    ~RingBuffer()                            = default;

    inline void Push(const element_type& aElt = std::nullopt)
    {
        ensureReserved();

        if (!mBuffer[mCtrl.m_write]) {
            mBuffer[mCtrl.m_write].emplace(aElt.value_or(value_type()));
        }
    }

    [[nodiscard]] inline value_type& Latest() noexcept
    {
        if (!mBuffer[mCtrl.m_current]) {
            mBuffer[mCtrl.m_current].emplace();
        }
        return mBuffer[mCtrl.m_current].value();
    }
    [[nodiscard]] inline value_type&   Oldest() noexcept { return mBuffer[mCtrl.m_read].value(); }
    [[nodiscard]] inline element_type& Previous() noexcept { return mBuffer[mPrevious]; }

    inline value_type& Discard() noexcept
    {
        auto& res = Oldest();
        mCtrl.consume(1);
        return res;
    }

   private:
    void ensureReserved()
    {
        mPrevious = mCtrl.m_current;
        auto size = mCtrl.reserve(1);

        if (size == 0) {
            // not enough space, overwrite oldest
            mBuffer[mCtrl.m_read].reset();
            mCtrl.consume(1);
            mCtrl.reserve(1);
        }
        mCtrl.commit(1);
    }
    container_type        mBuffer;
    uint32_t              mPrevious;
    bx::RingBufferControl mCtrl;
};

#include <string>

#include "doctest.h"

TEST_CASE("ring_buffer.simple_struct")
{
    struct SimpleStruct {
        SimpleStruct() : A(-1), B("") {}
        int         A;
        std::string B;
    };

    RingBuffer<SimpleStruct, 8> rb;

    CHECK(!rb.Previous());
    rb.Latest().A = 42;
    rb.Latest().B = "42";
    rb.Push();

    rb.Latest().A = 43;
    rb.Latest().B = "43";
    rb.Push();

    rb.Latest().A = 44;
    rb.Latest().B = "44";

    SimpleStruct&                s  = rb.Latest();
    SimpleStruct&                so = rb.Oldest();
    std::optional<SimpleStruct>& sp = rb.Previous();

    CHECK_EQ(s.A, 44);
    CHECK_EQ(so.A, 42);
    CHECK(sp);
    CHECK_EQ(sp->A, 43);
}

TEST_CASE("ring_buffer.round_trip")
{
    RingBuffer<uint32_t, 8> rb;

    for (uint32_t i = 0; i < rb.CAPACITY * 2; ++i) {
        rb.Latest() = i;
        CHECK_EQ(rb.Latest(), i);
        if (i != 0) {
            CHECK(rb.Previous());
            CHECK_EQ(*rb.Previous(), i - 1);
        }
        rb.Push();
    }
}
