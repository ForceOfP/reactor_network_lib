#include "TimerId.hpp"

#include "Timer.hpp"

std::weak_ptr<Timer> TimerId::get_value() {
    return value_;
}