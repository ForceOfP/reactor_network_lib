#include "TimerQueue.hpp"
#include "../logger/logger.hpp"
#include "TimeDataStructure.hpp"
#include "TimerId.hpp"
#include "Timer.hpp"
#include "../event_loop/EventLoop.hpp"

#include <chrono>
#include <cstdint>
#include <fmt/format.h>
#include <functional>
#include <iterator>
#include <memory>
#include <sys/timerfd.h>
#include <cassert>
#include <utility>
#include <vector>

int create_timer_fd() {
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                 TFD_NONBLOCK | TFD_CLOEXEC);

    if (timerfd < 0) {
        LOG_SYSFATAL << "Failed in timerfd_create";
    }
    return timerfd;
}

struct timespec how_much_time_from_now(TimeStamp when) {
    using namespace std::literals;
    uint64_t ms_interval = (when.value() - std::chrono::system_clock::now()) / std::chrono::microseconds(1);
    if (ms_interval < 100) {
        ms_interval = 100;
    }

    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(
        ms_interval / k_micro_seconds_per_second);
    ts.tv_nsec = static_cast<long>(
        (ms_interval % k_micro_seconds_per_second) * 1000);
    return ts;
}

static std::string to_string(TimeStamp& t) {
	std::time_t now_time_t = std::chrono::system_clock::to_time_t(t.value());
    std::tm* now_tm = std::localtime(&now_time_t);

	char buffer[128];
	strftime(buffer, sizeof(buffer), "[%F %T", now_tm);
	std::ostringstream ss;
	ss.fill('0');

	std::chrono::milliseconds ms;

    ms = std::chrono::duration_cast<std::chrono::milliseconds>(t.value().time_since_epoch()) % 1000;
    ss << buffer << '.' << ms.count() << "] ";

	return ss.str();
}

void read_timer_fd(int timerfd, TimeStamp now) {
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    LOG_TRACE << fmt::format("TimerQueue::handleRead() {} at {}", howmany, to_string(now));
    if (n != sizeof howmany) {
        LOG_ERROR << fmt::format("TimerQueue::handleRead() reads {} bytes instead of 8", n);
    }
}

void reset_timer_fd(int timerfd, TimeStamp expiration) {
    // wake up loop by timerfd_settime()
    struct itimerspec newValue;
    struct itimerspec oldValue;
    bzero(&newValue, sizeof newValue);
    bzero(&oldValue, sizeof oldValue);
    newValue.it_value = how_much_time_from_now(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret) {
        LOG_SYSERR << "timerfd_settime()";
    }
}

TimerQueue::TimerQueue(EventLoop* loop):
    loop_(loop), timer_fd_(create_timer_fd()), timer_fd_channel_(loop, timer_fd_), timers_(TimerList{}) {
    timer_fd_channel_.set_read_callback(std::bind(&TimerQueue::handle_read, this));
    timer_fd_channel_.enable_reading();
}

TimerQueue::~TimerQueue() {
    ::close(timer_fd_);
}

TimerId TimerQueue::add_timer(const TimerCallback &cb, TimeStamp when, double interval) {
    std::shared_ptr<Timer> timer = std::make_shared<Timer>(cb, when, interval);

    loop_->run_in_loop([this, timer] { add_timer_in_loop(timer); });
    return {timer};
}

void TimerQueue::add_timer_in_loop(std::shared_ptr<Timer> timer) {
    loop_->assert_in_loop_thread();
    auto [earliest_changed, timer_pointer] = insert(std::move(timer));
    if (earliest_changed) {
        if (auto p = timer_pointer.lock()) {
            reset_timer_fd(timer_fd_, p->get_expiration());
        }
    }
}

void TimerQueue::handle_read() {
    loop_->assert_in_loop_thread();
    TimeStamp now = std::chrono::system_clock::now();
    read_timer_fd(timer_fd_, now);

    std::vector<Entry> expired = get_expired(now);

    for (auto& e: expired) {
        e.second->run();
    }

    reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::get_expired(TimeStamp now) {
    auto last_iter = timers_.lower_bound(now);
    assert(last_iter == timers_.end() || now < last_iter->first);
    std::vector<Entry> expired(std::make_move_iterator(timers_.begin()), std::make_move_iterator(last_iter));
    timers_.erase(timers_.begin(), last_iter);
    return expired;
}

void TimerQueue::reset(std::vector<Entry>& expired, TimeStamp now) {
    TimeStamp next_expire;

    for (auto& e: expired) {
        if (e.second->repeat()) {
            e.second->restart(now);
            insert(e.second);
        }
    }

    if (!timers_.empty()) {
        next_expire = timers_.begin()->second->get_expiration();
    }

    if (next_expire.valid()) {
        reset_timer_fd(timer_fd_, next_expire);
    }
}

std::pair<bool, std::weak_ptr<Timer>> TimerQueue::insert(std::shared_ptr<Timer> timer) {
    bool earliestChanged = false;
    TimeStamp when = timer->get_expiration();
    auto iter = timers_.begin();
    if (iter == timers_.end() || when < iter->first) {
        earliestChanged = true;
    }
    auto result = timers_.insert(std::make_pair(when, std::move(timer)));
    return {earliestChanged, result->second};
}

void TimerQueue::cancel(TimerId id) {
    loop_->run_in_loop([this, id] {cancel_in_loop(id);});
}

void TimerQueue::cancel_in_loop(TimerId id) {
    if (auto x = id.get_value().lock()) {
        for (auto [k, v]: timers_) {
            if (x == v) {
                timers_.erase(k);
                break;
            }
        }
    }
}