#pragma once

#include <functional>
#ifndef Handler_Event_Loop_Thread_Marco
#define Handler_Event_Loop_Thread_Marco

#include <condition_variable>
#include <thread>
class HandlerEventLoop;

class HandlerEventLoopThread {
public:
    HandlerEventLoopThread(std::function<void ()> f);
    ~HandlerEventLoopThread();

    HandlerEventLoopThread(const HandlerEventLoopThread&) = delete;
    HandlerEventLoopThread& operator=(const HandlerEventLoopThread&) = delete;

    HandlerEventLoop* start_loop();

    //void set_looping_function(std::function<void ()> f);
private:
    void thread_func();
    
    HandlerEventLoop* loop_;
    bool exiting_;
    std::thread thread_;
    mutable std::mutex mut_;
    std::condition_variable cond_;

    std::function<void ()> functor_;
};

#endif