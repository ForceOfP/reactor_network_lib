#include "EventLoop.hpp"
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <sys/types.h>
#include <sys/eventfd.h>
#include <csignal>
#include <thread>
#include <cassert>

#include <poll.h>
#include <unistd.h>
#include <vector>
#include <fmt/format.h>

#include "Channel.hpp"
#include "../poller/DefaultPoller.hpp"
#include "../time_utils/TimerQueue.hpp"
#include "../time_utils/Timer.hpp"

thread_local EventLoop* t_loop_in_this_thread = nullptr;
const int k_poll_time_ms = 10000;

static int create_event_fd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_SYSERR << "Failed in eventfd";
        abort();
    }
    return evtfd;
}

class IngoreSigPipe {
public:
    IngoreSigPipe() {
        ::signal(SIGPIPE, SIG_IGN);
    }
};

IngoreSigPipe init_obj;

EventLoop::EventLoop():
    looping_(false), 
    quit_(false), 
    calling_pending_functors_(false), 
    thread_id_(std::this_thread::get_id()), 
    poller_(newDefaultPoller(this)),
    timer_queue_(std::make_unique<TimerQueue>(this)),
    wakeup_fd_(create_event_fd()),
    wakeup_channel_(std::make_unique<Channel>(this, wakeup_fd_)) {
    std::stringstream ss;
    std::string t;
    ss << thread_id_;
    ss >> t;

    LOG_TRACE << fmt::format("EventLoop created in thread {}", t);

    if (t_loop_in_this_thread) {
        ss.clear();
        
        LOG_FATAL << fmt::format("Another EventLoop exists in this thread {}", t);
    } else {
        t_loop_in_this_thread = this;
    }

    wakeup_channel_->set_read_callback(std::bind(&EventLoop::handle_read, this));
    wakeup_channel_->enable_reading();
}

EventLoop::~EventLoop() {
    assert(!looping_);
    ::close(wakeup_fd_);
    t_loop_in_this_thread = nullptr;
}

EventLoop* EventLoop::get_event_loop_of_current_thread() {
    return t_loop_in_this_thread;
}

void EventLoop::loop() {
    assert(!looping_);
    assert_in_loop_thread();
    looping_ = true;
    quit_ = false;  // FIXME: what if someone calls quit() before loop() ?

    LOG_TRACE << "EventLoop start looping";

    while (!quit_) {
        active_channels_.clear();
        poll_return_time_ = poller_->poll(k_poll_time_ms, &active_channels_);
        // @TODO: sort channel by priority
        for (auto & active_channel : active_channels_) {
            active_channel->handle_event(poll_return_time_);
        }
        do_pending_functors();
    }

    LOG_TRACE << "EventLoop stop looping";
    looping_ = false;
}

void EventLoop::abort_not_in_loop_thread() {
    std::stringstream ss;
    std::string t1, t2;
    ss << thread_id_;
    ss >> t1;
    ss.clear();
    ss << std::this_thread::get_id();
    ss >> t2;
    LOG_FATAL << fmt::format("EventLoop::abortNotInLoopThread - EventLoop was created in thread_id_ = {} current thread id = {}", t1, t2);
}

void EventLoop::quit() {
    quit_ = true;

    if (!is_in_loop_thread()) {
        wakeup();
    }
}

void EventLoop::update_channel(Channel* channel) {
    assert(channel->owner_loop() == this);
    assert_in_loop_thread();
    poller_->update_channel(channel);
}

void EventLoop::remove_channel(Channel *channel) {
    assert(channel->owner_loop() == this);
    assert_in_loop_thread();
    poller_->remove_channel(channel);
}

TimerId EventLoop::run_at(const TimeStamp &time, const TimerCallback &cb) {
    return timer_queue_->add_timer(cb, time, 0.0);
}

TimerId EventLoop::run_after(double delay, const TimerCallback &cb) {
    TimeStamp time(add_time(TimeStamp::now(), delay));
    return run_at(time, cb);
}

TimerId EventLoop::run_every(double interval, const TimerCallback &cb) {
    TimeStamp time(add_time(TimeStamp::now(), interval));
    return timer_queue_->add_timer(cb, time, interval);
}

void EventLoop::run_in_loop(const Functor &cb) {
    if (is_in_loop_thread()) {
        cb();
    } else {
        queue_in_loop(cb);
    }
}

void EventLoop::queue_in_loop(const Functor &cb) {
    {
        std::lock_guard<std::mutex> lock(pending_mut_);
        pending_functors_.push_back(cb);
    }

    if (!is_in_loop_thread() || calling_pending_functors_) {
        wakeup();
    }
}

void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = ::read(wakeup_fd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR << fmt::format("EventLoop::wakeup() writes {} bytes instead of 8", n);
    }
}

void EventLoop::handle_read() {
    uint64_t one = 1;
    ssize_t n = ::read(wakeup_fd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR << fmt::format("EventLoop::handle_read() writes {} bytes instead of 8", n);
    }
}

void EventLoop::do_pending_functors() {
    std::vector<Functor> functors;
    calling_pending_functors_ = true;

    {
        std::lock_guard<std::mutex> lock(pending_mut_);
        functors.swap(pending_functors_);
    }

    for (const auto& f: functors) {
        f();
    }
    calling_pending_functors_ = false;
}

void EventLoop::cancel(TimerId id) {
    return timer_queue_->cancel(std::move(id));
}