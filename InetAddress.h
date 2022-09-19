#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

class InetAdress {
public:
    explicit InetAdress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAdress(const sockaddr_in &addr) : sock_addr_(addr) { }

    std::string ToIp() const;
    std::string ToIpPort() const;
    uint16_t ToPort() const;

    const sockaddr_in* get_sock_addr() const { return &sock_addr_; }
private:
    sockaddr_in sock_addr_;
};