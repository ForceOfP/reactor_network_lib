#include <gtest/gtest.h>
#include <sys/socket.h>
#include "../../../tcp/Connector.hpp"
#include "../../../tcp/InetAddress.hpp"
#include "../../../tcp/SocketsOps.hpp"
#include "../../../event_loop/EventLoop.hpp"

TEST(Connector, start) {
    EventLoop loop;
    InetAddress addr("127.0.0.1", 9981);

    Connector connector(&loop, addr);
    auto connect_callback = [&loop](int sock_fd) {
        loop.quit();
        ASSERT_TRUE(true);
    };
    connector.set_new_connection_callback(connect_callback);
    connector.start();

    int sock_fd = sockets::createNonblockingOrDie();
    sockets::bindOrDie(sock_fd, addr.get_sock_addr_inet());
    sockets::listenOrDie(sock_fd);

    loop.loop();
}

TEST(Connector, stop) {
    // @TODO: Add test case.
}