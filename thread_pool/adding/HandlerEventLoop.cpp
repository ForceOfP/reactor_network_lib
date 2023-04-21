#include "HandlerEventLoop.hpp"
#include <cassert>
#include <utility>

void HandlerEventLoop::loop() {
    assert(!looping_);
    assert_in_loop_thread();
    looping_ = true;
    quit_ = false;  // FIXME: what if someone calls quit() before loop() ?

    LOG_TRACE << "EventLoop start looping";

    while(!quit_) {
        functor_();
    }

    LOG_TRACE << "EventLoop stop looping";
    looping_ = false;
}

void HandlerEventLoop::set_looping_function(std::function<void ()> f) {
    functor_ = std::move(f);
} 