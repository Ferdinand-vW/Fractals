#include "fractals/disk/DiskEventQueue.h"
#include "fractals/disk/DiskIOService.h"
#include "fractals/sync/QueueCoordinator.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

namespace fractals::disk
{

class MockIOLayer
{
  public:
    MOCK_METHOD(bool, createDirectory, (const std::string &));
    MOCK_METHOD(bool, createDirectories, (const std::string &));
    MOCK_METHOD(bool, pathExists, (const std::string &));

    MOCK_METHOD(bool, open, (const std::string &));
    MOCK_METHOD(void, write, (const std::vector<char> &));
    MOCK_METHOD(bool, writeFrom, (int64_t, const std::vector<char> &));

    std::istream &getStream()
    {
        return std::cin;
    }
};

class DiskIOServiceTestz : public ::testing::Test
{
  public:
    ::testing::StrictMock<MockIOLayer> ioLayer;
    DiskEventQueue queue;
    sync::QueueCoordinator coordinator;
    DiskIOService<MockIOLayer> service{coordinator, queue.getRightEnd()};
};

TEST_F(DiskIOServiceTestz, torrent)
{
}

TEST_F(DiskIOServiceTestz, piece)
{
}

TEST_F(DiskIOServiceTestz, announce)
{
}

} // namespace fractals::disk