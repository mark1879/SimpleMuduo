#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop,
                const std::string& name,
                int sock_fd,
                const InetAddress& local_addr,
                const InetAddress& peer_addr);
    ~TcpConnection();

    EventLoop* loop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& local_addr() const { return local_addr_; }
    const InetAddress& perr_addr() const { return peer_addr_; }

    bool connected() const { return state_ == kConnected; }

    void Send(const std::string &buf);
    void Shutdown();

    void set_connection_callback(const ConnectionCallback& cb)
    { connection_callback_ = cb; }

    void set_message_callback(const MessageCallback& cb)
    { message_callback_ = cb; }

    void set_write_complete_callback(const WriteCompleteCallback& cb)
    { write_complete_callback_ = cb; }

    void set_high_watermark_callback(const HighWaterMarkCallback& cb, size_t high_watermark)
    {
        high_watermark_callback_ = cb; 
        high_watermark_ = high_watermark;
    }

    void set_close_callback(const CloseCallback& cb)
    { close_callback_ = cb; }

    void ConnectEstablished();
    void ConnectDestroyed();

private:
    enum StateE {kDisconnected, kConnecting, kConnected, kDisconnecting};
    void set_state(StateE state) { state_ = state; }

    void HandleRead(Timestamp receive_time);
    void HandleWrite();
    void HandleClose();
    void HandleError();

    void SendInLoop(const void* message, size_t len);
    void ShutdownInLoop();

    EventLoop* loop_;       // This is definitely not base loop
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress local_addr_;
    const InetAddress peer_addr_;

    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    WriteCompleteCallback write_complete_callback_;
    HighWaterMarkCallback high_watermark_callback_;
    CloseCallback close_callback_;
    size_t high_watermark_;

    Buffer input_buffer_;
    Buffer output_buffer_;
};