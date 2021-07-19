#pragma once

#include <boost/move/utility_core.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/formatting_ostream.hpp>

namespace fractals::common {

    BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(logger, boost::log::sources::logger_mt)

}