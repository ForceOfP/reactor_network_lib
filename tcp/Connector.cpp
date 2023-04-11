#include "Connector.hpp"

#include "../event_loop/Channel.hpp"
#include "../event_loop/EventLoop.hpp"
#include "SocketsOps.hpp"
#include <cassert>
#include <cerrno>
#include <cstring>
#include <memory>
#include <fmt/format.h>

const int Connector::k_max_retry_delay_ms;
const int Connector::k_init_retry_delay_ms;

std::string to_string(Connector::State s) {
    switch (s) {
        case Connector::State::Disconnected: return "Disconnected";
        case Connector::State::Connected: return "Connected";
        case Connector::State::Connecting: return "Connecting";
    }
}

Connector::Connector(EventLoop* loop, const InetAddress& server_addr):
    loop_(loop),
    server_addr_(server_addr),
    connect_(false),
    state_(State::Disconnected),
    retry_delay_ms_(k_init_retry_delay_ms) {
    LOG_DEBUG << "ctor [Connector]";
}

Connector::~Connector() {
    LOG_DEBUG << "dtor [Connector]";
    loop_->cancel(timer_id_);
    assert(!channel_);
}

void Connector::start() {
    connect_ = true;
    loop_->run_in_loop([this] {start_in_loop();}); // FIXME: unsafe
}

void Connector::start_in_loop() {
    loop_->assert_in_loop_thread();
    assert(state_ == State::Disconnected);
    if (connect_) {
        connect();
    } else {
        LOG_DEBUG << "do not connect";
    }
}

void Connector::connect() {
    int sock_fd = sockets::createNonblockingOrDie();
    int ret = sockets::connect(sock_fd, server_addr_.get_sock_addr_inet());
    int saved_errno = (ret == 0) ? 0 : errno;
    switch (saved_errno) {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            connecting(sock_fd);
            break;

        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry(sock_fd);
            break;

        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            LOG_SYSERR << fmt::format("connect error in Connector::startInLoop {}",saved_errno);
            sockets::close(sock_fd);
            break;

        default:
            LOG_SYSERR << fmt::format("Unexpected error in Connector::startInLoop {}", saved_errno);
            sockets::close(sock_fd);
            // connectErrorCallback_();
            break;
    }
}

void Connector::restart() {
    loop_->assert_in_loop_thread();
    set_state(State::Disconnected);
    retry_delay_ms_ = k_init_retry_delay_ms;
    connect_ = true;
    start_in_loop();
}

void Connector::stop() {
    connect_ = false;
    loop_->cancel(timer_id_);
}

void Connector::connecting(int sock_fd) {
    set_state(State::Connecting);
    assert(!channel_);
    channel_ = std::make_unique<Channel>(loop_, sock_fd);
    channel_->set_write_callback([this] {handle_write();});
    channel_->set_error_callback([this] {handle_error();});

    channel_->enable_writing();
}

int Connector::remove_and_reset_channel() {
    channel_->disable_all();
    loop_->remove_channel(channel_.get());
    int sock_fd = channel_->get_fd();
    loop_->queue_in_loop([this] {reset_channel();});
    return sock_fd;
}

void Connector::reset_channel() {
    channel_.reset();
}

void Connector::handle_write() {
    LOG_TRACE << "Connector::handle_write() " + to_string(state_);

    if (state_ == State::Connecting) {
        int sock_fd = remove_and_reset_channel();
        int err = sockets::getSocketError(sock_fd);
        if (err) {
            LOG_WARN << fmt::format("Connector::handle_write - so_error = {} {}", err, strerror(err));
            retry(sock_fd);
        } else if (sockets::isSelfConnect(sock_fd)) {
            LOG_WARN << "Connector::handle_write - Self connect";
            retry(sock_fd);
        } else {    
            set_state(State::Connected);
            if (connect_) {
                new_connection_callback_(sock_fd);
            } else {
                sockets::close(sock_fd);
            }
        }
    } else {
        assert(state_ == State::Disconnected);
    }
}

void Connector::handle_error() {
    LOG_ERROR << "Connector::handle_error";
    assert(state_ == State::Connecting);

    int sock_fd = remove_and_reset_channel();
    int err = sockets::getSocketError(sock_fd);
    LOG_TRACE << fmt::format("so_error = {} {}", err, strerror(err));
    retry(sock_fd);
}

void Connector::retry(int sock_fd) {
    sockets::close(sock_fd);
    set_state(State::Disconnected);
    if (connect_) {
        LOG_INFO << fmt::format("Connector::retry - Retry connecting to {} in {} milliseconds"
                        , server_addr_.to_host_port(), retry_delay_ms_);
        timer_id_ = loop_->run_after(retry_delay_ms_ / 1000.0, [this] {start_in_loop();});
        retry_delay_ms_ = std::min(retry_delay_ms_, k_max_retry_delay_ms);
    } else {
        LOG_DEBUG << "do not connect";
    }
}