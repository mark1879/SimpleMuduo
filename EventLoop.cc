#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

// 防止一个线程创建多个 EventLoop
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的 Poller IO 复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建 wakeup fd，用来 notify 唤醒 SubReactor 处理新来的 channel
int CreateEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}


EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , calling_pending_functors_(false)
    , thread_id_(CurrentThread::tid())
    , poller_(Poller::NewDefaultPoller(this))
    , wakeup_fd_(CreateEventfd())
    , wakeup_channel_(new Channel(this, wakeup_fd_))    // 新建一个 Channel，用于唤醒当前的 EventLoop
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, thread_id_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, thread_id_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    wakeup_channel_->set_read_callback(std::bind(&EventLoop::HandleRead, this));

    // 每一个 eventloop 都将监听 wakeup_channel的 EPOLLIN 读事件
    wakeup_channel_->EnableReading();
}

EventLoop::~EventLoop()
{
    wakeup_channel_->DisableAll();
    wakeup_channel_->Remove();
    ::close(wakeup_fd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::Loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while(!quit_)
    {
        active_channels_.clear();

        // 监听两类 fd: client的fd、wakeup_fd
        poll_return_time_ = poller_->Poll(kPollTimeMs, &active_channels_);
        for (Channel *channel : active_channels_)
        {
            // Poller 监听到哪些 Channel 有发生事件
            // 然后上报给 EventLoop，通知 Channel 处理相应的事件
            channel->HandleEvent(poll_return_time_);
        }

        // 执行当前 EventLoop 事件循环需要处理的回调操作 
        DoPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}


// 退出事件循环  
// 1. loop 在自己的线程中调用 Quit  
// 2. 在非 loop 的线程中，调用 loop 的 Quit
void EventLoop::Quit()
{
    quit_ = true;

    // 如果是在其它线程中，调用 Quit
    // 需唤醒当前线程 
    if (!IsInLoopThread())  
    {
        Wakeup();
    }
}

void EventLoop::RunInLoop(Functor cb)
{
    if (IsInLoopThread()) // 在当前的 loop 线程中，执行cb
    {
        cb();
    }
    else // 在非当前 loop 线程中执行cb , 就需要唤醒 loop 所在线程，再执行cb
    {
        QueueInLoop(cb);
    }
}

// 把 cb 放入队列中，唤醒 loop 所在的线程，执行 cb
void EventLoop::QueueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pending_functors_.emplace_back(cb);
    }

    // || calling_pending_functors_：当前 loop 正在执行回调，但是 loop 又有了新的回调
    if (!IsInLoopThread() || calling_pending_functors_) 
    {
        Wakeup();
    }
}

void EventLoop::HandleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::HandleRead() reads %lu bytes instead of 8", n);
    }
}

// 用来唤醒loop所在的线程  
// 向 wakeupfd_ 写一个数据，wakeup_channel就发生读事件，当前loop线程就会被唤醒
// 没有读、写事件时，epoll_wait 会一直阻塞，所以需要唤醒
void EventLoop::Wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeup_fd_, &one, sizeof one);
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

void EventLoop::UpdateChannel(Channel *channel)
{
    poller_->UpdateChannel(channel);
}

void EventLoop::RemoveChannel(Channel *channel)
{
    poller_->RemoveChannel(channel);
}

bool EventLoop::HasChannel(Channel *channel)
{
    return poller_->HasChannel(channel);
}

void EventLoop::DoPendingFunctors() 
{
    std::vector<Functor> functors;
    calling_pending_functors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pending_functors_);
    }

    for (const Functor &functor : functors)
    {
        functor();
    }

    calling_pending_functors_ = false;
}