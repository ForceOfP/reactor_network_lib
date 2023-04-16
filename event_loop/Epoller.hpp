#pragma once

#ifndef Epoller_Marco
#define Epoller_Marco

#include "EventLoop.hpp"
#include "PollerInterface.hpp"
#include <map>
#include <vector>
struct epoll_event;

class Channel;
///
/// IO Multiplexing with epoll(4).
///
/// This class doesn't own the Channel objects.
class Epoller: public PollerInterface {
public:
    Epoller(EventLoop* loop);
    ~Epoller() override;

    Epoller(const Epoller&) = delete;
    Epoller& operator=(const Epoller&) = delete;

    using ChannelList = std::vector<Channel*>;

    /// Polls the I/O events.
    /// Must be called in the loop thread.
    TimeStamp poll(int timeout_ms, ChannelList* active_channel) override;
    /// Changes the interested I/O events.
    /// Must be called in the loop thread.
    void update_channel(Channel* channel) override;
    /// Remove the channel, when it destructs.
    /// Must be called in the loop thread.
    void remove_channel(Channel* channel) override;

    void assert_in_loop_thread() override {owner_loop_->assert_in_loop_thread();}
private:
    static const int k_init_event_list_size = 16;

    void fill_active_channels(int num_events, ChannelList* active_channel) const override;
    void update(int operation, Channel* channel);

    using EventList = std::vector<struct epoll_event>;
    using ChannelMap = std::map<int, Channel*>;
    
    EventLoop* owner_loop_;
    int epoll_fd_;
    EventList events_;
    ChannelMap channels_;
};

#endif