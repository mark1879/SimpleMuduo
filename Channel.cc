#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false) { 
    }

Channel::~Channel() {
}

void Channel::set_tie(const std::shared_ptr<void>& obj) {
    tie_ = obj;
    tied_ = true;
}

void Channel::Update() {
    loop_->UpdateChannel(this);
}

void Channel::Remove() {
    loop_->RemoveChannel(this);
}

void Channel::HandleEvent(TimeStamp recv_time) {
    if (tied_) {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard) {
            HandleEventWithGuard(recv_time);
        }
    } else {
        HandleEventWithGuard(recv_time);
    }
}

void Channel::HandleEventWithGuard(TimeStamp recv_time) {
    LOG_INFO("channel handle event, revents: %d\n", revents_);

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (close_callback_)
            close_callback_();
    }

    if (revents_ & EPOLLERR) {
        if (error_callback_)
            error_callback_();
    }

    if (revents_ & (EPOLLIN | EPOLLPRI)) {
        if (read_callback_)
            read_callback_(recv_time);
    }

    if (revents_ & EPOLLOUT) {
        if (write_callback_)
            write_callback_();
    }
}
