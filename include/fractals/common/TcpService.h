#pragma once

#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/socket.h>

namespace fractals::common
{
class TcpService
{
  public:
    TcpService() = default;

    int32_t connect(const std::string& host, uint32_t port)
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);

        if (fd < 0)
        {
            spdlog::error("TCPService::connect. Failed to create socket");
            return fd;
        }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) < 0)
        {
            spdlog::error("TCPService::connect. inet python fail");
            return -1;
        }

        int flags = fcntl(fd, F_GETFL, 0);
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        {
            spdlog::error("TCPService::connect. Could not set socket to NON_BLOCKING");
            return -1;
        }

        if (::connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 && errno != EINPROGRESS)
        {
            spdlog::error("TCPService::connect. connect fail {}", strerror(errno));
            return -1;
        }

        return fd;
    }
};
} // namespace fractals::common