#include "Buffer.hpp"
#include <cerrno>
#include <cstddef>
#include <sys/uio.h>

ssize_t Buffer::read_fd(int fd, int *savedErrno) {
    char extra_buff[65536];
    struct iovec vec[2];
    const size_t writable = writable_bytes();
    vec[0].iov_base = begin() + writer_index_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extra_buff;
    vec[1].iov_len = sizeof extra_buff;
    const ssize_t n = readv(fd, vec, 2);

    if (n < 0) {
        *savedErrno = errno;
    } else if (static_cast<size_t>(n) <= writable) {
        writer_index_ += n;
    } else {
        writer_index_ = buffer_.size();
        append(extra_buff, n - writable);
    }
    return n;
}