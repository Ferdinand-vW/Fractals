
#include "fractals/network/epoll/Epoll.h"
#include <fractals/network/epoll/Epoll.ipp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <sstream>
#include <unistd.h>

using namespace fractals::network;

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
    auto epollRes = epoll::Epoll<Fd>::epollCreate();

    ASSERT_EQ(epollRes.mErrc, epoll::ErrorCode::None);

    EXPECT_NO_THROW((void)epollRes.mEpoll.release());
}

TEST(EPOLL, add_and_remove_listener)
{
    auto epoll = epoll::Epoll<Fd>::epollCreate();

    ASSERT_EQ(epoll.mErrc, epoll::ErrorCode::None);

    auto std_in = std::make_unique<Fd>(Fd{0});
    auto res = epoll->add(std_in, epoll::EventCode::EpollIn);

    std::cout << epoll::ErrorCode::None << std::endl;

    ASSERT_EQ(res.mErrc, epoll::ErrorCode::None);

    auto res2 = epoll->erase(std_in);

    ASSERT_EQ(res2.mErrc, epoll::ErrorCode::None);

    auto fd = std::make_unique<Fd>(Fd{1});
    auto res3 = epoll->erase(fd);

    ASSERT_EQ(res3.mErrc, epoll::ErrorCode::Einval);

    EXPECT_NO_THROW((void)epoll->close());
}

TEST(EPOLL, wait)
{
    auto epoll = epoll::Epoll<Fd>::epollCreate();

    ASSERT_EQ(epoll.mErrc, epoll::ErrorCode::None);

    int mypipe[2];
    int pipeRes = pipe(mypipe);

    ASSERT_EQ(pipeRes, 0);

    auto readFd = std::unique_ptr<Fd>(new Fd{mypipe[0]});
    auto res = epoll->add(readFd, epoll::EventCode::EpollIn);

    ASSERT_EQ(res.mErrc, epoll::ErrorCode::None);
    
    std::string input("test");
    write_to_pipe(mypipe[1], input);

    auto waitResult = epoll->wait();

    ASSERT_EQ(waitResult.mErrc, epoll::ErrorCode::None);
    ASSERT_EQ(waitResult->size(), 1);
    ASSERT_EQ(waitResult->front().mData.fd, readFd->getFileDescriptor());
    
    char read_buf[READSIZE];
    int bytes_read = read(waitResult->front().mData.fd, read_buf, READSIZE);

    ASSERT_EQ(bytes_read, input.size());

    std::string s(read_buf);

    ASSERT_EQ(s, input);
}

TEST(EPOLL, wait_empty_input)
{
    auto epoll = epoll::Epoll<Fd>::epollCreate();

    ASSERT_EQ(epoll.mErrc, epoll::ErrorCode::None);

    int mypipe[2];
    int pipeRes = pipe(mypipe);

    ASSERT_EQ(pipeRes, 0);

    auto readFd = std::unique_ptr<Fd>(new Fd{mypipe[0]});
    auto res = epoll->add(readFd, epoll::EventCode::EpollIn);

    ASSERT_EQ(res.mErrc, epoll::ErrorCode::None);

    auto waitResult = epoll->wait(0);

    ASSERT_EQ(waitResult.mErrc, epoll::ErrorCode::None);
    ASSERT_EQ(waitResult->size(), 0);    
}
