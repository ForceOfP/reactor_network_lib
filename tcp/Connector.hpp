#pragma once

#ifndef Connector_Marco
#define Connector_Marco

#include "InetAddress.hpp"
#include "../time_utils/TimerId.hpp"
#include <functional>
#include <memory>

class Channel;
class EventLoop;

class Connector {
public:
    using NewConnectionCallback = std::function<void(int sock_fd)>;

    Connector(EventLoop* loop, const InetAddress& server_addr);
    ~Connector();

    Connector(const Connector&) = delete;
    Connector& operator=(const Connector&) = delete;

    void set_new_connection_callback(const NewConnectionCallback& cb) {
        new_connection_callback_ = cb;
    }

    InetAddress get_server_address() {return server_addr_;}   

    void start(); 
    void restart(); // must be called in loop thread
    void stop();

    enum class State {Disconnected, Connecting, Connected};
private:
    static const int k_max_retry_delay_ms = 30 * 1000;
    static const int k_init_retry_delay_ms = 500;

    void set_state(State s) {state_ = s;}
    void start_in_loop();
    void connect();
    void connecting(int sock_fd);
    void handle_write();
    void handle_error();
    void retry(int sock_fd);
    int remove_and_reset_channel();
    void reset_channel();

    EventLoop* loop_;
    InetAddress server_addr_;
    bool connect_;
    State state_;
    std::unique_ptr<Channel> channel_;
    NewConnectionCallback new_connection_callback_;
    int retry_delay_ms_;
    TimerId timer_id_;
};

using ConnectorPtr = std::shared_ptr<Connector>;

#endif