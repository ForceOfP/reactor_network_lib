#pragma once

#ifndef Tcp_Server_Marco
#define Tcp_Server_Marco

#include "Callbacks.hpp"
#include "TcpConnection.hpp"
#include <map>
#include <memory>

class EventLoop;
class Acceptor;
class EventLoopThreadPool;

class TcpServer {
public:
    TcpServer(EventLoop* loop, const InetAddress& listen_addr);
    ~TcpServer() = default;

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator= (const TcpServer&) = delete;

    void start();
    void set_thread_num(int num);

    void set_connection_callback(const ConnectionCallback& cb) {
        connect_callback_ = cb;
    }

    void set_message_callback(const MessageCallback& cb) {
        message_callback_ = cb;
    }

    void set_close_callback(const CloseCallback& cb) {
        close_callback_ = cb;
    }

    void set_write_callback(const WriteCompleteCallback& cb) {
        write_complete_callback_ = cb;
    }
private:
    void new_connection(int sock_fd, const InetAddress& peer_addr);

    void remove_connection(const TcpConnectionPtr& conn);
    
    void remove_connection_in_loop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::map<std::string, TcpConnectionPtr>;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;
    std::unique_ptr<EventLoopThreadPool> pool_;
    ConnectionCallback connect_callback_;
    MessageCallback message_callback_;
    CloseCallback close_callback_;
    WriteCompleteCallback write_complete_callback_;
    bool started_;
    int next_conn_id_;
    ConnectionMap connections_;

    EventLoop* loop_;
};

#endif