#include "Poller.hpp"
#include "Channel.hpp"
//#include "EventLoop.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <sstream>
#include <sys/poll.h>

#define FMT_HEADER_ONLY
#include "fmt/format.h"

Poller::Poller(EventLoop* loop): owner_loop_(loop) {}

TimeStamp Poller::poll(int time_out_ms, ChannelList *active_channels) {
    // xxx pollfds_ shouldn't change
    int num_events = ::poll(&*poll_fds_.data(), poll_fds_.size(), time_out_ms);
    TimeStamp now{std::chrono::system_clock::now()};

    if (num_events > 0) {
        LOG_TRACE << fmt::format("{} events happened", num_events);
        fill_active_channels(num_events, active_channels);
    } else if (num_events == 0) {
        LOG_TRACE << "nothing happened";
    } else {
        LOG_SYSERR << "Poller::poll()";
    }
    return now;
}

void Poller::fill_active_channels(int num_events, ChannelList* active_channels) const {
    for (auto pfd_iter = poll_fds_.cbegin(); pfd_iter != poll_fds_.cend(); pfd_iter++) {
        if (pfd_iter->revents > 0) {
            num_events--;
            auto channel_iter = channels_.find(pfd_iter->fd);
            assert(channel_iter != channels_.end());
            Channel* channel = channel_iter->second;
            assert(channel->get_fd() == pfd_iter->fd);
            channel->set_revents(pfd_iter->revents);
            active_channels->push_back(channel);
            if (num_events == 0) {
                break;
            }
        }
    }
}

void Poller::update_channel(Channel *channel) {
    assert_in_loop_thread();
    LOG_TRACE << fmt::format("fd = {} events = {}", channel->get_fd(), channel->get_events());

    if (channel->get_index() < 0) {
        // a new one, add to poll_fds_
        assert(channels_.find(channel->get_fd()) == channels_.end());
        struct pollfd new_pfd;
        new_pfd.fd = channel->get_fd();
        new_pfd.events = static_cast<short>(channel->get_events());
        new_pfd.revents = 0;
        poll_fds_.push_back(new_pfd);
        int idx = static_cast<int>(poll_fds_.size())-1;
        channel->set_index(idx);
        channels_[new_pfd.fd] = channel;
    } else {
        // update existing one
        assert(channels_.find(channel->get_fd()) != channels_.end());
        assert(channels_[channel->get_fd()] == channel);
        int idx = channel->get_index();
        assert(0 <= idx && idx < static_cast<int>(poll_fds_.size()));
        struct pollfd& pfd = poll_fds_[idx];
        assert(pfd.fd == channel->get_fd() || pfd.fd == -1-channel->get_fd());
        pfd.events = static_cast<short>(channel->get_events());
        pfd.revents = 0;
        if (channel->is_none_event()) {
            // ingore this pollfd
            pfd.fd = -1-channel->get_fd();
        }
    }
}

void Poller::remove_channel(Channel *channel) {
    assert_in_loop_thread();
    LOG_TRACE << fmt::format("fd = {}", channel->get_fd());
    assert(channels_.find(channel->get_fd()) != channels_.end());
    assert(channels_[channel->get_fd()] == channel);
    assert(channel->is_none_event());

    int idx = channel->get_index();
    assert(0 <= idx && idx < static_cast<int>(poll_fds_.size()));
    const struct pollfd& pfd = poll_fds_[idx];
    (void)pfd;
    assert(pfd.fd == -channel->get_fd()-1 && pfd.events == channel->get_events());
    size_t n = channels_.erase(channel->get_fd());
    assert(n == 1);
    (void)n;
    if (static_cast<size_t>(idx) == poll_fds_.size()-1) {
        poll_fds_.pop_back();
    } else {
        int channel_at_end = poll_fds_.back().fd;
        iter_swap(poll_fds_.begin()+idx, poll_fds_.end()-1);
        if (channel_at_end < 0) {
            channel_at_end = -channel_at_end-1;
        }
        channels_[channel_at_end]->set_index(idx);
        poll_fds_.pop_back();
    }
}