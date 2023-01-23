#include <cstdint>
#include <variant>
#include <vector>

namespace fractals::disk
{
    struct WriteData
    {
        uint32_t mPieceIndex;
        std::vector<char> mData;
    };

    using DiskEvent = std::variant<WriteData>;
}