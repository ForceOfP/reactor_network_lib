#pragma once

#ifndef Callbacks_Marco
#define Callbacks_Marco

#include "../time_utils/TimeDataStructure.hpp"
#include <functional>
#include <memory>
#include <string_view>
#include <cassert>
#include "../buffer/Buffer.hpp"

class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using TimerCallback = std::function<void()>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*, TimeStamp)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
// TODO: add HighWaterCallback
//using HighWaterCallback = std::function<void(const TcpConnectionPtr&)>;

template <typename T> requires std::is_pointer_v<T>
T CHECK_NOTNULL(T x) {
    assert(x);
    return x;
}

#endif