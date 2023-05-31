#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

// 主要包含了两个大模块 Channel、Poller
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void Loop();
    // 退出事件循环
    void Quit();

    Timestamp poll_return_time() const { return poll_return_time_; }
    
    // 在当前 loop 中执行 cb
    void RunInLoop(Functor cb);
    // 把 cb 放入队列中，唤醒 loop 所在的线程，执行 cb
    void QueueInLoop(Functor cb);

    // 用来唤醒 loop 所在的线程
    void Wakeup();

    // EventLoop的方法 =》 Poller的方法
    void UpdateChannel(Channel *channel);
    void RemoveChannel(Channel *channel);
    bool HasChannel(Channel *channel);

    // 判断 EventLoop 对象是否在自己的线程里面
    bool IsInLoopThread() const { return thread_id_ ==  CurrentThread::tid(); }
private:
    void HandleRead();
    void DoPendingFunctors();

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_;
    // 标识退出 loop 循环
    std::atomic_bool quit_;      
    
     // 记录当前 loop 所在线程的 id
    const pid_t thread_id_;    

    // poller 返回发生事件的 channels 的时间点
    Timestamp poll_return_time_; 
    std::unique_ptr<Poller> poller_;

    // 当 MainLoop 获取一个新用户的 channel，
    // 通过轮询算法选择一个 subloop，通过该成员唤醒 subloop 处理 channel
    int wakeup_fd_;
    std::unique_ptr<Channel> wakeup_channel_;

    ChannelList active_channels_;

    // 标识当前 loop 是否有需要执行的回调操作
    std::atomic_bool calling_pending_functors_;

    // 存储 loop 需要执行的所有的回调操作
    std::vector<Functor> pending_functors_; 

    // 互斥锁，用来保护上面 vecto r容器的线程安全操作
    std::mutex mutex_; 
};