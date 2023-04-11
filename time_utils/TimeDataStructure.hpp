#pragma once

#ifndef Time_Marco
#define Time_Marco

#include <cstdint>
#include <functional>
#include <chrono>

using namespace std::literals;

using TimerCallback = std::function<void()>;

class TimeStamp {
public:
    TimeStamp(): valid_(false) {}
    TimeStamp(std::chrono::system_clock::time_point t): val_(t), valid_(true) {}

    TimeStamp(const TimeStamp&) = default;
    TimeStamp& operator=(const TimeStamp&) = default;
    TimeStamp(TimeStamp&&) = default;
    TimeStamp& operator=(TimeStamp&&) = default;

    std::chrono::system_clock::time_point value() {
        return val_;
    }

    friend auto operator<=>(const TimeStamp& a, const TimeStamp& b) {
        return a.val_ <=> b.val_;
    }

    bool valid() {
        return valid_;
    }

    static TimeStamp now() {
        return {std::chrono::system_clock::now()};
    }

    std::string to_string() {
        std::time_t now_time_t = std::chrono::system_clock::to_time_t(this->value());
        std::tm* now_tm = std::localtime(&now_time_t);

        char buffer[128];
        strftime(buffer, sizeof(buffer), "[%F %T", now_tm);
        std::ostringstream ss;
        ss.fill('0');

        std::chrono::milliseconds ms;

        ms = std::chrono::duration_cast<std::chrono::milliseconds>(this->value().time_since_epoch()) % 1000;
        ss << buffer << '.' << ms.count() << "] ";

        return ss.str();
    }
private:
    std::chrono::system_clock::time_point val_;
    bool valid_;
};

const static uint64_t k_micro_seconds_per_second = 1000'000;
const static uint64_t k_nano_seconds_per_second = 1000'000'000;

// const static TimeStamp MAX_TIME = std::chrono::system_clock::from_time_t(UINT64_MAX);

#endif