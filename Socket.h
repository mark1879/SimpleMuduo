#pragma once

#include "InetAddress.h"
#include "noncopyable.h"

class InetAddress;

class Socket : noncopyable {
public:
    explicit Socket(int sockfd)
        : sock_fd_(sockfd)
        {}

    ~Socket();

    int get_sock_fd() const { return sock_fd_; }
    void BindAddress(const InetAddress &local_addr);
    void Listen();
    int Accept(InetAddress *peer_addr);

    void ShutdownWrite();

    void SetTcpNoDelay(bool on);
    void SetReuseAddr(bool on);
    void SetReusePort(bool on);
    void SetKeepAlive(bool on);

private:
    const int sock_fd_;
};