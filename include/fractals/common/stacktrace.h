#pragma once

#include <iostream>
#include <stdexcept>
#include <unistd.h>
// #define BOOST_STACKTRACE_USE_BACKTRACE

#include <boost/stacktrace.hpp>
#include <execinfo.h>
#include <fstream>

namespace fractals::common
{

inline void exceptionHandler()
{
    void *array[1024];
    size_t size = backtrace(array, 1024);
    char **btSyms = backtrace_symbols(array, size);

    std::ofstream ofs("logs/err.txt");
    ofs << "Error: signal abort:\n";
    for (int i = 0; i < size; ++i)
    {
        size_t len = strlen(btSyms[i]);
        ofs.write(btSyms[i], len);
        ofs << "\n";
    }

    ofs.close();

    exit(1);
}

inline void segfaultHandler(int sig)
{
    void *array[1024];
    size_t size = backtrace(array, 1024);
    char **btSyms = backtrace_symbols(array, size);

    std::ofstream ofs("logs/err.txt");
    ofs << "Error: signal " << sig << "\n";
    for (int i = 0; i < size; ++i)
    {
        size_t len = strlen(btSyms[i]);
        ofs.write(btSyms[i], len);
        ofs << "\n";
    }

    ofs.close();

    exit(1);
}
} // namespace fractals::common