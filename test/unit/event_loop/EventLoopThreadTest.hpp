#include <gtest/gtest.h>
#include "../../../event_loop/EventLoop.hpp"
#include "../../../event_loop/EventLoopThread.hpp"

TEST(EventLoopThread, start_loop) {
    EventLoopThread loop_thread;
    EventLoop *loop = loop_thread.start_loop();

    auto test = [&loop]() {
        loop->quit();
        ASSERT_TRUE(true);
    };

    loop->run_in_loop(test);
}