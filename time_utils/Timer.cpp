#include "Timer.hpp"

void Timer::restart(TimeStamp now) {
    if (repeat_) {
        expiration_ = add_time(now, interval_);
    } else {
        expiration_ = {};
    }
}