#include <gtest/gtest.h>
#include "../../../event_loop/EventLoop.hpp"
#include "../../../event_loop/Channel.hpp"
#include <netinet/in.h>
#include <string>
#include <sys/poll.h>
#include <sys/timerfd.h>
#include <sys/socket.h>

TEST(Channel, read_callback) {
    EventLoop loop;
    int timer_fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    Channel channel(&loop, timer_fd);
    
    std::string result = "nil";
    auto read_functor = [&loop, &result](TimeStamp t) {
        result = "read";
        loop.quit();
    };

    channel.set_read_callback(read_functor);
    channel.enable_reading();

    struct itimerspec how_long;
    bzero(&how_long, sizeof how_long);
    how_long.it_value.tv_nsec = 100000; // 0.1ms
    ::timerfd_settime(timer_fd, 0, &how_long, nullptr);

    loop.loop();
    ::close(timer_fd);
    ASSERT_EQ(result, "read");
}   


TEST(Channel, write_callback) {
    EventLoop loop;

    int file_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct pollfd file_pfd;
    file_pfd.fd = file_fd;
    file_pfd.events = POLLOUT;
    file_pfd.revents = 0;
    
    Channel channel(&loop, file_fd);
    
    std::string result = "nil";
    auto write_functor = [&loop, &result]() {
        result = "write";
        loop.quit();
    };

    channel.set_write_callback(write_functor);
    channel.enable_writing();

    loop.loop();
    
    ::poll(&file_pfd, 1, 100);

    ASSERT_EQ(result, "write");
}   
