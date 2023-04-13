#pragma once

#ifndef Event_Loop_Thread_Pool_Marco
#define Event_Loop_Thread_Pool_Marco

#include <memory>
#include <vector>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool {
public:
    EventLoopThreadPool(EventLoop* base_loop);
    ~EventLoopThreadPool() = default;

    EventLoopThreadPool(const EventLoopThreadPool&) = delete;
    EventLoopThreadPool& operator=(const EventLoopThreadPool&) = delete;

    void set_thread_num(int num_threads) {num_threads_ = num_threads;}
    size_t get_thread_num() {return threads_.size();}
    void start();
    EventLoop* get_next_loop();
private:
    EventLoop* base_loop_;
    bool started_;
    int num_threads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};

#endif