#pragma once

#include "noncopyable.h"

class InetAddress;

class Socket : noncopyable
{
public:
    explicit Socket(int fd)
        : fd_(fd)
    {}

    ~Socket();

    int fd() const { return fd_; }
    void BindAddress(const InetAddress &local_addr);
    void Listen();
    int Accept(InetAddress *peer_addr);

    void ShutdownWrite();

    void SetTcpNoDelay(bool on);
    void SetReuseAddr(bool on);
    void SetReusePort(bool on);
    void SetKeepAlive(bool on);
private:
    const int fd_;
};