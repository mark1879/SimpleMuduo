#include "InetAddress.h"
#include <strings.h>
#include <string.h>

InetAddress::InetAddress(uint16_t port, std::string ip)
{
    bzero(&sock_addr_, sizeof(sock_addr_));
    sock_addr_.sin_family = AF_INET;
    sock_addr_.sin_port = htons(port);
    sock_addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

std::string InetAddress::ToIP() const 
{
    char buf[kMaxBufSize] = {0};

    ::inet_ntop(AF_INET, &sock_addr_.sin_addr, buf, sizeof(buf));

    return buf;
}

std::string InetAddress::ToIPPort() const
{
    char buf[kMaxBufSize] = {0};

    ::inet_ntop(AF_INET, &sock_addr_.sin_addr, buf, sizeof(buf));
    size_t end = strlen(buf);
    uint16_t port = ntohs(sock_addr_.sin_port);
    sprintf(buf + end, ":%u", port);

    return buf;
}

uint16_t InetAddress::ToPort() const
{
    return ::ntohs(sock_addr_.sin_port);
}
