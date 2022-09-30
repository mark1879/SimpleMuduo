#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

#include <memory>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *base_loop, const std::string &name)
    : base_loop_(base_loop)
    , name_(name)
    , started_(false)
    , threads_num_(0)
    , next_(0)
{}

EventLoopThreadPool::~EventLoopThreadPool()
{}

void EventLoopThreadPool::Start(const ThreadInitCallback &cb) {
    started_ = true;

    for (int i = 0; i < threads_num_; i++) {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof(buf), "%s%d", name_.c_str(), i);
        EventLoopThread *t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->StartLoop());
    }

    // 整个服务端只有一个线程，运行着baseloop
    if (threads_num_ == 0 && cb) {
        cb(base_loop_);
    }
}

EventLoop* EventLoopThreadPool::GetNextLoop() {
    EventLoop *loop = base_loop_;

    if (!loops_.empty()) {
        loop = loops_[next_];
        ++next_;
        if (next_ >= loops_.size()) {
            next_ = 0;
        }
    }

    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::GetAllLoops() {
    if (loops_.empty()) {
        return std::vector<EventLoop*>(1, base_loop_);
    } else{
        return loops_;
    }
}
