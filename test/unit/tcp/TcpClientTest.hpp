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

TEST(TcpClient, connect) {
    InetAddress serverAddr("localhost", 9981);
    EventLoop loop;
    TcpClient client(&loop, serverAddr);

    auto client_connection = [&loop](const TcpConnectionPtr& p) {
        ASSERT_TRUE(true);
        loop.quit();
    };

    client.set_connection_callback(client_connection);
    //client.set_message_callback(on_message);
    client.enable_retry();
    client.connect();

    std::thread t([]() {
        EventLoop loop2;
        InetAddress serverAddr("localhost", 9981);
        TcpServer server(&loop2, serverAddr);
        server.start();
        loop2.loop();
    });
    t.detach();
    loop.loop();
}