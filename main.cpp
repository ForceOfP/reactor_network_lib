#include <gtest/gtest.h>
#include "test/test.cpp"
#include "init.hpp"

int main() {
    init_log();
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}

