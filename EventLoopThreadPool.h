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

    void set_num_threads(int num) { num_threads_ = num; }

    void Start(const ThreadInitCallback &cb = ThreadInitCallback());

    // 如果工作在多线程中，baseLoop_默认以轮询的方式分配 channel 给 subloop
    EventLoop* GetNextLoop();

    std::vector<EventLoop*> GetAllLoops();

    bool started() const { return started_; }
    const std::string name() const { return name_; }
private:

    EventLoop *base_loop_;
    std::string name_;
    bool started_;
    int num_threads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};