#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "../../../buffer/Buffer.hpp"

#define my_gtest

TEST(BufferTest, ctor) {
    Buffer a;
    auto b = new Buffer();
    auto c = std::make_unique<Buffer>();
    auto d = std::move(c); 
    delete(b);
    ASSERT_EQ(1, 1);
}

TEST(BufferTest, swap) {
    Buffer a;
    Buffer b;
    Buffer c;
    a.append("aaa");
    b.append("aaa");
    c.swap(b);
    ASSERT_EQ(a, c);
    ASSERT_NE(a, b);
}

TEST(BufferTest, iter) {
    Buffer a;
    a.append("xxx");
    ASSERT_EQ(a.readable_bytes(), 3);
    ASSERT_EQ(a.writable_bytes(), 1024-3);
    ASSERT_EQ(a.prependable_bytes(), 8);

    std::string appender(1024, 'c');
    a.append(appender); // resize to two times, length = 2*(k_init+k_pre)
    ASSERT_EQ(a.readable_bytes(), 1024+3);
    ASSERT_EQ(a.writable_bytes(), 0);
    ASSERT_EQ(a.prependable_bytes(), 8);

    appender = "tt";
    a.prepend(appender.data(), 2); // fill from start, this is an error use, just for test
    ASSERT_EQ(a.readable_bytes(), 1024+3+2);
    ASSERT_EQ(a.writable_bytes(), 0);
    ASSERT_EQ(a.prependable_bytes(), 6);

    a.retrieve(3);
    ASSERT_EQ(a.readable_bytes(), 1024+2);
    ASSERT_EQ(a.writable_bytes(), 0);
    ASSERT_EQ(a.prependable_bytes(), 6+3);

    a.has_written(2); // wrong use for test!
    ASSERT_EQ(a.readable_bytes(), 1024+2+2);
    ASSERT_EQ(a.writable_bytes(), -2);

    a.retrieve_all();
    ASSERT_EQ(a.readable_bytes(), 0);
    ASSERT_EQ(a.writable_bytes(), 1024+3);
    ASSERT_EQ(a.prependable_bytes(), 8);

    Buffer b;
    std::string appender2(1024, 'c');
    b.append(appender2);
    b.shrink(40);
    ASSERT_EQ(b.readable_bytes(), 1024);
    ASSERT_EQ(b.writable_bytes(), 40);
}

TEST(BufferTest, content) {
    Buffer a;
    a.append("xxx");
    ASSERT_EQ(a.retrieve_as_string(), "xxx");

    a.append("cccccaaa");

    a.retrieve(6);
    ASSERT_EQ(a.retrieve_as_string(), "aa");

    a.append("zzz");
    a.has_written(3);
    ASSERT_EQ(a.retrieve_as_string(), "zzzcca");

    a.retrieve_all();
    ASSERT_EQ(a.retrieve_as_string(), "");
}

TEST(BufferTest, read_fd) {
    // @TODO: fill it.
}
