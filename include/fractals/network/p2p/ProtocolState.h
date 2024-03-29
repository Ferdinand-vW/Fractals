#include <cstdint>

namespace fractals::network::p2p
{
    enum class ProtocolState : uint8_t
    {
        OPEN = 0,
        CLOSED = 1,
        ERROR = 2,
        HASH_CHECK_FAIL = 3,
        COMPLETE = 4
    };
}