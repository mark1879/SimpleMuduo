#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"
#include "Timestamp.h"
#include <errno.h>
#include <unistd.h>
#include <strings.h>

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
    , epoll_fd_(::epoll_create1(EPOLL_CLOEXEC))
    , event_list_(kInitEventListSize)
{
    if (epoll_fd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epoll_fd_);
}

Timestamp EPollPoller::Poll(int timeout_ms, ChannelList *activate_channel_list)
{
    LOG_INFO("%s: total fd count: %lu\n", __FUNCTION__, channel_map_.size());

    int events_num = ::epoll_wait(epoll_fd_, &*event_list_.begin(), static_cast<int>(event_list_.size()), timeout_ms);
    int saved_errno = errno;    // Prevents errno from being overwritten
    Timestamp now(Timestamp::now());

    if (events_num > 0)
    {
        LOG_INFO("%s: %d events triggered\n", __FUNCTION__, events_num);
        FillActivateChannels(events_num, activate_channel_list); 
        if (events_num == event_list_.size())
        {
            event_list_.resize(event_list_.size() * 2);
        }
    }
    else if (events_num == 0)
    {
        LOG_DEBUG("%s: timeout!\n", __FUNCTION__);
    }
    else
    {
        if (saved_errno != EINTR)
        {
            errno = saved_errno ;
            LOG_ERROR("%s: error!", __FUNCTION__);
        }
    }

    return now;
}

void EPollPoller::UpdateChannel(Channel* channel)
{
    const ChannelState channel_state = channel->state();
    LOG_INFO("%s: fd:%d, events:%d, channel_state:%d\n", __FUNCTION__, channel->fd(), channel->events(), static_cast<int>(channel_state));

    if (channel_state == ChannelState::kNew 
        || channel_state == ChannelState::kDeleted)
    {
        if (channel_state == ChannelState::kNew)
        {
            channel_map_[channel->fd()] = channel;
        }

        channel->set_state(ChannelState::kAdded);
        Update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        if (channel->IsNoneEvent())
        {
            Update(EPOLL_CTL_DEL, channel);
            channel->set_state(ChannelState::kDeleted);
        }
        else
        {
            Update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::RemoveChannel(Channel* channel)
{
    int fd = channel->fd();
    channel_map_.erase(fd);

    LOG_INFO("%s, fd:%d\n", __FUNCTION__, fd);

    if (channel->state() == ChannelState::kAdded)
    {
        Update(EPOLL_CTL_DEL, channel);
    }

    channel->set_state(ChannelState::kNew);
}

void EPollPoller::FillActivateChannels(int events_num, ChannelList *activate_channel_list) const
{
    for (int i = 0; i < events_num; ++i)
    {
        Channel* channel = static_cast<Channel *>(event_list_[i].data.ptr);
        channel->set_revents(event_list_[i].events);
        activate_channel_list->push_back(channel);
    }
}

void EPollPoller::Update(int operation, Channel* channel)
{
    int fd = channel->fd();

    epoll_event event;
    bzero(&event, sizeof(event));
    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if (::epoll_ctl(epoll_fd_, operation, fd, &event) < 0)
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

