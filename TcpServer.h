#pragma once

#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop,
                const InetAddress &listen_addr,
                const std::string &name,
                Option option = kNoReusePort);
    ~TcpServer();

    void set_thread_init_callback(const ThreadInitCallback &cb) { thread_init_callback_ = cb; }
    void set_connection_callback(const ConnectionCallback &cb) { connection_callback_ = cb; }
    void set_message_callback(const MessageCallback &cb) { message_callback_ = cb; }
    void set_write_complete_callback(const WriteCompleteCallback &cb) { write_complete_callback_ = cb; }

    void SetThreadNum(int num_threads);
    void Start();
private:
    void NewConnection(int sock_fd, const InetAddress &peer_addr);
    void RemoveConnection(const TcpConnectionPtr &conn);
    void RemoveConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_;   // base loop created by user

    const std::string ip_port_;
    const std::string name_;
    int next_conn_id_;
    ConnectionMap connections_;

    std::atomic_int started_;
    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> thread_pool_;

    ThreadInitCallback thread_init_callback_;
    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    WriteCompleteCallback write_complete_callback_;
};