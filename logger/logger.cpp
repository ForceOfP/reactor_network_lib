#include "logger.hpp"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>

static std::string getCurrentTime(std::chrono::system_clock::time_point& t) {
	std::time_t now_time_t = std::chrono::system_clock::to_time_t(t);
    std::tm* now_tm = std::localtime(&now_time_t);

	char buffer[128];
	strftime(buffer, sizeof(buffer), "[%F %T", now_tm);
	std::ostringstream ss;
	ss.fill('0');

	std::chrono::milliseconds ms;

    ms = std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch()) % 1000;
    ss << buffer << '.' << ms.count() << "] ";

	return ss.str();
}

void LogWritter::write(Logger* logger) {
    std::vector<LogInfo> trash{};
    logger->output(trash);
    std::stringstream ss;
    for (auto& log: trash) {
        ss << getCurrentTime(log.time_stamp) << log.context << '\n';
    }
    std::ofstream ofs;
    ofs.open(logger->log_path_, std::ios::app);
    ofs << ss.str();
}

void LogWritter::register_as(Logger* logger) {
    loggers_.push_back(logger);
}

void LogWritter::run(std::chrono::milliseconds delay) {
    while (running_) {
        std::this_thread::sleep_for(delay);

        for (auto logger: loggers_) {        
            std::lock_guard<std::mutex> lock{logger->collect_mutex_};
            if (!logger->container_.empty()) {
                write(logger);
                if (static_cast<int>(logger->state_) >= 6) {
                    abort();
                }
            }
        }
    }
}

void LogWritter::stop_run() {
    running_ = false;
}

Logger& Logger::operator<<(std::string_view context) {
    std::lock_guard<std::mutex> lock{collect_mutex_};
    container_.emplace_back();
    container_.back().context = context;
    container_.back().time_stamp = std::chrono::system_clock::now();
    return *this;
}

void Logger::output(std::vector<LogInfo>& trash) noexcept {
    std::swap(container_, trash);
}

Logger::Logger(std::string path, LogWritter& writter, int level): 
    log_path_(std::move(path)), writter_(writter), state_(static_cast<State>(level)) {    
    std::ofstream ofs;
    ofs.open(log_path_, std::ios::trunc);
    writter.register_as(this);
}

void Logger::register_to(LogWritter& writter) {
    writter.register_as(this);
}

Logger::~Logger() {
    writter_.stop_run();
    writter_.write(this);
}

LogWritter g_writter;
Logger LOG_TRACE("event_trace.log", g_writter, 1);
Logger LOG_DEBUG("event_debug.log", g_writter, 2);
Logger LOG_INFO("event_info.log", g_writter, 3);
Logger LOG_WARN("event_warn.log", g_writter, 4);
Logger LOG_ERROR("event_error.log", g_writter, 5);
Logger LOG_FATAL("event_fatal.log", g_writter, 6);
Logger LOG_SYSERR("event_system_error.log", g_writter, 5);
Logger LOG_SYSFATAL("event_system_fatal.log", g_writter, 6);