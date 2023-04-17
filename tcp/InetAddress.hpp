#pragma once

#ifndef Inet_Address_Marco
#define Inet_Address_Marco

#include "Socket.hpp"
#include <cstdint>
#include <netinet/in.h>
#include <string>
#include <string_view>

class InetAddress {
public:
    explicit InetAddress(uint16_t port = 0);

    InetAddress(std::string_view ip, uint16_t port);
    InetAddress(const struct sockaddr_in& addr): addr_(addr) {}

    [[nodiscard]] std::string to_host_port() const;

    [[nodiscard]] const struct sockaddr_in& get_sock_addr_inet() const {return addr_;}
    void set_sock_addr_inet(const struct sockaddr_in& addr) {addr_ = addr;}
private:
    struct sockaddr_in addr_;
};

#endif