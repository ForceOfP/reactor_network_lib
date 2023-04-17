#include "TcpClient.hpp"
#include "Connector.hpp"
#include "../logger/logger.hpp"
#include "../event_loop/EventLoop.hpp"
#include "SocketsOps.hpp"
#include <cassert>
#include <cstdio>
#include <functional>
#include <fmt/format.h>
#include <memory>
#include <mutex>

#include <sys/poll.h>

namespace detail
{

void remove_connection(EventLoop* loop, const TcpConnectionPtr& conn) {
  loop->queue_in_loop([conn] { conn->connect_destoryed(); });
}

void remove_connector(const ConnectorPtr& connector) {
  //connector->
}

}


TcpClient::TcpClient(EventLoop* loop, const InetAddress& server_addr):
    loop_(CHECK_NOTNULL(loop)),
    connector_(std::make_shared<Connector>(loop, server_addr)),
    retry_(false),
    connect_(true),
    next_conn_id_(1) {
    connector_->set_new_connection_callback
        (std::bind(&TcpClient::new_connection, this, std::placeholders::_1));
    // FIXME setConnectFailedCallback
    LOG_INFO << "TcpClient::TcpClient - connector";
}

TcpClient::~TcpClient() {
    LOG_INFO << "TcpClient::~TcpClient - connector";
    TcpConnectionPtr conn = nullptr;
    {
        std::lock_guard<std::mutex> lock(mut_);
        conn = connection_;
    }
    if (conn) {
        // FIXME: not 100% safe, if we are in different thread
        CloseCallback cb = [this](auto && PH1) { return detail::remove_connection(loop_, std::forward<decltype(PH1)>(PH1)); };
        loop_->run_in_loop([conn, cb] { conn->set_close_callback(cb); });
    } else {
        connector_->stop();
        // FIXME: HACK
        loop_->run_after(1, [this] {detail::remove_connector(connector_);});
    }
}

void TcpClient::connect() {
    // FIXME: check state
    LOG_INFO << fmt::format("TcpClient::connect - connecting to {}", connector_->get_server_address().to_host_port());
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect() {
    connect_ = false;

    {
        std::lock_guard<std::mutex> lock(mut_);
        if (connection_) {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop() {
    connect_ = false;
    connector_->stop();
}

void TcpClient::new_connection(int sock_fd) {
    loop_->assert_in_loop_thread();

    struct pollfd test_conn;
    test_conn.fd = sock_fd;
    test_conn.events = POLLIN;
    test_conn.revents = 0;
    if (::poll(&test_conn, 1, 0) < 0) {
        LOG_SYSERR << fmt::format("TcpClient::new_connection(): cannot connect to fd: {}", sock_fd);
        return;
    }

    InetAddress peer_addr(sockets::getPeerAddr(sock_fd));

    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peer_addr.to_host_port().data(), next_conn_id_);
    ++next_conn_id_;
    std::string conn_name = buf;

    InetAddress local_addr(sockets::getLocalAddr(sock_fd));
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop_, conn_name, sock_fd, local_addr, peer_addr);
    conn->set_connection_callback(connection_callback_);
    conn->set_message_callback(message_callback_);
    conn->set_write_callback(write_complete_callback_);
    conn->set_close_callback([this](auto && PH1) { remove_connection(std::forward<decltype(PH1)>(PH1)); });
    {
        std::lock_guard<std::mutex> lock(mut_);
        connection_ = conn;
    }
    conn->connect_established();
}

void TcpClient::remove_connection(const TcpConnectionPtr& conn) {
    loop_->assert_in_loop_thread();
    assert(loop_ == conn->get_loop());

    {
        std::lock_guard<std::mutex> lock(mut_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queue_in_loop([conn] { conn->connect_destoryed(); });
    if (retry_ && connect_) {
        LOG_INFO << fmt::format("TcpClient::connect - Reconnecting to {}", connector_->get_server_address().to_host_port());
        connector_->restart();
    }
}