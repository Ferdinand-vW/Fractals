#pragma once

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <spdlog/common.h>

namespace fractals::common {

    inline void setupLogging()
    {
        auto logger = spdlog::basic_logger_mt("basic_logger", "logs/log.txt", true);
        logger->set_pattern("[%H:%M:%S.%F] [%^%l%$] [%t] %v");

        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::debug);
        spdlog::flush_every(std::chrono::seconds(1));
        
        spdlog::info("Logging initialized");
    }
}