#pragma once

#ifndef Channel_Marco
#define Channel_Marco

#include <functional>
#include "../time_utils/TimeDataStructure.hpp"

class EventLoop;
class Channel {
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(TimeStamp)>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    Channel(const Channel& src) = delete;
    Channel& operator=(const Channel& rhs) = delete;

    void handle_event(TimeStamp receive_time);
    void set_read_callback(const ReadEventCallback& cb) {
        read_callback_ = cb;
    }
    void set_write_callback(const EventCallback& cb) {
        write_callback_ = cb;
    }
    void set_error_callback(const EventCallback& cb) {
        error_callback_ = cb;
    }
    void set_close_callback(const EventCallback& cb) {
        close_callback_ = cb;
    }

    [[nodiscard]] int get_fd() const {return fd_;}
    [[nodiscard]] int get_events() const {return events_;}
    void set_revents(int r) {revents_ = r;}
    [[nodiscard]] bool is_none_event() const {return events_ == k_none_event_;}

    void enable_reading() {events_ |= k_read_event_; update();}
    void enable_writing() {events_ |= k_write_event_; update();}
    void disable_writing() {events_ &= ~k_write_event_; update();}
    void disable_all() {events_ = k_none_event_; update();}
    [[nodiscard]] bool is_writing() const {return events_ & k_write_event_;}

    // for Poller
    int get_index() {return index_;}
    void set_index(int i) {index_ = i;}

    EventLoop* owner_loop() {return loop_;}
private:
    void update();

    static const int k_none_event_;
    static const int k_read_event_;
    static const int k_write_event_;

    EventLoop* loop_;
    const int fd_;
    int events_;
    int revents_;
    int index_;

    ReadEventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback error_callback_;
    EventCallback close_callback_;

    bool event_handling_;
};

#endif