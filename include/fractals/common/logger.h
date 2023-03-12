#pragma once

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace fractals::common {

    inline void setupLogging()
    {
        auto logger = spdlog::basic_logger_mt("basic_logger", "logs/log.txt");
        logger->set_pattern("[%t][%E %F] %v");
        spdlog::set_default_logger(logger);
        
        spdlog::info("Logging initialized");
    }
}