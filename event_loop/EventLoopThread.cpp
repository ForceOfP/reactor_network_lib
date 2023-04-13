#include "EventLoopThread.hpp"
#include "EventLoop.hpp"
#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

EventLoopThread::EventLoopThread():
    loop_(nullptr),
    exiting_(false), 
    thread_(),
    mut_(),
    cond_()
{}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    loop_->quit();
    if (thread_.joinable()) thread_.join();
}

EventLoop* EventLoopThread::start_loop() {
    thread_ = std::thread([this] { thread_func(); });

    {
        std::unique_lock<std::mutex> lock(mut_);
        while (loop_ == nullptr) {
            cond_.wait(lock);
        }
    }

    return loop_;
}

void EventLoopThread::thread_func() {
    EventLoop loop;

    {
        std::lock_guard<std::mutex> lock(mut_);
        loop_ = &loop;
        cond_.notify_all();
    }

    loop.loop();
}