#pragma once
#include "noncopyable.h"
#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;
class Timestamp;

class Poller : noncopyable
{
public: 
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller() = default;

    // Polls the I/O events
    // Must be called in the loop thread.
    virtual Timestamp Poll(int timeout_ms, ChannelList *activate_channel_list) = 0;
    
    // Changes the interested I/O events
    // Must be called in the loop thread.
    virtual void UpdateChannel(Channel* channel) = 0;

    // Remove the channel, when it destructs
    // Must be called in the loop thread.
    virtual void RemoveChannel(Channel* channel) = 0;

    bool HasChannel(Channel* channel) const;

    static Poller* NewDefaultPoller(EventLoop *loop);

protected:
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channel_map_;

private:
    EventLoop *owner_loop_;
};