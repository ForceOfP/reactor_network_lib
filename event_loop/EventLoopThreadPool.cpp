#include "EventLoopThreadPool.hpp"
#include "../event_loop/EventLoopThread.hpp"
#include "../event_loop/EventLoop.hpp"
#include <cassert>
#include <cstddef>
#include <memory>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop):
    base_loop_(base_loop), started_(false), num_threads_(0), next_(0) {

}

void EventLoopThreadPool::start() {
    assert(!started_);
    base_loop_->assert_in_loop_thread();

    started_ = true;

    for (int i = 0; i < num_threads_; i++) {
        auto t = std::make_unique<EventLoopThread>();
        threads_.push_back(std::move(t));
        loops_.push_back(t->start_loop()); 
    }
}

EventLoop* EventLoopThreadPool::get_next_loop() {
    base_loop_->assert_in_loop_thread();
    EventLoop* loop = base_loop_;

    if (!loops_.empty()) {
        loop = loops_[next_];
        next_++;
        if (static_cast<size_t>(next_) >= loops_.size()) {
            next_ = 0;
        }
    }
    return loop;
}
