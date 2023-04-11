#include "InetAddress.hpp"
#include "SocketsOps.hpp"
#include <cstdint>
#include <strings.h>
#include <sys/socket.h>

static const in_addr_t k_in_addr_any = INADDR_ANY;

static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in));

InetAddress::InetAddress(uint16_t port) {
    bzero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = sockets::hostToNetwork32(k_in_addr_any);
    addr_.sin_port = sockets::hostToNetwork16(port);
}

InetAddress::InetAddress(std::string_view ip, uint16_t port) {
    bzero(&addr_, sizeof addr_);
    sockets::fromHostPort(ip.data(), port, &addr_);
}

std::string InetAddress::to_host_port() const {
    char buf[32];
    sockets::toHostPort(buf, sizeof buf, addr_);
    return buf;
}
