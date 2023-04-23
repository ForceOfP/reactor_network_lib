#include "TcpServer.hpp"
#include "../event_loop/EventLoop.hpp"
#include "../event_loop/EventLoopThread.hpp"
#include "../event_loop/EventLoopThreadPool.hpp"
#include "../logger/logger.hpp"
#include "Acceptor.hpp"
#include "SocketsOps.hpp"
#include <cassert>
#include <cstdio>
#include <functional>
#include <memory>
#include <sys/types.h>
#include <type_traits>
#include <fmt/format.h>

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listen_addr):
    name_(listen_addr.to_host_port()),
    acceptor_(std::make_unique<Acceptor>(loop, listen_addr)),
    pool_(std::make_unique<EventLoopThreadPool>(loop)),
    started_(false),
    next_conn_id_(1),
    loop_(CHECK_NOTNULL(loop)) {
    acceptor_->set_new_connection_callback(
        std::bind(&TcpServer::new_connection, this, std::placeholders::_1, std::placeholders::_2));
}

void TcpServer::start() {
    if (!started_) {
        started_ = true;
        pool_->start();
    }

    if (!acceptor_->listenning()) {
        loop_->run_in_loop([capture0 = acceptor_.get()] { capture0->listen(); });
    }
}

void TcpServer::new_connection(int sock_fd, const InetAddress& peer_addr) {
    loop_->assert_in_loop_thread();
    char buf[32];
    snprintf(buf, sizeof buf, "#%d", next_conn_id_);
    next_conn_id_++;
    std::string conn_name = name_ + buf;

    LOG_INFO << fmt::format("TcpServer::new_connection [{}] - new connection [{}] from {}"
                    , name_, conn_name, peer_addr.to_host_port());
    InetAddress local_addr(sockets::getLocalAddr(sock_fd));
    EventLoop* io_loop = pool_->get_next_loop();
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(io_loop, conn_name, sock_fd, local_addr, peer_addr);

    connections_[conn_name] = conn;    
    conn->set_connection_callback(connect_callback_);
    conn->set_message_callback(message_callback_);
    conn->set_close_callback([this](auto && PH1) { remove_connection(std::forward<decltype(PH1)>(PH1)); });
    conn->set_write_callback(write_complete_callback_);

    conn->set_owner_loop(io_loop);

    conn->connect_established();
}

void TcpServer::remove_connection(const TcpConnectionPtr& conn) {
    loop_->run_in_loop([this, conn] {remove_connection_in_loop(conn);});
}

void TcpServer::remove_connection_in_loop(const TcpConnectionPtr& conn) {
    loop_->assert_in_loop_thread();
    LOG_INFO << fmt::format("TcpServer::remove_connection_in_loop() [{}] - connection {}", name_, conn->get_name());
    size_t n = connections_.erase(conn->get_name().data());
    assert(n == 1);
    (void)n;
    EventLoop* io_loop = conn->get_loop();
    io_loop->queue_in_loop([conn] { conn->connect_destoryed(); });
}

void TcpServer::set_thread_num(int num) {
    pool_->set_thread_num(num);
}

