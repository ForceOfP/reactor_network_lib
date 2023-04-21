#pragma once
#ifndef Thread_Pool_Marco
#define Thread_Pool_Marco

#include "ThreadsafeQueue.hpp"
#include "JoinThreads.hpp"
#include "FunctionWrapper.hpp"
#include "WorkStealingQueue.hpp"
#include "adding/HandlerEventLoopThread.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <future>
#include <type_traits>
#include <vector>

class thread_pool {
private:
    using task_t = function_wrapper;
    std::atomic_bool done_;
    threadsafe_queue<task_t> pool_work_queue_;
    std::vector<std::unique_ptr<work_stealing_queue>> queues_;
    std::vector<std::unique_ptr<HandlerEventLoopThread>> threads_;
    //join_threads joiner_;
    inline static thread_local work_stealing_queue* local_work_queue_;
    inline static thread_local unsigned my_index_;
private:
    void worker_thread(unsigned index) {
        my_index_ = index;
        local_work_queue_ = queues_[my_index_].get();
        while (!done_) {
            run_pending_task();
        }
    }

    bool pop_task_from_local_queue(task_t& task) {
        return local_work_queue_ && local_work_queue_->try_pop(task);
    }

    bool pop_task_from_pool_queue(task_t& task) {
        return pool_work_queue_.try_pop(task);
    }

    bool pop_task_from_other_thread(task_t& task) {
        for (unsigned i = 0; i < queues_.size(); i++) {
            unsigned const index = (my_index_ + 1) % queues_.size();
            if (queues_[index]->try_steal(task)) {
                return true;
            }
        }
        return false;
    }
public:
    thread_pool(): done_(false) {
        unsigned const thread_const = std::thread::hardware_concurrency();
        try {
            for (unsigned i = 0; i < thread_const; i++) {
                queues_.push_back(std::make_unique<work_stealing_queue>());
            }
            for (unsigned i = 0; i < thread_const; i++) {
                threads_.push_back(std::make_unique<HandlerEventLoopThread>([this] {run_pending_task();}));
            }
        } catch(...) {
            done_ = true;
            throw;
        }
    }

    ~thread_pool() {
        done_ = true;
    }

    template<std::invocable FunctionType> 
    std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType f) {
        using result_t = typename std::result_of<FunctionType()>::type;
        std::packaged_task<result_t()> task(std::move(f));
        std::future<result_t> res(task.get_future());
        if (local_work_queue_) {
            local_work_queue_->push(std::move(task));
        } else {
            pool_work_queue_.push(std::move(task));
        }
        return res;
    }

    void run_pending_task() {
        task_t task;
        if (pop_task_from_local_queue(task)
            || pop_task_from_pool_queue(task)
            || pop_task_from_other_thread(task)) {
            task();
        } else {
            std::this_thread::yield();
        }
    }
};

#endif