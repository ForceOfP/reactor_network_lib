#pragma once

#ifndef Event_Loop_Thread_Marco
#define Event_Loop_Thread_Marco

#include <condition_variable>
#include <thread>
class EventLoop;

class EventLoopThread {
public:
    EventLoopThread();
    ~EventLoopThread();

    EventLoopThread(const EventLoopThread&) = delete;
    EventLoopThread& operator=(const EventLoopThread&) = delete;

    EventLoop* start_loop();
private:
    void thread_func();
    
    EventLoop* loop_;
    bool exiting_;
    std::thread thread_;
    mutable std::mutex mut_;
    std::condition_variable cond_;
    bool ready_;
};

#endif