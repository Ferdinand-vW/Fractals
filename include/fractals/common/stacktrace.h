#pragma once

#define BOOST_STACKTRACE_USE_BACKTRACE

#include <boost/stacktrace.hpp>
#include <fstream>

namespace fractals::common
{
inline void exceptionHandler()
{
    try
    {
        std::ofstream ofs("logs/err.txt");

        ofs << boost::stacktrace::stacktrace();

        ofs.close();
    }
    catch (...)
    {
    }

    std::abort();
}

inline void segfaultHandler(int)
{
    try
    {
        std::ofstream ofs("logs/err.txt");

        ofs << "SEGFAULT:\n";
        ofs << boost::stacktrace::stacktrace();

        ofs.close();
    }
    catch (...)
    {
    }

    std::abort();
}
} // namespace fractals::common