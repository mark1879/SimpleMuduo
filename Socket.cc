#include "Socket.h"
#include "Logger.h"

#include <unistd.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

Socket::~Socket() {
    close(sock_fd_);
}

void Socket::BindAddress(const InetAddress &local_addr) {
    if (0 != bind(sock_fd_, (sockaddr*)local_addr.get_sock_addr(), sizeof(sockaddr_in))) {
        LOG_FATAL("bind sockfd:%d fail\n", sock_fd_);
    }
}

void Socket::Listen() {
    if (0 != listen(sock_fd_, 1024)) {
        LOG_FATAL("listen sockfd:%d fail\n", sock_fd_);
    }
}

int Socket::Accept(InetAddress *peer_addr) {
    sockaddr_in addr;
    socklen_t len_addr = sizeof(addr);
    bzero(&addr, len_addr);
    
    int conn_fd = accept4(sock_fd_, (sockaddr *)&addr, &len_addr, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (conn_fd >= 0) {
        peer_addr->set_sock_addr(addr);
    }

    return conn_fd;
}

void Socket::SetTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(sock_fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void Socket::SetReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::SetReusePort(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::SetKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(sock_fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}