#include <gtest/gtest.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "../../../event_loop/EventLoopThread.hpp"
#include "../../../event_loop/EventLoop.hpp"
#include "../../../event_loop/EventLoopThreadPool.hpp"
#include "../../../tcp/Connector.hpp"
#include "../../../tcp/InetAddress.hpp"
#include "../../../tcp/Socket.hpp"
#include "../../../tcp/SocketsOps.hpp"
#include "../../../tcp/TcpClient.hpp"
#include "../../../tcp/TcpServer.hpp"

TEST(TcpServer, start) {
    std::thread t([]() {
        EventLoop loop2;
        InetAddress serverAddr("localhost", 9981);
        TcpServer server(&loop2, serverAddr);

        auto on_connect = [&loop2](const TcpConnectionPtr&) {
            ASSERT_TRUE(true);
            loop2.quit();
        };
        server.set_connection_callback(on_connect);
        server.start();
        loop2.loop();
    });
    t.detach();
    InetAddress serverAddr("localhost", 9981);
    EventLoop loop;
    TcpClient client(&loop, serverAddr);
    auto client_connection = [&loop](const TcpConnectionPtr& p) {
        loop.quit();
    };
    client.set_connection_callback(client_connection);
    client.enable_retry();
    client.connect();
    loop.loop();
}