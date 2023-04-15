#include <gtest/gtest.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "../../../event_loop/EventLoop.hpp"
#include "../../../tcp/Connector.hpp"
#include "../../../tcp/InetAddress.hpp"
#include "../../../tcp/Socket.hpp"
#include "../../../tcp/SocketsOps.hpp"

TEST(Socket, accept) {
    int sock_fd = sockets::createNonblockingOrDie();
    int sock_fd2 = sockets::createNonblockingOrDie();
    Socket socket(sock_fd);
    InetAddress addr("127.0.0.1", 9981);
    EventLoop loop;

    auto connect = [&sock_fd2, &addr](){
        for (;;) {
            auto ret = sockets::connect(sock_fd2, addr.get_sock_addr_inet());
            if (ret != -1) {
                ::close(sock_fd2);
                break;
            }
        }
    };
    std::thread t{connect};

    socket.accept(&addr);
    t.join();
}