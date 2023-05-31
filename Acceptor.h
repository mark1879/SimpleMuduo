#pragma once
#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;
    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &cb) 
    {
        new_connection_callback_ = cb;
    }

    bool listenning() const { return listenning_; }
    void Listen();
private:
    void HandleRead();
    
    EventLoop *loop_;           // Acceptor用的就是用户定义的那个baseLoop，也称作mainLoop
    Socket accept_socket_;      // 服务器本地 socket
    Channel accept_channel_;
    NewConnectionCallback new_connection_callback_;
    bool listenning_;
};