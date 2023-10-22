#include "fractals/disk/IOLayer.h"
#include "fractals/common/utils.h"
#include <filesystem>
#include <spdlog/spdlog.h>
#include <system_error>

namespace fractals::disk
{

IOLayer::~IOLayer()
{
    stream.close();
}

bool IOLayer::createDirectories(const std::filesystem::path &path)
{
    std::error_code errorCode;
    if (!std::filesystem::create_directories(path.parent_path(), errorCode))
    {
        if (!errorCode)
        {
            spdlog::info("IOLayer::createDirectories. Path already exists {}",
                         path.parent_path().c_str());
            return true;
        }

        spdlog::error("IOLayer::createDirectories. Error {} occurred when creating directory: {}",
                      errorCode.message(), path.parent_path().c_str());
        return false;
    }
    else
    {
        spdlog::info("IOLayer::createDirectories. Created dirs in path {}",
                     path.parent_path().c_str());
    }

    return true;
}

bool IOLayer::createFile(const std::filesystem::path &path)
{
    if (std::filesystem::exists(path))
    {
        spdlog::info("IOLayer:: file already exists {}", path.c_str());
        return true;
    }

    if (stream.is_open())
    {
        spdlog::warn(
            "IOLayer::createFile. Stream already opened. Closing before opening new stream.");
        stream.close();
    }

    stream.open(path, std::fstream::out);

    if (stream.fail())
    {
        spdlog::error("IOLayer::createFile. Error '{}' occurred when creating: {}",
                      std::strerror(errno), path.c_str());
        return false;
    }

    stream.close();

    return true;
}

bool IOLayer::open(const std::filesystem::path &filePath)
{
    if (stream.is_open())
    {
        spdlog::warn("IOLayer::open. Stream already opened. Closing before opening new stream.");
        stream.close();
    }

    if (!std::filesystem::exists(filePath.parent_path()))
    {
        spdlog::error("IOLayer::open. File path does not exist: {}", filePath.c_str());
        return false;
    }

    stream.open(filePath, std::fstream::in | std::fstream::out | std::fstream::binary);

    if (stream.fail())
    {
        spdlog::error("IOLayer::open. Error '{}' occurred when opening: {}", std::strerror(errno),
                      filePath.c_str());
        return false;
    }

    return true;
}

void IOLayer::close()
{
    if (!stream.is_open())
    {
        spdlog::warn("IOLayer::close. Stream not opened. Cannot close non-open stream.");
    }

    stream.close();
}

void IOLayer::write(std::string_view bytes, uint64_t numBytes)
{
    if (!stream.is_open())
    {
        spdlog::error("IOLayer::write. Cannot write to closed stream.");
        return;
    }

    stream.write(bytes.data(), numBytes);
}

void IOLayer::writeFrom(int64_t offset, std::string_view bytes, uint64_t numBytes)
{
    if (!stream.is_open())
    {
        spdlog::error("IOLayer::write. Cannot write to closed stream.");
        return;
    }

    stream.seekp(offset);

    spdlog::debug("IOLayer::writeFrom. Offset={} tellp={} tellg={}", offset, stream.tellp(),
                  stream.tellg());

    this->write(bytes, numBytes);
}

std::istream &IOLayer::getStream()
{
    return stream;
}

} // namespace fractals::disk