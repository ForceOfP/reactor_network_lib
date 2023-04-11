#pragma once

#ifndef TimerId_Marco
#define TimerId_Marco

#include <memory>
#include <utility>

class Timer;

class TimerId {
public:
    TimerId() = default;
    TimerId(std::weak_ptr<Timer>  t): value_(std::move(t)) {}
    // TimerId(const TimerId& src) = default;
    // TimerId& operator=(const TimerId& src) = default;

    std::weak_ptr<Timer> get_value();
private:
    std::weak_ptr<Timer> value_;
};

#endif