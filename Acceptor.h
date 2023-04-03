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
    using NewConnectionCallback = std::function<void(int sock_fd, const InetAddress&)>;

    Acceptor(EventLoop *loop, const InetAddress &listen_addr, bool reuse_port);
    ~Acceptor();

    void set_new_connection_callback(const NewConnectionCallback &cb) 
    {   new_connection_callback_ = std::move(cb); }

    bool listenning() const { return listenning_; }
    void Listen();
private:
    void HandleRead();
    
    EventLoop *loop_;       // main loop, created by user
    Socket accept_socket_;
    Channel accept_channel_;
    NewConnectionCallback new_connection_callback_;
    bool listenning_;
};