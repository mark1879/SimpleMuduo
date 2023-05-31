#include "EventLoopThread.h"
#include "EventLoop.h"


EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, 
        const std::string &name)
        : loop_(nullptr)
        , exiting_(false)
        , thread_(std::bind(&EventLoopThread::ThreadFunc, this), name)
        , mutex_()
        , cond_()
        , callback_(cb)
{

}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->Quit();
        thread_.Join();
    }
}

EventLoop* EventLoopThread::StartLoop()
{
    thread_.Start();

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr)
        {
            cond_.wait(lock);
        }
        loop = loop_;
    }

    return loop;
}

void EventLoopThread::ThreadFunc()
{
     // 创建一个独立的 eventloop，
     // 和上面的线程是一一对应的，one loop per thread
    EventLoop loop;

    if (callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.Loop();
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}