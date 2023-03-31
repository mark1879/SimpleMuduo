#pragma once
#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>; 

    EventLoopThreadPool(EventLoop *base_loop, const std::string &name_arg);
    ~EventLoopThreadPool();

    void set_thread_num(int num) { thread_num_ = num; }

    void Start(const ThreadInitCallback &cb = ThreadInitCallback());

    EventLoop* GetNextLoop();

    std::vector<EventLoop*> GetAllLoops();

    bool started() const { return started_; }
    const std::string name() const { return name_; }

private:
    EventLoop *base_loop_;
    std::string name_;
    bool started_;
    int thread_num_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;     // EventLoops are all created on stack
};