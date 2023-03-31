#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

// prevent one thread creating multiple EventLoops
__thread EventLoop* loop_in_this_thread_ = nullptr;
 
const int kPollTimeMs = 10000;

int CreateEventFd()
{
    int event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (event_fd < 0)
    {
        LOG_FATAL("create event fd error:%d \n", errno);
    }

    return event_fd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , calling_pending_functors_(false)
    , thread_id_(CurrentThread::tid())
    , poller_(Poller::NewDefaultPoller(this))
    , wakeup_fd_(CreateEventFd())
    , wakeup_channel_(new Channel(this, wakeup_fd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, thread_id_);
    if (loop_in_this_thread_)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", loop_in_this_thread_, thread_id_);
    }
    else
    {
        loop_in_this_thread_ = this;
    }

    wakeup_channel_->set_read_callback(std::bind(&EventLoop::HandleRead, this));
    wakeup_channel_->EnableReading(); 
}

EventLoop::~EventLoop()
{
    wakeup_channel_->DisableAll();
    wakeup_channel_->Remove();
    close(wakeup_fd_);
    loop_in_this_thread_ = nullptr;
}

void EventLoop::Loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while(!quit_)
    {
        activate_channel_list_.clear();

        poll_return_time_ = poller_->Poll(kPollTimeMs, &activate_channel_list_);
        
        for (Channel *channel : activate_channel_list_)
        {
            channel->HandleEvent(poll_return_time_);
        }

        DoPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

void EventLoop::Quit()
{
    quit_ = true;

    if (!IsInLoopThread())
    {
        WakeUp();
    }
}

void EventLoop::RunInLoop(Functor cb)
{
    if (IsInLoopThread())
    {
        cb();
    }
    else 
    {
        QueueInLoop(cb);
    }
}

void EventLoop::QueueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pending_functors_.emplace_back(cb);
    }

    if (!IsInLoopThread() || calling_pending_functors_) 
    {
        WakeUp();
    }
}

void EventLoop::HandleRead()
{
  uint64_t one = 1;
  ssize_t ret = read(wakeup_fd_, &one, sizeof(one));
  if (ret != sizeof(one))
  {
    LOG_ERROR("EventLoop::HandleRead() reads %lu bytes instead of 8", ret);
  }
}

void EventLoop::WakeUp()
{
    uint64_t one = 1;
    ssize_t n = write(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::WakeUp() writes %lu bytes instead of 8 \n", n);
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