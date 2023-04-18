#pragma once

#ifndef EL_MARCO
#define EL_MARCO

#include "../logger/logger.hpp"
#include "../time_utils/TimerId.hpp"
#include "../time_utils/TimeDataStructure.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class Channel;
class PollerInterface;
class TimerQueue;

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    EventLoop(const EventLoop& src) = delete;
    EventLoop& operator=(const EventLoop& rhs) = delete;

    using Functor = std::function<void()>;

    void loop();
    void assert_in_loop_thread() {
        if (!is_in_loop_thread()) {
            abort_not_in_loop_thread();
        }
    }

    bool is_in_loop_thread() const {
        return thread_id_ == std::this_thread::get_id();
    }

    EventLoop* get_event_loop_of_current_thread();
    void update_channel(Channel* channel);
    void remove_channel(Channel* channel);
    void quit();

    TimerId run_at(const TimeStamp& time, const TimerCallback& cb);
    TimerId run_after(double delay, const TimerCallback& cb);
    TimerId run_every(double interval, const TimerCallback& cb);

    void run_in_loop(const Functor& cb);
    void queue_in_loop(const Functor& cb);

    void wakeup();
    void cancel(TimerId id);
private:
    void abort_not_in_loop_thread();
    void handle_read();
    void do_pending_functors();

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_;
    std::atomic_bool quit_;
    std::atomic_bool calling_pending_functors_;
    const std::thread::id thread_id_;
    std::unique_ptr<PollerInterface> poller_;
    std::unique_ptr<TimerQueue> timer_queue_;
    int wakeup_fd_;
    std::unique_ptr<Channel> wakeup_channel_;
    ChannelList active_channels_;
    mutable std::mutex pending_mut_;
    std::vector<Functor> pending_functors_;

    TimeStamp poll_return_time_;
};

#endif