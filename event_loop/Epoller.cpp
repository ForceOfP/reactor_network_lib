#include "Epoller.hpp"

#include "Channel.hpp"
#include "../logger/logger.hpp"
#include "EventLoop.hpp"

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fmt/format.h>

// On Linux, the constants of poll(2) and epoll(4)
// are expected to be the same.
static_assert(EPOLLIN == POLLIN);
static_assert(EPOLLPRI == POLLPRI);
static_assert(EPOLLOUT == POLLOUT);
static_assert(EPOLLRDHUP == POLLRDHUP);
static_assert(EPOLLERR == POLLERR);
static_assert(EPOLLHUP == POLLHUP);

namespace
{
    const int k_new = -1;
    const int k_added = 1;
    const int k_deleted = 2;
}

Epoller::Epoller(EventLoop* loop):
    owner_loop_(loop),
    epoll_fd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(k_init_event_list_size) {
    if (epoll_fd_ < 0) {
        LOG_SYSFATAL << "Epoller::Epoller";
    }
}

Epoller::~Epoller() {
    ::close(epoll_fd_);
}

TimeStamp Epoller::poll(int timeout_ms, ChannelList* active_channels) {
    int num_events = ::epoll_wait(
        epoll_fd_, events_.data(), static_cast<int>(events_.size()), timeout_ms);
    TimeStamp now(TimeStamp::now());
    if (num_events > 0) {
        LOG_TRACE << fmt::format("{} events happened", num_events);
        fill_active_channels(num_events, active_channels);
        if (static_cast<size_t>(num_events) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (num_events == 0) {
        LOG_TRACE << " nothing happended";
    } else {
        LOG_SYSERR << "EPoller::poll()";
    }
    return now;
}

void Epoller::fill_active_channels(int num_events, ChannelList* active_channels) const {
    assert(static_cast<size_t>(num_events) <= events_.size());
    for (size_t i = 0; i < num_events; i++) {
        auto* channel = static_cast<Channel*>(events_[i].data.ptr);
    #ifndef NDEBUG
        int fd = channel->get_fd();
        auto it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
    #endif
        channel->set_revents(events_[i].events);
        active_channels->push_back(channel);
    }
}

void Epoller::update_channel(Channel *channel) {
    assert_in_loop_thread();
    LOG_TRACE << fmt::format("fd = {} events = {}", channel->get_fd(), channel->get_events());
    const int index = channel->get_index();
    if (index == k_new || index == k_deleted) {
        int fd = channel->get_fd();
        if (index == k_new) {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        } else {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->set_index(k_added);
        update(EPOLL_CTL_ADD, channel);
    } else {
        int fd = channel->get_fd();
        (void)fd;
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == k_added);
        if (channel->is_none_event()) {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(k_deleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void Epoller::remove_channel(Channel *channel) {
    assert_in_loop_thread();
    int fd = channel->get_fd();
    LOG_TRACE << fmt::format("fd = {}", fd);
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->is_none_event());
    int index = channel->get_index();
    assert(index == k_added || index == k_deleted);
    size_t n = channels_.erase(fd);
    (void)n;
    assert(n == 1);

    if (index == k_added) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(k_new);
}

void Epoller::update(int operation, Channel* channel) {
    struct epoll_event event;
    bzero(&event, sizeof event);
    event.events = channel->get_events();
    event.data.ptr = channel;
    int fd = channel->get_fd();
    if (::epoll_ctl(epoll_fd_, operation, fd, &event)) {
        if (operation == EPOLL_CTL_DEL) {
            LOG_SYSERR << fmt::format( "epoll_ctl op = {} fd = {}", operation, fd);
        } else {
            LOG_SYSFATAL << fmt::format( "epoll_ctl op = {} fd = {}", operation, fd);;
        }
    }
}