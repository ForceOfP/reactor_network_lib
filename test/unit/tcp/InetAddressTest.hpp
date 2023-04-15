#include <gtest/gtest.h>
#include "../../../tcp/InetAddress.hpp"

TEST(InetAddress, to_host_port) {
    InetAddress addr("127.0.0.1", 9981); 
    ASSERT_EQ(addr.to_host_port(), "127.0.0.1:9981");
}