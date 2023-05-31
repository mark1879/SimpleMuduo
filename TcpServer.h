#pragma once

/**
 * 用户使用muduo编写服务器程序
 */ 
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

// 对外的服务器编程使用的类
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
                const InetAddress &listenAddr,
                const std::string &nameArg,
                Option option = kNoReusePort);
    ~TcpServer();

    void set_thread_init_callback(const ThreadInitCallback &cb) { thread_init_callback_ = cb; }
    void set_connection_callback(const ConnectionCallback &cb) { connection_callback_ = cb; }
    void set_message_callback(const MessageCallback &cb) { message_callback_ = cb; }
    void set_write_complete_callback(const WriteCompleteCallback &cb) { write_complete_callback_ = cb; }

    // 设置底层subloop的个数
    void SetThreadNum(int num_threads);

    // 开启服务器监听
    void Start();
private:
    void NewConnection(int sockfd, const InetAddress &peerAddr);
    void RemoveConnection(const TcpConnectionPtr &conn);
    void RemoveConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;


    EventLoop *loop_;               // base loop 用户定义
    const std::string ip_port_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_;                 // 运行在mainLoop，任务就是监听新连接事件
    std::shared_ptr<EventLoopThreadPool> thread_pool_;   // one loop per thread

    ConnectionCallback connection_callback_;             // 有新连接时的回调
    MessageCallback message_callback_;                   // 有读写消息时的回调
    WriteCompleteCallback write_complete_callback_;      // 消息发送完成以后的回调
    ThreadInitCallback thread_init_callback_;           // loop线程初始化的回调

    std::atomic_int started_;
    int next_conn_id_;
    ConnectionMap connections_;     // 保存所有的连接
};