#include "chatserver.hpp"
#include <functional>
#include <iostream>
#include <string>
#include "json.hpp"
#include "chatservice.hpp"

using namespace std;
using namespace placeholders; // std::placeholders
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg) : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册连接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册读写事件回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 配置线程数
    _server.setThreadNum(4);
}

void ChatServer::start()
{
    _server.start();
}

// 连接回调
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 客户端断开连接
    if (!conn->connected())
    {
        ChatService::instance()->clientClosedException(conn);

        conn->shutdown();
    }
}

// 读写事件回调
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    // 数据的反序列化 字符串==>数据对象 包含msg_type或msg_id(业务标识)
    // 不直接使用if条件判断，否则网络模块与业务模块 强耦合
    // 一个消息ID映射一个事件处理 conn js time
    string buf = buffer->retrieveAllAsString();
    json js = json::parse(buf);

    // 无业务层的方法调用，业务变化，网络模块不需要改变
    MsgHandler msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());

    // 回调消息绑定好的事件处理器，执行响应的业务
    msgHandler(conn, js, time);
}