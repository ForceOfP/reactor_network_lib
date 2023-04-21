#pragma once
#ifndef Join_Threads_Marco
#define Join_Threads_Marco

#include <thread>
#include <vector>

class join_threads {
private:
    std::vector<std::thread>& threads_;
public:
    explicit join_threads(std::vector<std::thread>& threads): threads_(threads) {}
    ~join_threads() {
        for (auto & thread : threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
};

#endif