#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop *loop)
    : owner_loop_(loop)
{}

bool Poller::HasChannel(Channel *channel) const
{
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}