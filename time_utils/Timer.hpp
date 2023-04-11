#pragma once

#ifndef Timer_Marco
#define Timer_Marco

#include "TimeDataStructure.hpp"
#include <chrono>
#include <memory>
#include <utility>

class Timer: public std::enable_shared_from_this<Timer> {
public:
    Timer(TimerCallback cb, TimeStamp when, double interval):
        callback_(std::move(cb)), expiration_(when), interval_(interval), repeat_(interval > 0.0) {}
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
/*     Timer(Timer&&) = default;
    Timer& operator=(Timer&&) = default; */


    void run() const {
        callback_();
    }

    TimeStamp get_expiration() const {return expiration_;}
    bool repeat() const {return repeat_;}

    void restart(TimeStamp now); 
private:
    const TimerCallback callback_;
    TimeStamp expiration_;
    const double interval_;
    const bool repeat_;
};

inline TimeStamp add_time(TimeStamp time_stamp, double seconds) {
    auto t = time_stamp.value();
    t += std::chrono::microseconds(int(seconds * 1000000));
    return {t};
}

#endif