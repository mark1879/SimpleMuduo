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

TcpServer::TcpServer(EventLoop* loop,
                const InetAddress& listen_addr,
                const std::string& name,
                Option option)
                : loop_(CheckLoopNotNull(loop))
                , ip_port_(listen_addr.ToIPPort())
                , name_(name)
                , acceptor_(new Acceptor(loop, listen_addr, option == kReusePort))
                , thread_pool_(new EventLoopThreadPool(loop, name_))
                , connection_callback_()
                , message_callback_()
                , next_conn_id_(1)
                , started_(0)
{
    acceptor_->set_new_connection_callback(std::bind(&TcpServer::NewConnection, this, 
        std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for (auto &item : connections_)
    {
        TcpConnectionPtr conn(item.second); 
        item.second.reset();

        conn->loop()->RunInLoop(
            std::bind(&TcpConnection::ConnectDestroyed, conn)
        );
    }
}

void TcpServer::SetThreadNum(int num_threads)
{
    thread_pool_->set_thread_num(num_threads);
}

void TcpServer::Start()
{
    if (started_++ == 0)
    {
        thread_pool_->Start(thread_init_callback_);
        loop_->RunInLoop(std::bind(&Acceptor::Listen, acceptor_.get()));
    }
}

void TcpServer::NewConnection(int sock_fd, const InetAddress &peer_addr)
{
    EventLoop *ioLoop = thread_pool_->GetNextLoop(); 
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ip_port_.c_str(), next_conn_id_);
    ++next_conn_id_;
    std::string conn_name = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
        name_.c_str(), conn_name.c_str(), peer_addr.ToIPPort().c_str());

    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addr_len = sizeof local;
    if (::getsockname(sock_fd, (sockaddr*)&local, &addr_len) < 0)
    {
        LOG_ERROR("sockets::GetLocalAddr");
    }

    InetAddress local_addr(local);

    TcpConnectionPtr conn(new TcpConnection(
                            ioLoop,
                            conn_name,
                            sock_fd,
                            local_addr,
                            peer_addr));

    connections_[conn_name] = conn;
    conn->set_connection_callback(connection_callback_);
    conn->set_message_callback(message_callback_);
    conn->set_write_complete_callback(write_complete_callback_);

    conn->set_close_callback(
        std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1)
    );

    ioLoop->RunInLoop(std::bind(&TcpConnection::ConnectEstablished, conn));
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
    EventLoop *ioLoop = conn->loop(); 
    ioLoop->QueueInLoop(
        std::bind(&TcpConnection::ConnectDestroyed, conn)
    );
}