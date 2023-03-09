#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

void Channel::set_tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

void Channel::Update()
{

}

void Channel::Remove()
{

}

void Channel::HandleEvent(TimeStamp received_time)
{
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
            HandleEventWithGuard(received_time);
    }
    else
    {
        HandleEventWithGuard(received_time);
    }
}

void Channel::HandleEventWithGuard(TimeStamp received_time)
{
    LOG_INFO("channel HandleEvent revents:%d\n", revents_);

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (close_callback_)
        {
            close_callback_();
        }
    }

    if (revents_ & EPOLLERR)
    {
        if (error_callback_)
        {
            error_callback_();
        }
    }

    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (read_callback_)
        {
            read_callback_(received_time);
        }
    }

    if (revents_ & EPOLLOUT)
    {
        if (write_callback_)
        {
            write_callback_();
        }
    }
}