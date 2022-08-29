
#include <fractals/network/p2p/Epoll.ipp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <sstream>
#include <unistd.h>

using namespace fractals::network::p2p;

struct Fd
{
    int32_t fd;

    public:
    int32_t getFileDescriptor()
    {
        return fd;
    }
};

int READSIZE=64*1024;

void write_to_pipe (int fd, std::string text)
{
  FILE *stream;
  stream = fdopen (fd, "w");
  fprintf (stream, "%s", text.c_str());
  fclose (stream);
}

TEST(EPOLL, create_delete)
{
    auto epoll = Epoll<Fd>::createEpoll(-1);

    ASSERT_TRUE(epoll);

    EXPECT_NO_THROW((void)epoll.release());
}

TEST(EPOLL, add_and_remove_listener)
{
    auto epoll = Epoll<Fd>::createEpoll(-1);

    ASSERT_TRUE(epoll);

    auto std_in = std::make_unique<Fd>(Fd{0});
    int res = epoll->addListener(std_in, Epoll<Fd>::Action::READ);

    ASSERT_TRUE(res);

    int res2 = epoll->removeListener(std_in);

    ASSERT_TRUE(res2);

    auto fd = std::make_unique<Fd>(Fd{1});
    int res3 = epoll->removeListener(fd);

    ASSERT_EQ(res3, -1);

    EXPECT_NO_THROW((void)epoll.release());
}

TEST(EPOLL, wait)
{
    auto epoll = Epoll<Fd>::createEpoll(-1);

    ASSERT_TRUE(epoll);

    int mypipe[2];
    pipe(mypipe);

    auto readFd = std::unique_ptr<Fd>(new Fd{mypipe[0]});
    int res = epoll->addListener(readFd, Epoll<Fd>::Action::READ);

    ASSERT_TRUE(res);

    
    write_to_pipe(mypipe[1], "test");

    auto waitResult = epoll->wait();

    ASSERT_EQ(waitResult.resultCode, 1);
    ASSERT_EQ(waitResult.mData.size(), 1);
    ASSERT_EQ(waitResult.mData[0].data.fd, readFd->getFileDescriptor());
    
    char read_buf[READSIZE];
    int bytes_read = read(waitResult.mData[0].data.fd, read_buf, READSIZE);

    ASSERT_EQ(bytes_read, READSIZE);

    std::string s(read_buf);

    ASSERT_EQ(s, "test");
}
