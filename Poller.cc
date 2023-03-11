#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop* loop)
        : owner_loop_(loop)
{}

bool Poller::HasChannel(Channel* channel) const
{
    auto it = channel_map_.find(channel->fd());

    return it != channel_map_.end() && it->second == channel;
}

