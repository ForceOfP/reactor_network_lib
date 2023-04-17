#pragma once

#ifndef Socket_Macro
#define Socket_Macro

class InetAddress;

class Socket {
public:
    explicit Socket(int fd): sock_fd_(fd) {}
    ~Socket();

    Socket(const Socket&) = delete;
    Socket& operator= (const Socket&) = delete;

    int get_fd() {return sock_fd_;}

    void bind_address(const InetAddress& address);
    void listen();

    int accept(InetAddress* peer_addr);

    void set_reuse_addr(bool on);
    // @TODO: add reuse port
    void set_tcp_no_delay(bool on);

    void shutdown_write();
private:
    const int sock_fd_;
};

#endif