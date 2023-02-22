#pragma once

#include <filesystem>

namespace fractals::test
{
    class TestData
    {
        public:
            static std::string makePath(std::string path)
            {
                std::filesystem::path s = TEST_DATA_PATH;
                return s /= path;
            }
        
        private:
            inline static std::filesystem::path getBasePath(const std::string& s)
            {
                return std::filesystem::path(s).parent_path();
            }

        inline static const std::filesystem::path TEST_DATA_PATH = getBasePath(__FILE__);
    };
}