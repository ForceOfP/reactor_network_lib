#include <chrono>
#include <functional>
#include <gtest/gtest.h>
#include "../../../event_loop/EventLoop.hpp"

#include <thread>
#include <future>

bool run_in_another_thread() {
    EventLoop loop;
    auto run_another = [&loop]() {
        return loop.is_in_loop_thread();
    };
    std::future<bool> result = std::async(std::launch::async,run_another);
    result.wait();
    return result.get();
}

bool run_in_this_thread() {
    auto run_this = []() {
        EventLoop loop;
        return loop.is_in_loop_thread();
    };
    std::future<bool> result = std::async(std::launch::async, run_this);
    result.wait();
    return result.get();
}

TEST(EventLoop, is_loop_in_thread) {
    ASSERT_FALSE(run_in_another_thread());
    ASSERT_TRUE(run_in_this_thread());
}

TEST(EventLoop, get_event_loop_of_current_thread) {
    EventLoop loop;

    auto test = [&loop]() {
        auto l = loop.get_event_loop_of_current_thread();
        ASSERT_TRUE(l->is_in_loop_thread());
        loop.quit();
    };

    loop.run_after(0.00001, test);
    loop.loop();
}

TEST(EventLoop, quit) {
    EventLoop loop;

    auto test = [&loop]() {
        loop.quit();
    };

    loop.run_after(0.00001, test);
    loop.loop();
    ASSERT_TRUE(true);
}

TEST(EventLoop, run_after) {
    const auto delay = 1ms;
    EventLoop loop;

    auto test = [&loop]() {
        loop.quit();
    };

    loop.run_after(0.0001, test); // 0.1ms
    auto start = std::chrono::system_clock::now();
    loop.loop();
    auto end = std::chrono::system_clock::now();
    ASSERT_TRUE(end - start < delay);
}

TEST(EventLoop, run_every) {
    const auto delay = 4ms;
    EventLoop loop;
    int cnt = 0;
    auto test = [&loop, &cnt]() {
        cnt++;
        if (cnt == 4) {
            loop.quit();
        }
    };

    loop.run_every(0.0001, test); // 0.1ms
    auto start = std::chrono::system_clock::now();
    loop.loop();
    auto end = std::chrono::system_clock::now();
    ASSERT_TRUE(end - start < delay);
    ASSERT_EQ(cnt, 4);
}

TEST(EventLoop, run_in_loop) {
    EventLoop loop;

    auto test = [&loop]() {
        ASSERT_TRUE(true);
        loop.quit();
    };

    loop.queue_in_loop(test);
}

// TODO: mock IO-thread and test
TEST(EventLoop, wakeup) {
    EventLoop loop;

    auto test = [&loop]() {
        loop.wakeup();
        loop.quit();
        ASSERT_TRUE(true);
    };

    loop.run_in_loop(test);
}