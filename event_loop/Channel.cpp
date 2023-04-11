#include "Channel.hpp"
#include "EventLoop.hpp"
#include <cassert>
#include <sys/poll.h>

const int Channel::k_none_event_ = 0;
const int Channel::k_read_event_ = POLLIN | POLLPRI;
const int Channel::k_write_event_ = POLLOUT;

Channel::Channel(EventLoop* loop, int fd_arg): 
    loop_(loop), 
    fd_(fd_arg), 
    events_(0), 
    revents_(0), 
    index_(-1),
    event_handling_(false) {

}

Channel::~Channel() {
    assert(!event_handling_);
}

void Channel::update() {
    loop_->update_channel(this);
}

void Channel::handle_event(TimeStamp receive_time) {
    event_handling_ = true;
    if (revents_ & POLLNVAL) {
        LOG_WARN << "Channel::handle_event() POLLNVAL";
    }

    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
        LOG_WARN << "Channel::handle_event() POLLHUP";
        if (close_callback_) close_callback_();
    }
    
    if (revents_ & (POLLERR | POLLNVAL)) {
        if (error_callback_) error_callback_();
    }

    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if (read_callback_) read_callback_(receive_time);
    }

    if (revents_ & POLLOUT) {
        if (write_callback_) write_callback_();
    }
    event_handling_ = false;
}