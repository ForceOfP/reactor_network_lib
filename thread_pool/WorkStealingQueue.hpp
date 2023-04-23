#pragma once
#ifndef Work_Stealing_Queue_Marco
#define Work_Stealing_Queue_Marco

#include "FunctionWrapper.hpp"
#include <algorithm>
#include <mutex>
#include <deque>

class WorkStealingQueue {
private:
    using data_t = FunctionWrapper;
    std::deque<data_t> mission_queue_;
    mutable std::mutex mut_;
public:
    WorkStealingQueue() = default;
    WorkStealingQueue& operator= (const WorkStealingQueue& rhs) = delete;
    WorkStealingQueue(const WorkStealingQueue& src) = delete;

    void push(data_t data) {
        std::lock_guard<std::mutex> lock(mut_);
        mission_queue_.push_front(std::move(data));
    } 

    bool empty() const {
        std::lock_guard<std::mutex> lock(mut_);
        return mission_queue_.empty();
    }

    bool try_pop(data_t& res) {
        std::lock_guard<std::mutex> lock(mut_);
        if (mission_queue_.empty()) {
            return false;
        }
        res = std::move(mission_queue_.front());
        mission_queue_.pop_front();
        return true;
    }

    bool try_steal(data_t& res) {
        std::lock_guard<std::mutex> lock(mut_);
        if (mission_queue_.empty()) {
            return false;
        }
        res = std::move(mission_queue_.back());
        mission_queue_.pop_back();
        return true;
    }
};

#endif