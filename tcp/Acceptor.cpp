#include "Acceptor.hpp"
#include "Socket.hpp"
#include "SocketsOps.hpp"
#include "InetAddress.hpp"
#include "../event_loop/EventLoop.hpp"

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listen_addr):
    loop_(loop),
    accept_socket_(sockets::createNonblockingOrDie()),
    accept_channel_(loop, accept_socket_.get_fd()),
    listenning_(false) {
    accept_socket_.set_reuse_addr(true);
    accept_socket_.bind_address(listen_addr);
    accept_channel_.set_read_callback(std::bind(&Acceptor::handle_read, this));
}

void Acceptor::listen() {
    loop_->assert_in_loop_thread();
    listenning_ = true;
    accept_socket_.listen();
    accept_channel_.enable_reading();
}

void Acceptor::handle_read() {
    loop_->assert_in_loop_thread();
    InetAddress peer_addr(0);
    // FixMe: loop until no more
    int conn_fd = accept_socket_.accept(&peer_addr);
    if (conn_fd >= 0) {
        if (new_connection_callback_) {
            new_connection_callback_(conn_fd, peer_addr);
        } else {
            sockets::close(conn_fd);
        }
    }
}