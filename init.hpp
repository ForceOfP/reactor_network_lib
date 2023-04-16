#include "logger/logger.hpp"
#include <thread>
#include "time_utils/TimeDataStructure.hpp"

void init_log() {
    std::thread writter_thread(&LogWritter::run, &g_writter, 500ms);
    writter_thread.detach();
}
