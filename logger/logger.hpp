#pragma once

#include <atomic>
#ifndef LOGGER_MARCO
#define LOGGER_MARCO

#include <fstream>
#include <ostream>
#include <string_view>
#include <thread>
#include <vector>
#include <mutex>

class Logger;

struct LogInfo {
    std::string context;
    std::chrono::system_clock::time_point time_stamp;
    LogInfo()= default;
};

class LogWritter {
public:
    LogWritter() = default;
    ~LogWritter() = default;
    void run(std::chrono::milliseconds delay);
    void stop_run();
    void register_as(Logger*);
    void write(Logger*);
private:
    std::ofstream log_handle_;
    std::vector<Logger*> loggers_;
    std::atomic_bool running_ = true;
};

class Logger {
public:
    explicit Logger(std::string path, LogWritter& writter, int level);
    ~Logger();
    Logger& operator<<(std::string_view context);
    friend class LogWritter;
    enum class State { TRACE = 1, DEBUG = 2, INFO = 3, WARN = 4, ERROR = 5, FATAL = 6, NUM_LOG_LEVELS = 7 }; 
private:
    std::vector<LogInfo> container_;
    std::mutex collect_mutex_; 
    std::string log_path_;
    LogWritter& writter_;
    State state_;

    void register_to(LogWritter& writter);

    void output(std::vector<LogInfo>& trash) noexcept;
};

extern LogWritter g_writter;
extern Logger LOG_TRACE, LOG_INFO, LOG_FATAL, LOG_ERROR, LOG_WARN, LOG_SYSERR, LOG_SYSFATAL, LOG_DEBUG;

#endif