#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

class Poller : noncopyable {
public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    virtual TimeStamp Poll(int timeout_ms, ChannelList *active_channels)  = 0;
    virtual void UpdateChannel(Channel *channel) = 0;
    virtual void RemoveChannel(Channel *Channel) = 0;

    bool HasChannel(Channel *channel) const;

    static Poller* NewDefaultPoller(EventLoop *loop);

protected:
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;

private:
    EventLoop *ower_loop_;
};