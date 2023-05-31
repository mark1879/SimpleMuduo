#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Channel;

/**
 * epoll的使用  
 * epoll_create
 * epoll_ctl    add/mod/del
 * epoll_wait
 */ 
class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    Timestamp Poll(int timeout_ms, ChannelList *active_channels) override;
    void UpdateChannel(Channel *channel) override;
    void RemoveChannel(Channel *channel) override;
private:
    static const int kInitEventListSize = 16;

    // 填写活跃的连接
    void FillActiveChannels(int num_events, ChannelList *active_channels) const;
    // 更新 channel 通道
    void Update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};