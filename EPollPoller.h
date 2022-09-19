#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Channel;

class EPollPoller : public Poller {
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    TimeStamp Poll(int timeout_ms, ChannelList *active_channels)  override;
    void UpdateChannel(Channel *channel) override;
    void RemoveChannel(Channel *Channel) override;

private:
    static const int kInitEventListSize = 16;

    void FillActiveChannels(int num_events, ChannelList *active_channels) const;
    void Update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};