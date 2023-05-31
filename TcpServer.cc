#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <strings.h>
#include <functional>

static EventLoop* CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop,
                const InetAddress &listen_addr,
                const std::string &name_arg,
                Option option)
                : loop_(CheckLoopNotNull(loop))
                , ip_port_(listen_addr.ToIpPort())
                , name_(name_arg)
                , acceptor_(new Acceptor(loop, listen_addr, option == kReusePort))
                , thread_pool_(new EventLoopThreadPool(loop, name_))
                , connection_callback_()
                , message_callback_()
                , next_conn_id_(1)
                , started_(0)
{
    // 当有新用户连接时， 会执行 TcpServer::NewConnection 回调
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::NewConnection, this, 
        std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for (auto &item : connections_)
    {
        
        TcpConnectionPtr conn(item.second); 
        item.second.reset();

        // 销毁连接
        conn->loop()->RunInLoop(
            std::bind(&TcpConnection::ConnectDestroyed, conn)
        );
    }
}

void TcpServer::SetThreadNum(int num_threads)
{
    thread_pool_->set_num_threads(num_threads);
}

void TcpServer::Start()
{
    // 防止一个TcpServer对象被 start 多次
    if (started_++ == 0) 
    {
        // 启动底层的loop线程池
        thread_pool_->Start(thread_init_callback_); 

        // 开始监听
        loop_->RunInLoop(std::bind(&Acceptor::Listen, acceptor_.get()));
    }
}

// 有新的客户端的连接，acceptor 会执行这个回调操作
void TcpServer::NewConnection(int sockfd, const InetAddress &peer_addr)
{
    // 轮询算法，选择一个sub loop，来管理 channel
    EventLoop *io_loop = thread_pool_->GetNextLoop(); 

    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ip_port_.c_str(), next_conn_id_);
    std::string conn_name = name_ + buf;

    ++next_conn_id_;
   
    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
        name_.c_str(), conn_name.c_str(), peer_addr.ToIpPort().c_str());

    // 通过 sockfd 获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    ::bzero(&local, sizeof(local));
    socklen_t addrlen = sizeof(local);
    if (::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }

    InetAddress local_addr(local);

    // 根据连接成功的sockfd，创建 TcpConnection 连接对象
    TcpConnectionPtr conn(new TcpConnection(
                            io_loop,
                            conn_name,
                            sockfd,         // Socket Channel
                            local_addr,
                            peer_addr));

    connections_[conn_name] = conn;

    // 下面的回调都是用户设置 TcpServer => TcpConnection => Channel=> Poller=> notify channel 回调
    conn->set_connection_callback(connection_callback_);
    conn->set_message_callback(message_callback_);
    conn->set_write_complete_callback(write_complete_callback_);

    conn->set_close_callback(
        std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1)
    );

    // 直接调用TcpConnection::connectEstablished
    io_loop->RunInLoop(std::bind(&TcpConnection::ConnectEstablished, conn));
}

void TcpServer::RemoveConnection(const TcpConnectionPtr &conn)
{
    loop_->RunInLoop(
        std::bind(&TcpServer::RemoveConnectionInLoop, this, conn)
    );
}

void TcpServer::RemoveConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n", 
        name_.c_str(), conn->name().c_str());

    connections_.erase(conn->name());
    EventLoop *io_loop = conn->loop(); 

    io_loop->QueueInLoop(
        std::bind(&TcpConnection::ConnectDestroyed, conn)
    );
}