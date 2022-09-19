#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <strings.h>

const int kNew = -1;        // the channel is new
const int kAdded = 1;       // the channel has been added to epoller
const int kDeleted = 2;     // the channel has been removed from epoller

EPollPoller::EPollPoller(EventLoop *loop) 
    : Poller(loop), epollfd_(epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize) {

    if (epollfd_ < 0)
        LOG_FATAL("%s, epoll_create error:%d\n", __FUNCTION__, errno);
}

EPollPoller::~EPollPoller() {
    close(epollfd_);
}

TimeStamp EPollPoller::Poll(int timeout_ms, ChannelList *active_channels) {
    LOG_INFO("%s, fd total count:%lu \n", __FUNCTION__, channels_.size());

    int num_events = epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeout_ms);
    int save_errno = errno;
    TimeStamp now(TimeStamp::now());

    if (num_events > 0) {

        LOG_INFO("%s, %d events happened\n", __FUNCTION__, num_events);
        FillActiveChannels(num_events, active_channels);

        if (num_events == events_.size()) {
            events_.resize(events_.size() * 2);
        }  
    } else if (num_events == 0) {

        LOG_DEBUG("%s, timeout!\n", __FUNCTION__);
    } else {

        if (save_errno != EINTR){
            errno = save_errno;
            LOG_ERROR("%s, err!", __FUNCTION__);
        }
    }

    return now;
}

void EPollPoller::UpdateChannel(Channel *channel) {
    const int index = channel->index();

    LOG_INFO("%s, fd=%d  events=%d  index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);

    if (index == kNew || index == kDeleted) {
        if (index == kNew) {
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(kAdded);
        Update(EPOLL_CTL_ADD, channel);
    } else {    // channel has been registed

        int fd = channel->fd();
        if (channel->IsNoneEvent()) {
            Update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        } else {
            Update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::RemoveChannel(Channel *channel) {
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("%s, fd=%d\n", __FUNCTION__, fd);

    int index = channel->index();
    if (index == kAdded) {
        Update(EPOLL_CTL_DEL, channel);
    }

    channel->set_index(kNew);
}


// record the activate connections
void EPollPoller::FillActiveChannels(int num_events, ChannelList *activate_channels) const {
    for (int i = 0; i < num_events; i++) {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activate_channels->push_back(channel);
    }
}

// upate the chanel's operation, epoll_ctl add/mod/del
void EPollPoller::Update(int operation, Channel *channel) {
    epoll_event event;
    bzero(&event, sizeof(event));

    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel; 

    if (epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            LOG_ERROR("%s, epoll_ctl del error:%d\n", __FUNCTION__, errno);
        } else {
            LOG_FATAL("%s, epoll_ctl add/mod error:%d\n", __FUNCTION__, errno);
        }
    }
}