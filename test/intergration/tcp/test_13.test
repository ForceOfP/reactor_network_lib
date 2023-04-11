#include <bits/types/struct_itimerspec.h>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <string>
#include <string_view>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>
#include "buffer/Buffer.hpp"
#include "event_loop/Channel.hpp"
#include "event_loop/EventLoop.hpp"
//#include "event_loop/EventLoopThread.hpp"
#include "tcp/Callbacks.hpp"
#include "tcp/Socket.hpp"
#include "tcp/TcpServer.hpp"
//#include "thread_pool/ThreadPool.cpp"
#include <sys/timerfd.h>
#include "tcp/Acceptor.hpp"
#include "tcp/SocketsOps.hpp"
#include "tcp/InetAddress.hpp"
#include "tcp/TcpClient.hpp"
#include "init.hpp"

EventLoop* g_loop;
int g_flag = 0;

using namespace std;

string message = "Hello\n";

void on_connection(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        cout << "on_connection(): new connection [" << conn->get_name() << "] from " 
             << conn->get_peer_address().to_host_port() << endl;
        conn->send(message);
    } else {
        cout << "on_connection(): connection [" << conn->get_name() << "] is down\n";
    }
}

void on_message(const TcpConnectionPtr& conn, Buffer* buf, TimeStamp receive_time) {
    cout << "on_message(): received " 
         << buf->readable_bytes()
         << " bytes form connection [" 
         << conn->get_name() 
         << "] at "
         << receive_time.to_string()
         << endl;
    conn->send(buf->retrieve_as_string());
}

int main() {
    init_log();
    InetAddress serverAddr("localhost", 9981);
    EventLoop loop;
    TcpClient client(&loop, serverAddr);

    client.set_connection_callback(on_connection);
    client.set_message_callback(on_message);
    client.enable_retry();
    client.connect();
    
    loop.loop();
}