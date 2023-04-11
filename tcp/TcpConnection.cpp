#include "TcpConnection.hpp"
#include "Callbacks.hpp"
#include "Socket.hpp"
#include "../event_loop/Channel.hpp"
#include "../event_loop/EventLoop.hpp"
#include <cassert>
#include <cstddef>
#include <cstring>
#include <functional>
#include <memory>
#include <string_view>
#include <fmt/format.h>
#include <sys/types.h>
#include <unistd.h>
#include "../logger/logger.hpp"
#include "SocketsOps.hpp"
#include <cerrno>

std::string to_string(TcpConnection::State s) {
    switch (s) {
        case TcpConnection::State::Connected:
            return "Connected";
        case TcpConnection::State::Connecting:
            return "Connecting";
        case TcpConnection::State::Disconnected:
            return "Disconnected";
        case TcpConnection::State::Disconnecting:
            return "Disconnecting";
    }
}

TcpConnection::TcpConnection(EventLoop* loop, 
                            std::string_view name,
                            int sock_fd,
                            const InetAddress& local_addr,
                            const InetAddress& peer_addr):
    loop_(CHECK_NOTNULL(loop)),
    name_(name),
    state_(State::Connecting),
    socket_(std::make_unique<Socket>(sock_fd)),
    channel_(std::make_unique<Channel>(loop, sock_fd)),
    local_addr_(local_addr),
    peer_addr_(peer_addr) {
    LOG_DEBUG << fmt::format("TcpConnection::ctor[{}] at fd = {}", name_, sock_fd);

    channel_->set_read_callback([this](auto && PH1) { handle_read(std::forward<decltype(PH1)>(PH1)); });
    channel_->set_write_callback([this] { handle_write(); });
    channel_->set_error_callback([this] { handle_error(); });
    channel_->set_close_callback([this] { handle_close(); });
}

TcpConnection::~TcpConnection() {
    LOG_DEBUG << fmt::format("TcpConnection::dtor[{}] at fd = {}", name_, channel_->get_fd());
}

void TcpConnection::send(std::string_view message) {
    if (state_ == State::Connected) {
        if (loop_->is_in_loop_thread()) {
            send_in_loop(message);
        } else {
            loop_->run_in_loop([this, message]{send_in_loop(message);});
        }
    }
}

void TcpConnection::send_in_loop(std::string_view message) {
    loop_->assert_in_loop_thread();
    ssize_t n_wrote = 0;
    // if no thing in output queue, try writing directly
    if (!channel_->is_writing() && output_buffer_.readable_bytes() == 0) {
        n_wrote = ::write(channel_->get_fd(), message.data(), message.size());
        if (n_wrote >= 0) {
            if (static_cast<size_t>(n_wrote) < message.size()) {
                LOG_TRACE << "I am going to write more data";
            } else if (write_complete_callback_) {
                loop_->queue_in_loop([this] {write_complete_callback_(shared_from_this());});
            }
        } else {
            n_wrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_SYSERR << "TcpConnection::send_in_loop";
            }
        }
    }

    assert(n_wrote >= 0);
    if (static_cast<size_t>(n_wrote) < message.size()) {
        output_buffer_.append(message.data() + n_wrote, message.size() - n_wrote);
        if (!channel_->is_writing()) {
            channel_->enable_writing();
        }
    }
}

void TcpConnection::shutdown() {
    // FIXME: use compare and swap
    if (state_ == State::Connected) {
        set_state(State::Disconnecting);
        // FIXME: shared_from_this()?
        loop_->run_in_loop([this] {shutdown_in_loop();});
    }
}

void TcpConnection::shutdown_in_loop() {
    loop_->assert_in_loop_thread();
    if (!channel_->is_writing()) {
        socket_->shutdown_write();
    }
}

void TcpConnection::connect_established() {
    loop_->assert_in_loop_thread();
    assert(state_ == State::Connecting);
    set_state(State::Connected);
    channel_->enable_reading();
    connect_callback_(shared_from_this());
}

void TcpConnection::connect_destoryed() {
    loop_->assert_in_loop_thread();
    assert(state_ == State::Connected || state_ == State::Disconnecting);
    set_state(State::Disconnected);
    channel_->disable_all();
    connect_callback_(shared_from_this());

    loop_->remove_channel(channel_.get());
}

void TcpConnection::handle_read(TimeStamp receive_time) {
    int saved_errno = 0;
    ssize_t n = input_buffer_.read_fd(channel_->get_fd(), &saved_errno);
    if (n > 0) {
        message_callback_(shared_from_this(), &input_buffer_, receive_time);
    } else if (n == 0) {
        handle_close();
    } else {
        errno = saved_errno;
        LOG_SYSERR << "Tcp::handle_read";
        handle_error();
    }
}

void TcpConnection::handle_write() {
    loop_->assert_in_loop_thread();
    if (channel_->is_writing()) {
        ssize_t n = ::write(channel_->get_fd(), output_buffer_.peek(), output_buffer_.readable_bytes());
        if (n > 0) {
            output_buffer_.retrieve(n);
            if (output_buffer_.readable_bytes() == 0) {
                channel_->disable_writing();
                if (write_complete_callback_) {
                    loop_->queue_in_loop([this] {write_complete_callback_(shared_from_this());});
                }
                if (state_ == State::Disconnecting) {
                    shutdown_in_loop();
                }
            } else {
                LOG_TRACE << "I am going to write more data";
            }
        } else {
            LOG_SYSERR << "TcpConnection::handle_write";
        }
    } else {
        LOG_TRACE << "Connection is down, no more writing";
    }
}

void TcpConnection::handle_close() {
    loop_->assert_in_loop_thread();

    LOG_TRACE << fmt::format("TcpConnection::handle_close state = {}", to_string(state_));
    assert(state_ == State::Connected || state_ == State::Disconnecting);
    // we don't close fd, leave it to dtor, so we can find leaks easily.
    channel_->disable_all();
    // must be the last line
    close_callback_(shared_from_this());
}

void TcpConnection::handle_error() {
    int err = sockets::getSocketError(channel_->get_fd());
    LOG_ERROR << fmt::format("TcpConnection::handle_error [{}] - SO_ERROR = {}", err, strerror(err));
} 

void TcpConnection::set_tcp_no_delay(bool on) {
    socket_->set_tcp_no_delay(on);
}