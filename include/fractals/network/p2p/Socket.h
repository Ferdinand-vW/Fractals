#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <sys/socket.h>

namespace fractals::network::p2p
{
    class Socket
    {
        int32_t mFd{-1};

        public:
            Socket(int32_t fd): mFd(fd){}

            static std::unique_ptr<Socket> makeClientSocket(std::string address, uint16_t port)
            {
                auto fd = socket(AF_INET, SOCK_STREAM, 0);
                return std::unique_ptr<Socket>(new Socket(fd));
            }
            static std::unique_ptr<Socket> makeServerSocket();

            int32_t getFileDescriptor() const
            {
                return mFd;
            }

            void close();
    };
}