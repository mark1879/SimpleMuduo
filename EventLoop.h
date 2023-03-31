#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "CurrentThread.h"
#include "Timestamp.h"

class Channel;
class Poller;

class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void Loop();
    void Quit();

    Timestamp PollReturnTime() const { return poll_return_time_; }

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

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_;
    std::atomic_bool quit_;

    const pid_t thread_id_;

    Timestamp poll_return_time_;
    std::unique_ptr<Poller> poller_;

    // used to wake up sub channel
    int wakeup_fd_;
    std::unique_ptr<Channel> wakeup_channel_;

    ChannelList activate_channel_list_;

    std::atomic_bool calling_pending_functors_;
    std::vector<Functor> pending_functors_;
    std::mutex mutex_;
};