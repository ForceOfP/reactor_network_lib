#pragma once

#ifndef Poller_Marco_Interface
#define Poller_Marco_Interface

#include "EventLoop.hpp"
#include <vector>
#include "../time_utils/TimeDataStructure.hpp"

struct pollfd;

class Channel;

class PollerInterface {
public:
    using ChannelList = std::vector<Channel*>;
    virtual ~PollerInterface() = default;

    virtual TimeStamp poll(int time_out_ms, ChannelList* active_channels) = 0;
    virtual void update_channel(Channel* channel) = 0;
    virtual void remove_channel(Channel* channel) = 0;
    virtual void assert_in_loop_thread() = 0;
private:
    virtual void fill_active_channels(int num_events, ChannelList* active_channels) const = 0;
};

#endif