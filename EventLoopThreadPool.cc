#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

#include <memory>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *loop, const std::string &name_arg)
    : base_loop_(loop)
    , name_(name_arg)
    , started_(false)
    , num_threads_(0)
    , next_(0)
{}

EventLoopThreadPool::~EventLoopThreadPool()
{
  // Don't delete loop, it's stack variable
}

void EventLoopThreadPool::Start(const ThreadInitCallback &cb)
{
    started_ = true;

    for (int i = 0; i < num_threads_; ++i)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);

        EventLoopThread *t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));

        // 底层创建线程，绑定一个新的 EventLoop，并返回该 loop 的地址
        loops_.push_back(t->StartLoop()); 
    }

    // 整个服务端只有一个线程，运行着 baseloop
    if (num_threads_ == 0 && cb)
    {
        cb(base_loop_);
    }
}

// 如果工作在多线程中，baseLoop_默认以轮询的方式分配 channel 给 subloop
EventLoop* EventLoopThreadPool::GetNextLoop()
{
    EventLoop *loop = base_loop_;

    // 通过轮询获取下一个处理事件的loop
    if (!loops_.empty()) 
    {
        loop = loops_[next_];
        ++next_;
        if (next_ >= loops_.size())
        {
            next_ = 0;
        }
    }

    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::GetAllLoops()
{
    if (loops_.empty())
    {
        return std::vector<EventLoop*>(1, base_loop_);
    }
    else
    {
        return loops_;
    }
}