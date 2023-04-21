#pragma once
#ifndef Handler_Event_Loop_Marco
#define Handler_Event_Loop_Marco

#include "../../event_loop/EventLoop.hpp"


class HandlerEventLoop: public EventLoop {
public:
    void loop() override;
    void set_looping_function(std::function<void()>);
private:
    std::function<void()> functor_;
};

#endif