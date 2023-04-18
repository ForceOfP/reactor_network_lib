#ifndef Poller_Marco
#define Poller_Marco

#include "../event_loop/EventLoop.hpp"
#include "PollerInterface.hpp"
#include <vector>
#include <map>
#include <chrono>
#include "../time_utils/TimeDataStructure.hpp"

struct pollfd;

class Channel;

class Poller: public PollerInterface {
public:
    using ChannelList = std::vector<Channel*>;
    Poller(EventLoop* loop);
    ~Poller() override = default;

    Poller(const Poller&) = delete;
    Poller& operator=(const Poller&) = delete;

    TimeStamp poll(int time_out_ms, ChannelList* active_channels) override;

    void update_channel(Channel* channel) override;
    void remove_channel(Channel* channel) override;

    void assert_in_loop_thread() override {
        owner_loop_->assert_in_loop_thread();
    }
private:
    void fill_active_channels(int num_events, ChannelList* active_channels) const override;

    using PollFdList = std::vector<struct pollfd>;
    using ChannelMap = std::map<int, Channel*>;

    EventLoop* owner_loop_;
    PollFdList poll_fds_;
    ChannelMap channels_;
};  

#endif
