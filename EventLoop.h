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

class EventLoop {
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // start loop
    void Loop();
    // end loop
    void Quit();

    TimeStamp poll_return_time() const { return poll_return_time_; }

    void RunInLoop(Functor cb);
    void QueueInLoop(Functor cb);
    
    void WakeUp();

    void UpdateChannel(Channel *channel);
    void RemoveChannel(Channel *channel);
    bool HasChannel(Channel *channel);

    bool IsInLoopThread() const { return thread_id_ == CurrentThread::tid(); }
private:
    void HandleRead();
    void DoPendingFunctors();
    
    using ChannelList = std::vector<Channel *>;

    std::atomic_bool looping_;
    std::atomic_bool quit_;

    const pid_t thread_id_;

    TimeStamp poll_return_time_;
    std::unique_ptr<Poller> poller_;

    int wakeup_fd_;  // 主要作用，当mainLoop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该成员唤醒subloop处理channel
    std::unique_ptr<Channel> wakeup_channel_;

    ChannelList active_channels_;

    std::atomic_bool calling_pending_functors_;
    std::vector<Functor> pending_functors_;
    std::mutex mutex_;
};