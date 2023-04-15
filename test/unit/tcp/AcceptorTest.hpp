#include <gtest/gtest.h>
#include "../../../tcp/Acceptor.hpp"
#include "../../../tcp/InetAddress.hpp"
#include "../../../tcp/SocketsOps.hpp"
#include "../../../event_loop/EventLoop.hpp"

TEST(Acceptor, listening) {
    EventLoop loop;
    InetAddress addr(9981);
    Acceptor acc(&loop, addr);
    acc.listen();
    ASSERT_TRUE(acc.listenning());
}

void new_connection(int sock_fd, const InetAddress& peer_addr) {    
    sockets::close(sock_fd);
    ASSERT_TRUE(true);
}

TEST(Acceptor, listen) {
    EventLoop loop;
    InetAddress addr(9981);
    Acceptor acc(&loop, addr);
    acc.set_new_connection_callback(new_connection);
    acc.listen();

    auto stop = [&loop]() {
        loop.quit();
    };

    loop.run_after(0.05, stop);
    loop.loop();
}

