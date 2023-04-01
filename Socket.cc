#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"

#include <unistd.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

Socket::~Socket()
{
    ::close(fd_);
}

void Socket::BindAddress(const InetAddress &local_addr)
{
    if (0 != ::bind(fd_, (sockaddr*)local_addr.sock_addr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind sockfd:%d failed, errno: %d\n", fd_, errno);
    }
}

void Socket::Listen()
{
    if (0 != listen(fd_, 1024))
    {
        LOG_FATAL("listen sockfd:%d failed, errno: %d\n", fd_, errno);
    }
}

int Socket::Accept(InetAddress *peer_addr)
{
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    bzero(&addr, sizeof(addr));
    int conn_fd = ::accept4(fd_, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (conn_fd >= 0)
    {
        peer_addr->set_sock_addr(addr);
    }
    else
    {
        LOG_FATAL("accept sockfd: %d failed, errno: %d\n", fd_, errno);
    }

    return conn_fd;
}

void Socket::ShutdownWrite()
{
    if (::shutdown(fd_, SHUT_WR) < 0)
    {
        LOG_ERROR("shutdown sockfd: %d failed, errno: %d\n", fd_, errno);
    }
}

void Socket::SetTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void Socket::SetReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::SetReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}

void Socket::SetKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}