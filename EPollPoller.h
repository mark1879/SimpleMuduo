#pragma once

#include "Poller.h"
#include "Timestamp.h"
#include <vector>
#include <sys/epoll.h>

class Channel;

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    TimeStamp Poll(int timeout_ms, ChannelList *activate_channel_list) override;
    void UpdateChannel(Channel* channel) override;
    void RemoveChannel(Channel* channel) override;

private:
    void FillActivateChannels(int events_num, ChannelList *activate_channel_list) const;
    void Update(int operation, Channel* channel);

private:
    using EventList = std::vector<epoll_event>;

    int epoll_fd_;
    EventList event_list_;
    static const int kInitEventListSize = 16;
};