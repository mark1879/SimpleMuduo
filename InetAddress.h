#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

class InetAdress {
public:
    explicit InetAdress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAdress(const sockaddr_in &addr) : addr_(addr) { }


private:
    sockaddr_in addr_;
};