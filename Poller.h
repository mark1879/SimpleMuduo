#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    // 给所有 IO 复用保留统一的接口
    virtual Timestamp Poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void UpdateChannel(Channel *channel) = 0;
    virtual void RemoveChannel(Channel *channel) = 0;
    
    // 判断参数 channel 是否在当前 Poller 当中
    bool HasChannel(Channel *channel) const;

    // EventLoop 可以通过该接口获取默认的 IO 复用的具体实现
    static Poller* NewDefaultPoller(EventLoop *loop);
protected:
    // key：sockfd
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
private:
    // 定义 Poller 所属的事件循环 EventLoop
    EventLoop *owner_loop_; 
};