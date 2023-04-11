#pragma once

#ifndef Timer_Queue_Marco
#define Timer_Queue_Marco

#include <functional>
#include <memory>
#include <utility>
#include <set>
#include <vector>
#include <map>
#include "TimeDataStructure.hpp"
#include "../event_loop/Channel.hpp"

class EventLoop;
class TimerId;
class Timer;

class TimerQueue {
public:
    using Entry = std::pair<TimeStamp, std::shared_ptr<Timer>>;
    using TimerList = std::multimap<TimeStamp, std::shared_ptr<Timer>, std::less<TimeStamp>>;
    TimerQueue(EventLoop* loop);
    ~TimerQueue();

    TimerId add_timer(const TimerCallback& cb, TimeStamp when, double interval);

    void cancel(TimerId id);
private:
    void add_timer_in_loop(std::shared_ptr<Timer>);
    void handle_read();

    std::vector<Entry> get_expired(TimeStamp now);
    void reset(std::vector<Entry>& expired, TimeStamp now);

    std::pair<bool, std::weak_ptr<Timer>> insert(std::shared_ptr<Timer>);
    void cancel_in_loop(TimerId id);

    EventLoop* loop_;
    const int timer_fd_;
    Channel timer_fd_channel_;
    TimerList timers_;
};

#endif