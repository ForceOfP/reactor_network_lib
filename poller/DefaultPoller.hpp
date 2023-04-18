#include "Epoller.hpp"
#include "Poller.hpp"
#include "PollerInterface.hpp"

#include <cstdlib>

PollerInterface* newDefaultPoller(EventLoop* loop) {
    if (::getenv("I_USE_POLL")) {
        return new Poller(loop);
    } else {
        return new Epoller(loop);
    }
}