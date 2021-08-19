#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/fmt/ostr.h>

namespace websocket {

class Logger {
public:
    static Logger& Get() {
        static Logger logger;
        return logger;
    }

    ~Logger();

    Logger& AddDebugOutputLogger();

    Logger& AddFileLogger(const std::string& file_name);

    std::shared_ptr<spdlog::logger> GetLogger(const std::string& name, bool async_logging = true);

private:
    struct LoggerImpl;

    Logger();

    std::vector<spdlog::sink_ptr> sinks_;
};

}
