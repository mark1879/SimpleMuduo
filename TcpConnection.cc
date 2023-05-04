#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <errno.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <string>

static EventLoop* CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection Loop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

void DefaultConnectionCallback(const TcpConnectionPtr& conn)
{
    LOG_INFO("%s -> %s is %s", conn->local_addr().ToIPPort().c_str()
                , conn->peer_addr().ToIPPort().c_str()
                , (conn->connected() ? "UP" : "DOWN"));
  // do not call conn->forceClose(), because some users want to register message callback only.
}

void DefaultMessageCallback(const TcpConnectionPtr&, Buffer* buf, Timestamp)
{
     buf->RetrieveAll();
}

TcpConnection::TcpConnection(EventLoop *loop, 
                const std::string &name, 
                int sock_fd,
                const InetAddress& local_addr,
                const InetAddress& peer_addr)
    : loop_(CheckLoopNotNull(loop))
    , name_(name)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(sock_fd))
    , channel_(new Channel(loop, sock_fd))
    , local_addr_(local_addr)
    , peer_addr_(peer_addr)
    , high_watermark_(64 * 1024 * 1024)     // 64M
{
    channel_->set_read_callback(
        std::bind(&TcpConnection::HandleRead, this, std::placeholders::_1)
    );

    channel_->set_write_callback(
        std::bind(&TcpConnection::HandleWrite, this)
    );

    channel_->set_close_callback(
        std::bind(&TcpConnection::HandleClose, this)
    );

    channel_->set_error_allback(
        std::bind(&TcpConnection::HandleError, this)
    );

    LOG_INFO("TcpConnection::ctor[%s] at sock_fd=%d\n", name_.c_str(), sock_fd);
    socket_->SetKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", 
        name_.c_str(), channel_->fd(), (int)state_);
}

void TcpConnection::Send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if (loop_->IsInLoopThread())
        {
            SendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->RunInLoop(std::bind(
                &TcpConnection::SendInLoop,
                this,
                buf.c_str(),
                buf.size()
            ));
        }
    }
}

void TcpConnection::SendInLoop(const void* data, size_t len)
{
    ssize_t bytes_wrote = 0;
    size_t bytes_remain = len;
    bool fault_error = false;

    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing!");
        return; 
    }

    if (!channel_->IsWriting() && output_buffer_.ReadableBytes() == 0)
    {
        bytes_wrote = ::write(channel_->fd(), data, len);
        if (bytes_wrote >= 0)
        {
            bytes_remain = len - bytes_wrote;
            if (bytes_remain == 0 && write_complete_callback_)
            {
                // 数据已经发送完毕，不需要再注册 EPOLLOUT 事件，即不会收到 HandleWrite 回调
                loop_->QueueInLoop(std::bind(write_complete_callback_, shared_from_this()));
            }
        }
        else 
        {
            bytes_wrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::SendInLoop");
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    fault_error = true;
                }
            }
        }
    }

    // 将剩余的数据存入缓冲区，并注册 EPOLLOUT 事件，等待 HandleWrite 回调
    if (!fault_error && bytes_remain > 0) 
    {
        size_t old_len = output_buffer_.ReadableBytes();

        if (old_len + bytes_remain >= high_watermark_
            && old_len < high_watermark_    // 如果 old_len >= high_watermark_, 则已经回调过
            && high_watermark_callback_)
        {
            loop_->QueueInLoop(std::bind(high_watermark_callback_, shared_from_this(), old_len + bytes_remain));
        }

        output_buffer_.Append((char*)data + bytes_wrote, bytes_remain);
        if (!channel_->IsWriting())
        {
            channel_->EnableWriting();
        }
    }
}

void TcpConnection::Shutdown()
{
    if (state_ == kConnected)
    {
        // 要等缓冲中的数据发送完毕，再关闭 socket 的 write 功能
        set_state(kDisconnecting);     
        loop_->RunInLoop(std::bind(&TcpConnection::ShutdownInLoop, this));
    }
}

void TcpConnection::ShutdownInLoop()   
{
    if (!channel_->IsWriting())
    {
        socket_->ShutdownWrite();
    }
}

void TcpConnection::ConnectEstablished()
{
    set_state(kConnected);
    channel_->set_tie(shared_from_this());
    channel_->EnableReading();

    connection_callback_(shared_from_this());
}

void TcpConnection::ConnectDestroyed()
{
    if (state_ == kConnected)
    {
        set_state(kDisconnected);
        channel_->DisableAll();

        connection_callback_(shared_from_this());
    }
    channel_->Remove();
}

void TcpConnection::HandleRead(Timestamp received_time)
{
    int saved_errno = 0;
    ssize_t n = input_buffer_.ReadFd(channel_->fd(), &saved_errno);
    if (n > 0)
    {
        message_callback_(shared_from_this(), &input_buffer_, received_time);
        // input_buffer_.Retrieve(n);  // 数据已经回调给用户，释放 Buffer？ Muduo 中没有这个逻辑
    }
    else if (n == 0)
    {
        HandleClose();  // 客户端断开
    }
    else
    {
        errno = saved_errno;
        LOG_ERROR("TcpConnection::handleRead");
        HandleError();
    }
}

void TcpConnection::HandleWrite()
{
    if (channel_->IsWriting())
    {
        int saved_errno = 0;
        ssize_t n = output_buffer_.WriteFd(channel_->fd(), &saved_errno);
        if (n > 0)
        {
            output_buffer_.Retrieve(n);     // 释放 Buffer
            if (output_buffer_.ReadableBytes() == 0)
            {   
                // 缓冲区中的数据发送完毕，注销 EPOLLOUT 事件
                channel_->DisableWriting();
                if (write_complete_callback_)
                {
                    // 唤醒 loop 对应的线程
                    loop_->QueueInLoop(
                        std::bind(write_complete_callback_, shared_from_this())
                    );
                }

                // 主动调用过 Shutdown
                if (state_ == kDisconnecting)
                {
                    ShutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::HandleWrite");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
    }
}

void TcpConnection::HandleClose()
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n", channel_->fd(), (int)state_);
    set_state(kDisconnected);
    channel_->DisableAll();

    TcpConnectionPtr conn_ptr(shared_from_this());
    connection_callback_(conn_ptr);
    close_callback_(conn_ptr);
}

void TcpConnection::HandleError()
{
    int opt_val;
    socklen_t opt_len = sizeof(opt_val);
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &opt_val, &opt_len) < 0)
    {
        err = errno;
    }
    else
    {
        err = opt_val;
    }

    LOG_ERROR("TcpConnection::HandleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}