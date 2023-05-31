#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <strings.h>

// channel 未添加到 poller 中
const int kNew = -1;  
// channel 已添加到 poller 中
const int kAdded = 1;
// channel 从 poller 中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}


EPollPoller::~EPollPoller() 
{
    ::close(epollfd_);
}


Timestamp EPollPoller::Poll(int timeout_ms, ChannelList *active_channels)
{
    LOG_INFO("func=%s => fd total count:%lu \n", __FUNCTION__, channels_.size());

    int num_events = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeout_ms);
    int save_errno = errno;
    Timestamp now(Timestamp::Now());

    if (num_events > 0)
    {
        LOG_INFO("%d events happened \n", num_events);

        FillActiveChannels(num_events, active_channels);
        if (num_events == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (num_events == 0)
    {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else
    {
        if (save_errno != EINTR)
        {
            errno = save_errno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }

    return now;
}


// Channel Update/Remove => EventLoop UpdateChannel/RemoveChannel => Poller UpdateChannel/RemoveChannel
void EPollPoller::UpdateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);

    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(kAdded);
        Update(EPOLL_CTL_ADD, channel);
    }
    else  // channel 已经在 poller 上注册过了
    {
        int fd = channel->fd();
        if (channel->IsNoneEvent())
        {
            Update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            Update(EPOLL_CTL_MOD, channel);
        }
    }
}


// 从 poller 中删除 channel
void EPollPoller::RemoveChannel(Channel *channel) 
{
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);
    
    int index = channel->index();
    if (index == kAdded)
    {
        Update(EPOLL_CTL_DEL, channel);
    }

    channel->set_index(kNew);
}


void EPollPoller::FillActiveChannels(int num_events, ChannelList *active_channels) const
{
    for (int i=0; i < num_events; ++i)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);

        // EventLoop 拿到它的 poller 给它返回的所有发生事件的 channel 列表
        active_channels->push_back(channel); 
    }
}

// 更新 channel 通道 epoll_ctl add/mod/del
void EPollPoller::Update(int operation, Channel *channel)
{
    epoll_event event;
    bzero(&event, sizeof event);
    
    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = fd; 
    event.data.ptr = channel;
    
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}