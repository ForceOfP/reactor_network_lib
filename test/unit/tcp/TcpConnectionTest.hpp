#include <gtest/gtest.h>
#include <memory>
#include <sys/socket.h>

#include "../../../tcp/TcpConnection.hpp"
#include "../../../event_loop/EventLoop.hpp"
#include "../../../event_loop/Channel.hpp"
#include "../../../tcp/InetAddress.hpp"
#include "../../../tcp/Socket.hpp"
#include "../../../tcp/SocketsOps.hpp"

TEST(TcpConnection, connect) {
    EventLoop loop;
    InetAddress local("127.0.0.1", 9981);
    InetAddress peer("127.0.0.1", 9982);
    int sock_fd = sockets::createNonblockingOrDie();
    sockets::bindOrDie(sock_fd, peer.get_sock_addr_inet());

    auto conn = std::make_shared<TcpConnection>(&loop, "1", sock_fd, local, peer);

    bool visited_connect = false;    
    auto connect_functor = [&visited_connect](const TcpConnectionPtr& p) {
        visited_connect = true;
    };
    conn->set_owner_loop(&loop);
    conn->set_connection_callback(connect_functor);
    conn->connect_established();

    visited_connect = false;    
    auto close_functor = [&visited_connect](const TcpConnectionPtr& p) {
        visited_connect = true;
    };
    conn->set_close_callback(close_functor);
    conn->connect_destoryed();

    ASSERT_TRUE(visited_connect);
}

TEST(TcpConnection, send) {
    // @TODO: add send test case
}

TEST(TcpConnection, shutdown) {
    // @TODO: add shutdown test case
}