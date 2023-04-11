#pragma once

#ifndef Buffer_Marco
#define Buffer_Marco

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <type_traits>
#include <vector>
#include <cassert>
#include <string>

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode

class Buffer {
public:
    static const size_t k_cheap_prepend = 8;
    static const size_t k_initial_size = 1024;
    
    Buffer(): 
        buffer_(k_cheap_prepend + k_initial_size),
        reader_index_(k_cheap_prepend),
        writer_index_(k_cheap_prepend) {
        assert(readable_bytes() == 0);
        assert(writable_bytes() == k_initial_size);
        assert(prependable_bytes() == k_cheap_prepend);
    }

    void swap(Buffer& rhs) {
        buffer_.swap(rhs.buffer_);
        std::swap(writer_index_, rhs.writer_index_);
        std::swap(reader_index_, rhs.reader_index_);
    }

    [[nodiscard]] size_t readable_bytes() const {
        return writer_index_ - reader_index_;
    }

    [[nodiscard]] size_t writable_bytes() const {
        return buffer_.size() - writer_index_;
    }

    [[nodiscard]] size_t prependable_bytes() const {
        return reader_index_;
    }

    [[nodiscard]] const char* peek() const {
        return begin() + reader_index_;
    }

    // retrieve returns void, to prevent
    // string str(retrieve(readableBytes()), readableBytes());
    // the evaluation of two functions are unspecified
    void retrieve(size_t len) {
        assert(len <= readable_bytes());
        reader_index_ += len;
    }

    void retrieve_until(const char* end) {
        assert(peek() <= end);
        assert(end <= begin_write());
        retrieve(end - peek());
    }

    void retrieve_all() {
        reader_index_ = k_cheap_prepend;
        writer_index_ = k_cheap_prepend;
    }

    std::string retrieve_as_string() {
        std::string str(peek(), readable_bytes());
        retrieve_all();
        return str;
    }

    void append(std::string_view str) {
        ensure_writeable_bytes(str.size());
        std::copy(str.data(), str.data() + str.size(), begin_write());
        has_written(str.size());
    }

    void append(const void* data, size_t len) {
        auto data_ = static_cast<const char*>(data);
        ensure_writeable_bytes(len);
        std::copy(data_, data_ + len, begin_write());
        has_written(len);
    } 

    void ensure_writeable_bytes(size_t len) {
        if (writable_bytes() < len) {
            make_space(len);
        }
        assert(writable_bytes() > len);
    }

    void has_written(size_t len) {
        writer_index_ += len;
    }

    char* begin_write() {
        return begin() + writer_index_;
    }
    
    [[nodiscard]] const char* begin_write() const {
        return begin() + writer_index_;
    }

    void prepend(const void* data, size_t len) {
        assert(len <= prependable_bytes());
        reader_index_ -= len;
        auto data_ = static_cast<const char*>(data);
        std::copy(data_, data_ + len, begin_write() + reader_index_);
    }

    void shrink(size_t reserve) {
        std::vector<char> buf(k_cheap_prepend + readable_bytes() + reserve);
        std::copy(peek(), peek() + readable_bytes(), buf.begin() + k_cheap_prepend);
        buf.swap(buffer_);
    }

    /// Read data directly into buffer.
    ///
    /// It may implement with readv(2)
    /// @return result of read(2), @c errno is saved
    ssize_t read_fd(int fd, int* savedErrno);

private:
    char* begin() {
        return &*buffer_.begin();
    }

    [[nodiscard]] const char* begin() const {
        return &*buffer_.begin();
    }

    void make_space(size_t len) {
        if (writable_bytes() + prependable_bytes() < len + k_cheap_prepend) {
            buffer_.resize(writer_index_ + len);
        } else {
            assert(k_cheap_prepend < reader_index_);
            size_t readable = readable_bytes();
            std::copy(begin() + reader_index_, begin() + writer_index_, begin() + k_cheap_prepend);
            reader_index_ = k_cheap_prepend;
            writer_index_ = reader_index_ + readable;
            assert(readable == readable_bytes());
        }
    }

    std::vector<char> buffer_;
    size_t reader_index_;
    size_t writer_index_;
};

#endif