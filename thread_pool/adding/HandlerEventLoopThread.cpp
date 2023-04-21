#include "HandlerEventLoopThread.hpp"
#include "HandlerEventLoop.hpp"
#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

HandlerEventLoopThread::HandlerEventLoopThread(std::function<void ()> f):
    loop_(nullptr),
    exiting_(false), 
    thread_(),
    mut_(),
    cond_(),
    functor_(std::move(f))
{}

HandlerEventLoopThread::~HandlerEventLoopThread() {
    exiting_ = true;
    loop_->quit();
    if (thread_.joinable()) thread_.join();
}

HandlerEventLoop* HandlerEventLoopThread::start_loop() {
    thread_ = std::thread([this] { thread_func(); });

    {
        std::unique_lock<std::mutex> lock(mut_);
        while (loop_ == nullptr) {
            cond_.wait(lock);
        }
    }

    return loop_;
}

void HandlerEventLoopThread::thread_func() {
    HandlerEventLoop loop;

    {
        std::lock_guard<std::mutex> lock(mut_);
        loop_ = &loop;
        loop_->set_looping_function(functor_);
        cond_.notify_all();
    }

    loop.loop();
}
