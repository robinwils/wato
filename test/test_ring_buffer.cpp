#include <doctest.h>

#include <string>

#include <core/queue/ring_buffer.hpp>

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

    for (uint32_t i = 0; i < rb.kCapacity * 2; ++i) {
        rb.Latest() = i;
        CHECK_EQ(rb.Latest(), i);
        if (i != 0) {
            CHECK(rb.Previous());
            CHECK_EQ(*rb.Previous(), i - 1);
        }
        rb.Push();
    }
}
