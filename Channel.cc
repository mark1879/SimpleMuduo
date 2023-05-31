#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{}


Channel::~Channel()
{}


// channel的tie方法什么时候调用？
// 一个 TcpConnection 新连接创建的时候 TcpConnection => Channel 
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}


// 当改变 channel 所表示 fd 的 events 事件后, 
// Update 负责在 poller 里面更改 fd 相应的事件 epoll_ctl
void Channel::Update()
{
    // 通过 channel 所属的 EventLoop
    // 调用 poller 的相应方法，注册 fd 的 events 事件
    loop_->UpdateChannel(this);
}

// 在 channel 所属的 EventLoop 中， 把当前的channel删除掉
void Channel::Remove()
{
    loop_->RemoveChannel(this);
}

// fd 得到 poller 通知以后处理事件
void Channel::HandleEvent(Timestamp receiveTime)
{
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            HandleEventWithGuard(receiveTime);
        }
    }
    else
    {
        HandleEventWithGuard(receiveTime);
    }
}

// 根据 poller 通知的 channel 发生的具体事件
// 由 channel 负责调用具体的回调操作
void Channel::HandleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d\n", revents_);

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
            read_callback_(receiveTime);
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