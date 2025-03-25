#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;

class ChatServer
{
public:
    ChatServer(EventLoop *loop,
               const InetAddress &listenAddr,
               const string &nameArg);

    void start();

private:
    // 连接回调
    void onConnection(const TcpConnectionPtr &);

    // 读写事件回调
    void onMessage(const TcpConnectionPtr &,
                   Buffer *,
                   Timestamp);

    TcpServer _server;  // 无默认构造函数
    EventLoop *_loop;   // 用于退出事件循环
};

#endif