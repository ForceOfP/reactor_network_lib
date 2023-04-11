#ifndef Poller_Marco
#define Poller_Marco

#include "EventLoop.hpp"
#include <vector>
#include <map>
#include <chrono>
#include "../time_utils/TimeDataStructure.hpp"

struct pollfd;

class Channel;

class Poller {
public:
    using ChannelList = std::vector<Channel*>;
    Poller(EventLoop* loop);
    ~Poller() = default;

    Poller(const Poller&) = delete;
    Poller& operator=(const Poller&) = delete;

    TimeStamp poll(int time_out_ms, ChannelList* active_channels);

    void update_channel(Channel* channel);
    void remove_channel(Channel* channel);

    void assert_in_loop_thread() {
        owner_loop_->assert_in_loop_thread();
    }
private:
    void fill_active_channels(int num_events, ChannelList* active_channels) const;

    using PollFdList = std::vector<struct pollfd>;
    using ChannelMap = std::map<int, Channel*>;

    EventLoop* owner_loop_;
    PollFdList poll_fds_;
    ChannelMap channels_;
};  

#endif
