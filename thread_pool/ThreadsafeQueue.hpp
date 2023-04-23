#pragma once
#ifndef Threadsafe_Queue_Marco
#define Threadsafe_Queue_Marco

#include <memory>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <utility>

template<typename T>
class ThreadsafeQueue {
private:
    std::queue<std::shared_ptr<T>> data_;
    std::condition_variable data_cond_;
    mutable std::mutex mut_;
public:
    ThreadsafeQueue() = default;

    void push(T value) {
        std::shared_ptr<T> target {std::make_shared<T>(std::move(value))};        
        std::lock_guard<std::mutex> lock(mut_);
        data_.push(target);
        data_cond_.notify_one();
    }

    void pop() {
        wait_and_pop();
    }

    void wait_and_pop(T& target) {
        std::unique_lock<std::mutex> lock(mut_);
        data_cond_.wait(lock, [this]() {
            return !data_.empty();
        });
        target = std::move(*data_.front());
        data_.pop();
    }

    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> lock(mut_);
        data_cond_.wait(lock, [this]() {
            return !data_.empty();
        });
        std::shared_ptr<T> ret {data_.front()};
        data_.pop();
        return ret;
    }

    bool try_pop(T& target) {
        std::lock_guard<std::mutex> lock(mut_);
        if (data_.empty()) {
            return false;
        }
        target = std::move(*data_.front());
        data_.pop();
        return true;
    }

    std::shared_ptr<T> try_pop() {
        std::lock_guard<std::mutex> lock(mut_);
        if (data_.empty()) {
            return std::make_shared<T>();
        }
        std::shared_ptr<T> ret {*data_.front()};
        data_.pop();
        return ret;        
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mut_);
        return data_.empty();
    }
};

#endif