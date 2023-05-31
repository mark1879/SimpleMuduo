#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

// 封装socket地址类型
class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr)
        : sock_addr_(addr)
    {}

    std::string ToIp() const;
    std::string ToIpPort() const;
    uint16_t ToPort() const;

    const sockaddr_in* sock_addr() const {return &sock_addr_;}
    void set_sock_addr(const sockaddr_in &addr) { sock_addr_ = addr; }
private:
    sockaddr_in sock_addr_;
};