#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr) {}

    std::string ToIP() const;
    std::string ToIPPort() const;
    uint16_t ToPort() const;

    const sockaddr_in* sock_addr() const 
    { return &sock_addr_; }

    void set_sock_addr(const sockaddr_in &addr) 
    { sock_addr_ = addr; }

private:
    sockaddr_in sock_addr_;
    const size_t kMaxBufSize = 64;
};