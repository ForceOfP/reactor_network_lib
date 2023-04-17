#include "Acceptor.hpp"
#include "../event_loop/EventLoop.hpp"
#include "InetAddress.hpp"
#include "Socket.hpp"
#include "SocketsOps.hpp"

#include <asm-generic/errno-base.h>
#include <cassert>
#include <cerrno>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listen_addr)
    : loop_(loop), accept_socket_(sockets::createNonblockingOrDie()),
      accept_channel_(loop, accept_socket_.get_fd()), listenning_(false),
      idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
    assert(idle_fd_ >= 0);
    accept_socket_.set_reuse_addr(true);
    accept_socket_.bind_address(listen_addr);
    accept_channel_.set_read_callback(
        std::bind(&Acceptor::handle_read_once, this));
}

void Acceptor::listen() {
    loop_->assert_in_loop_thread();
    listenning_ = true;
    accept_socket_.listen();
    accept_channel_.enable_reading();
}

void Acceptor::handle_read_once() {
    loop_->assert_in_loop_thread();
    InetAddress peer_addr;
    int conn_fd = accept_socket_.accept(&peer_addr);
    if (conn_fd >= 0) {
        struct pollfd test_conn;
        test_conn.fd = conn_fd;
        test_conn.events = POLLOUT;
        test_conn.revents = 0;
        if (::poll(&test_conn, 1, 0) < 0) {
            sockets::close(conn_fd);
        } else {
            if (new_connection_callback_) {
                new_connection_callback_(conn_fd, peer_addr);
            } else {
                sockets::close(conn_fd);
            }
        }

    } else {
        LOG_SYSERR << "in Acceptor::handle_read";
        // Read the section named "The special problem of
        // accept()ing when you can't" in libev's doc.
        // By Marc Lehmann, author of libev.
        if (errno == EMFILE) {
            ::close(idle_fd_);
            idle_fd_ = ::accept(accept_socket_.get_fd(), nullptr, nullptr);
            ::close(idle_fd_);
            idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}

void Acceptor::handle_read_all() {
    loop_->assert_in_loop_thread();
    InetAddress peer_addr;
    while (int conn_fd = accept_socket_.accept(&peer_addr)) {
        if (conn_fd >= 0) {
            struct pollfd test_conn;
            test_conn.fd = conn_fd;
            test_conn.events = POLLOUT;
            test_conn.revents = 0;
            if (::poll(&test_conn, 1, 0) < 0) {
                sockets::close(conn_fd);
            } else {
                if (new_connection_callback_) {
                    new_connection_callback_(conn_fd, peer_addr);
                } else {
                    sockets::close(conn_fd);
                }
            }
        } else {
            // Read the section named "The special problem of
            // accept()ing when you can't" in libev's doc.
            // By Marc Lehmann, author of libev.
            if (errno == EMFILE) {
                ::close(idle_fd_);
                idle_fd_ = ::accept(accept_socket_.get_fd(), nullptr, nullptr);
                ::close(idle_fd_);
                idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
            } else {
                break;
            }
        }
    }
}