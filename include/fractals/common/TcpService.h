#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
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
            return fd;
        }

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) < 0)
        {
            return -1;
        }

        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        if (::connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 || errno != EINPROGRESS)
        {
            return -1;
        }

        return fd;
    }
};
} // namespace fractals::common