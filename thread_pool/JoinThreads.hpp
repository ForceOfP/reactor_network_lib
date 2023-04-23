#pragma once
#ifndef Join_Threads_Marcp
#define Join_Threads_Marcp

#include <thread>
#include <vector>

class JoinThreads {
private:
    std::vector<std::thread>& threads_;
public:
    explicit JoinThreads(std::vector<std::thread>& threads): threads_(threads) {}
    ~JoinThreads() {
        for (auto & thread : threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
};

#endif