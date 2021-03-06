#include <sstream>
#include <vector>
#ifdef _WIN32
#include <filesystem>
#else
#include <boost/filesystem.hpp>
#endif
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/msvc_sink.h>

#include "logger.h"

namespace websocket {

using spdlog::sinks::sink;

#ifdef _WIN32
class DebugOutputSink : public spdlog::sinks::base_sink<std::mutex> {
public:
    DebugOutputSink() {
    }

private:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        fmt::memory_buffer formatted;
        sink::formatter_->format(msg, formatted);
        OutputDebugStringA(fmt::to_string(formatted).c_str());
    }

    void flush_() override {
    }
};
#endif

static void CreateLogsDir() {
#ifdef _WIN32
    namespace Fs = std::filesystem;
#else
    namespace Fs = boost::filesystem;
#endif
    const Fs::path log_path("logs");

    if (!Fs::exists(log_path)) {
        Fs::create_directory(log_path);
    }
}

Logger::Logger() {
    spdlog::init_thread_pool(8192, 2);
}

Logger::~Logger() {
}

std::shared_ptr<spdlog::logger> Logger::GetLogger(const std::string& name, bool async_logging) {
    auto logger = spdlog::get(name);
    if (logger != nullptr) {
        return logger;
    }

    if (async_logging) {
        logger = std::make_shared<spdlog::async_logger>(name,
                                                        std::begin(sinks_),
                                                        std::end(sinks_),
                                                        spdlog::thread_pool(),
                                                        spdlog::async_overflow_policy::block);
    }
    else {
        logger = std::make_shared<spdlog::logger>(name,
                                                  std::begin(sinks_),
                                                  std::end(sinks_));
    }

    logger->set_level(spdlog::level::debug);
    logger->set_pattern("[%H:%M:%S.%e][%l][%n] %v");
    logger->flush_on(spdlog::level::debug);

    spdlog::register_logger(logger);
    return logger;
}

Logger& Logger::AddDebugOutputLogger() {	
    sinks_.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
#ifdef _WIN32
    sinks_.push_back(std::make_shared<DebugOutputSink>());
#endif
    return *this;
}

Logger& Logger::AddFileLogger(const std::string& file_name) {
    CreateLogsDir();

    std::ostringstream ostr;
    ostr << "logs/" << file_name;
    sinks_.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(ostr.str(), 1024 * 1024, 0));
    return *this;
}

}
