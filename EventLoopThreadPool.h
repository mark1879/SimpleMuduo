#pragma once
#include "noncopyable.h"
#include "EventLoop.h"
#include "EventLoopThread.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoopThreadPool : noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop *base_loop, const std::string &name);
    ~EventLoopThreadPool();

    void Start(const ThreadInitCallback &cb = ThreadInitCallback());
    EventLoop* GetNextLoop();
    std::vector<EventLoop*> GetAllLoops();

    void set_threads_num(int num) { threads_num_ = num; }
    bool started() const { return started_; }
    const std::string get_name() const { return name_; }
private:
    EventLoop *base_loop_;
    std::string name_;
    bool started_;
    int threads_num_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};