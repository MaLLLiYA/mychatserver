#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <functional>
#include <unordered_map>
#include <mutex>
#include "redis.hpp"
#include "json.hpp"
#include "friendmodel.hpp"
#include "usermodel.hpp"
#include "offmsgmodel.hpp"
#include "groupmodel.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

// 业务类
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService *instance();

    // 登录回调
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 注册回调
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 服务器异常，重置业务
    void reset();

    // 一对一聊天
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 获取消息ID对应的MsgHandler
    MsgHandler getHandler(int msgid);

    // 客户端异常退出
    void clientClosedException(const TcpConnectionPtr &conn);

    // 添加好友
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 创建群组
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 加入群组
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 群聊天
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 注销
    void logout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    
    // 
    void handlerRedisSubscribeMsg(int channel,string message);

private:
    ChatService();

    unordered_map<int, MsgHandler> _msgHandlerMap;

    // 在线用户的连接信息
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;

    // 数据操作类对象
    UserModel _usermodel;
    OffMsgModel _offMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;
    Redis _redis;
};

#endif
