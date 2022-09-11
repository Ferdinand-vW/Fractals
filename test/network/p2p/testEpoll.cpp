
#include "fractals/network/epoll/Epoll.h"
#include "fractals/network/epoll/IEpoll.h"
#include <fractals/network/epoll/Epoll.ipp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>
#include <optional>
#include <sstream>
#include <sys/epoll.h>
#include <unistd.h>

using namespace fractals::network::epoll;

struct MockEpoll
{
    MockEpoll()
    {

    }

    MOCK_METHOD(int,epoll_ctl,(int, int, struct epoll_event*));
    MOCK_METHOD(int,epoll_wait,(struct epoll_event *events, int, int));
    MOCK_METHOD(void,close,());

    static std::unique_ptr<MockEpoll> epoll_create(int size)
    {
        return std::make_unique<MockEpoll>();
    }
};

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
    auto epollRes = EpollImpl<IEpoll>::epollCreate();

    ASSERT_EQ(epollRes.mErrc, ErrorCode::None);

    EXPECT_NO_THROW((void)epollRes.mEpoll.release());
}

TEST(EPOLL, add_and_remove_listener)
{
    auto epoll = EpollImpl<IEpoll>::epollCreate();

    ASSERT_EQ(epoll.mErrc, ErrorCode::None);

    auto std_in = std::make_unique<Fd>(Fd{0});
    auto res = epoll->add(std_in, EventCode::EpollIn);

    std::cout << ErrorCode::None << std::endl;

    ASSERT_EQ(res.mErrc, ErrorCode::None);

    auto res2 = epoll->erase(std_in);

    ASSERT_EQ(res2.mErrc, ErrorCode::None);

    auto fd = std::make_unique<Fd>(Fd{1});
    auto res3 = epoll->erase(fd);

    ASSERT_EQ(res3.mErrc, ErrorCode::Einval);

    EXPECT_NO_THROW((void)epoll->close());
}

TEST(EPOLL, wait)
{
    auto epoll = EpollImpl<IEpoll>::epollCreate();

    ASSERT_EQ(epoll.mErrc, ErrorCode::None);

    int mypipe[2];
    int pipeRes = pipe(mypipe);

    ASSERT_EQ(pipeRes, 0);

    auto readFd = std::unique_ptr<Fd>(new Fd{mypipe[0]});
    auto res = epoll->add(readFd, EventCode::EpollIn);

    ASSERT_EQ(res.mErrc, ErrorCode::None);
    
    std::string input("test");
    write_to_pipe(mypipe[1], input);

    auto waitResult = epoll->wait();

    ASSERT_EQ(waitResult.mErrc, ErrorCode::None);
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
    auto epoll = EpollImpl<IEpoll>::epollCreate();

    ASSERT_EQ(epoll.mErrc, ErrorCode::None);

    int mypipe[2];
    int pipeRes = pipe(mypipe);

    ASSERT_EQ(pipeRes, 0);

    auto readFd = std::unique_ptr<Fd>(new Fd{mypipe[0]});
    auto res = epoll->add(readFd, EventCode::EpollIn);

    ASSERT_EQ(res.mErrc, ErrorCode::None);

    auto waitResult = epoll->wait(0);

    ASSERT_EQ(waitResult.mErrc, ErrorCode::None);
    ASSERT_EQ(waitResult->size(), 0);    
}

TEST(EPOLL, mod_fd_does_not_exist)
{
    auto epoll = EpollImpl<MockEpoll>::epollCreate();

    auto readFd = std::unique_ptr<Fd>(new Fd{0});

    auto ctl_res = epoll->mod(readFd, EventCode::EpollOut);

    ASSERT_EQ(ctl_res.mErrc, ErrorCode::EnoEnt);
}

TEST(EPOLL, mod_update_events)
{
    auto epoll = EpollImpl<MockEpoll>::epollCreate();

    auto readFd = std::unique_ptr<Fd>(new Fd{0});

    EXPECT_CALL(*epoll->getUnderlying(),epoll_ctl).Times(4);

    ASSERT_EQ(epoll->getEvents(readFd), EventCodes({}));

    auto res1 = epoll->add(readFd, EventCode::EpollIn);
    ASSERT_EQ(res1.mErrc, ErrorCode::None);

    ASSERT_EQ(epoll->getEvents(readFd), EventCode::EpollIn);

    auto res2 = epoll->mod(readFd, EventCode::EpollErr);
    ASSERT_EQ(res2.mErrc, ErrorCode::None);

    ASSERT_EQ(epoll->getEvents(readFd), EventCode::EpollErr);

    auto res3 = epoll->mod(readFd, EventCode::EpollErr | EventCode::EpollIn);
    ASSERT_EQ(res3.mErrc, ErrorCode::None);

    ASSERT_EQ(epoll->getEvents(readFd), EventCode::EpollErr | EventCode::EpollIn);

    auto res4 = epoll->erase(readFd);
    ASSERT_EQ(res4.mErrc, ErrorCode::None);

    ASSERT_EQ(epoll->getEvents(readFd), EventCodes({}));

}

