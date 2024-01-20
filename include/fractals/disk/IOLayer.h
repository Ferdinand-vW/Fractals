#pragma once

#include <fractals/common/utils.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fractals::disk
{
class IOLayer
{
  public:
    ~IOLayer();
    bool createDirectories(const std::filesystem::path& dir);
    bool createFile(const std::filesystem::path &path);

    bool open(const std::filesystem::path& filePath);
    void close();
    void write(std::string_view bytes, uint64_t numBytes);
    void writeFrom(int64_t offset, std::string_view bytes, uint64_t numBytes);
    std::istream& getStream();

  private:
  
    std::fstream stream;
};
} // namespace fractals::disk