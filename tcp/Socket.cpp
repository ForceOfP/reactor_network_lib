#include "Socket.hpp"
#include "SocketsOps.hpp"
#include "InetAddress.hpp"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>  // bzero
#include <sys/socket.h>

Socket::~Socket() {
    sockets::close(sock_fd_);
}

void Socket::bind_address(const InetAddress &address) {
    sockets::bindOrDie(sock_fd_, address.get_sock_addr_inet());
}

void Socket::listen() {
    sockets::listenOrDie(sock_fd_);
}

int Socket::accept(InetAddress *peer_addr) {
    struct sockaddr_in addr;
    bzero(&addr, sizeof addr);
    int conn_fd = sockets::accept(sock_fd_, &addr);
    if (conn_fd > 0) {
        peer_addr->set_sock_addr_inet(addr);
    }
    return conn_fd;
}

void Socket::set_reuse_addr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
    // FIXME CHECK
}

void Socket::shutdown_write() {
    sockets::shutdownWrite(sock_fd_);
}

void Socket::set_tcp_no_delay(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sock_fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
    // FIXME CHECK
}