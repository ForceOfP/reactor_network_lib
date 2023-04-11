#pragma once

#ifndef Tcp_Client_Marco
#define Tcp_Client_Marco

#include <memory>
#include <mutex>
#include "Callbacks.hpp"
#include "InetAddress.hpp"
#include "TcpConnection.hpp"

class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;

class TcpClient {
public:
    TcpClient(EventLoop* loop, const InetAddress& server_addr);
    ~TcpClient();

    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    void connect();
    void disconnect();
    void stop();

    TcpConnectionPtr get_connection() const {
        std::lock_guard<std::mutex> lock(mut_);
        return connection_;
    }

    bool retry() const;
    void enable_retry() { retry_ = true; }

    // not thread safe
    void set_connection_callback(const ConnectionCallback& cb) {
        connection_callback_ = cb;
    }
    // not thread safe
    void set_message_callback(const MessageCallback& cb) {
        message_callback_ = cb;
    }

    // not thread safe
    void set_write_complete_callback(const WriteCompleteCallback& cb) {
        write_complete_callback_ = cb;
    } 
private:
    void new_connection(int sock_fd);
    void remove_connection(const TcpConnectionPtr& conn);

    EventLoop* loop_;
    ConnectorPtr connector_;
    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    WriteCompleteCallback write_complete_callback_;
    bool retry_;
    bool connect_;
    int next_conn_id_;
    mutable std::mutex mut_;
    TcpConnectionPtr connection_;
};

#endif