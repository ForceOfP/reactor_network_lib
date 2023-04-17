#pragma once

#include <atomic>
#ifndef Tcp_Connection_Marco
#define Tcp_Connection_Marco

#include "Callbacks.hpp"
#include "InetAddress.hpp"
#include <memory>
#include <string_view>

class Channel;
class EventLoop;
class Socket;

class TcpConnection: public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(EventLoop* loop, std::string_view name, int sock_fd, const InetAddress& local_addr, const InetAddress& peer_addr);
    ~TcpConnection();

    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    EventLoop* get_loop() const {return loop_;}

    std::string_view get_name() const {return name_;}
    const InetAddress& get_local_address() {return local_addr_;}
    const InetAddress& get_peer_address() {return peer_addr_;}
    bool connected() const {return state_ == State::Connected;}

    void set_connection_callback(const ConnectionCallback& cb) {connect_callback_ = cb;}
    void set_message_callback(const MessageCallback& cb) {message_callback_ = cb;}
    void set_close_callback(const CloseCallback& cb) {close_callback_ = cb;}
    void set_write_callback(const WriteCompleteCallback& cb) {write_complete_callback_ = cb;}

    void connect_established();
    void connect_destoryed();

    void send(std::string_view message);
    void shutdown();
    void set_tcp_no_delay(bool on);
    // @TODO: add tcp keep-alive

    enum class State {Connecting, Connected, Disconnected, Disconnecting};
private:

    void set_state(State s) {state_ = s;}
    void handle_read(TimeStamp);
    void handle_write();
    void handle_close();
    void handle_error();

    void send_in_loop(std::string_view message);
    void shutdown_in_loop();

    EventLoop* loop_;
    std::string name_;
    std::atomic<State> state_; 

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    InetAddress local_addr_;
    InetAddress peer_addr_;
    ConnectionCallback connect_callback_;
    MessageCallback message_callback_;
    CloseCallback close_callback_;
    WriteCompleteCallback write_complete_callback_;
    Buffer input_buffer_;
    Buffer output_buffer_;
};

#endif