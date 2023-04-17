#pragma once

#ifndef Acceptor_Marco
#define Acceptor_Marco

#include <functional>

#include "../event_loop/Channel.hpp"
#include "Socket.hpp"

class EventLoop;
class InetAddress;

class Acceptor {
public:
    using NewConnectionCallback = std::function<void(int sock_fd, const InetAddress&)>;

    Acceptor(EventLoop* loop, const InetAddress& listen_addr);

    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;

    void set_new_connection_callback(const NewConnectionCallback& cb) {
        new_connection_callback_ = cb;
    }

    [[nodiscard]] bool listenning() const {return listenning_;}
    void listen();
private:
    [[maybe_unused]] void handle_read_once();
    [[maybe_unused]] void handle_read_all();

    EventLoop* loop_;
    Socket accept_socket_;
    Channel accept_channel_;
    NewConnectionCallback new_connection_callback_;
    bool listenning_;
    int idle_fd_;
};

#endif