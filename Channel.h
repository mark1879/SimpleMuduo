#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;

/**
 * Channel 理解为通道，封装了 sockfd 和其感兴趣的 event，如EPOLLIN、EPOLLOUT事件
 * 还绑定了 poller 返回的具体事件
 */ 
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd 得到 poller 通知以后处理事件
    void HandleEvent(Timestamp receiveTime);  

    void set_read_callback(ReadEventCallback cb) { read_callback_ = std::move(cb); }
    void set_write_callback(EventCallback cb) { write_callback_ = std::move(cb); }
    void set_close_callback(EventCallback cb) { close_callback_ = std::move(cb); }
    void set_error_callback(EventCallback cb) { error_callback_ = std::move(cb); }

    // 防止 channel 被手动 remove 掉
    // channel 还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; }

    // 设置 fd 相应的事件状态
    void EnableReading() { events_ |= kReadEvent; Update(); }
    void DisableReading() { events_ &= ~kReadEvent; Update(); }
    void EnableWriting() { events_ |= kWriteEvent; Update(); }
    void DisableWriting() { events_ &= ~kWriteEvent; Update(); }
    void DisableAll() { events_ = kNoneEvent; Update(); }

    // 返回 fd 当前的事件状态
    bool IsNoneEvent() const { return events_ == kNoneEvent; }
    bool IsWriting() const { return events_ & kWriteEvent; }
    bool IsReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    EventLoop* OwnerLoop() { return loop_; }
    void Remove();
private:

    void Update();
    void HandleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;
    const int fd_;    // fd, Poller 监听的对象
    int events_;    // 注册 fd 感兴趣的事件
    int revents_;   // poller 返回的具体发生的事件
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    ReadEventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback close_callback_;
    EventCallback error_callback_;
};

