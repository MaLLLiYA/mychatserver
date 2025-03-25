#include "chatservice.hpp"
#include "public.hpp"
#include <functional>
#include <muduo/base/Logging.h>
#include <string>
#include <vector>
#include <iostream>
using namespace std;
using namespace placeholders;
using namespace muduo;

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 注册消息以及对应的事件回调
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGOUT_MSG, std::bind(&ChatService::logout, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 连接redis
    if (_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&ChatService::handlerRedisSubscribeMsg, this, _1, _2));
    }
}

// 获取消息ID对应的MsgHandler
MsgHandler ChatService::getHandler(int msgid)
{
    // 处理异常：msgid对应的Handler不存在
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            LOG_ERROR << "msgid: " << msgid << " can not find a handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 服务器异常，重置业务
void ChatService::reset()
{
    _usermodel.resetState();
}

// 登录回调
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "DO LOGIN SERVICE!!!";
    // id password
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _usermodel.query(id);
    if (user.getId() == id && user.getPassword() == pwd)
    {
        if (user.getState() == "online")
        {
            // 重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "用户已登录，请勿重复登录";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功，记录连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // 登录成功，订阅消息
            _redis.subscribe(id);

            // 登录成功，更新用户状态
            user.setState("online");
            _usermodel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 登录成功，检查用户是否有离线消息
            vector<string> msgs = _offMsgModel.query(id);

            if (!msgs.empty())
            {
                // 存在离线消息
                response["offlinemsg"] = msgs;

                // 删除离线消息
                _offMsgModel.remove(id);
            }

            // 登录成功，获取好友列表
            vector<User> friends = _friendModel.query(id);
            if (!friends.empty())
            {
                // 好友列表非空
                vector<string> v;
                for (User &user : friends)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();

                    v.push_back(js.dump());
                }
                response["friends"] = v;
            }

            vector<Group> groupuserVec = _groupModel.queryGroup(id);

            if (!groupuserVec.empty())
            {
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();

                    vector<string> userV;

                    for (GroupUser &user : group.getUsers()) // vector<GroupUser>
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }
            conn->send(response.dump());
        }
    }
    else
    {
        // 用户名或密码错误
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户名或密码错误";
        conn->send(response.dump());
    }
}

// 注册回调
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "DO REG SERVICE!!!";
    // name password
    string name = js["name"];
    string pwd = js["password"];
    User user;
    user.setName(name);
    user.setPassword(pwd);

    bool state = _usermodel.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 客户端异常退出
void ChatService::clientClosedException(const TcpConnectionPtr &conn)
{
    User user;

    {
        lock_guard<mutex> lock(_connMutex);
        // 删除map中的项
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    // 取消订阅
    _redis.unsbsubscribe(user.getId());

    if (user.getId() != -1)
    {
        // 更新用户状态
        user.setState("offline");
        _usermodel.updateState(user);
    }
}

// 注销
void ChatService::logout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "DO LOGOUT SERVICE!!!";
    int userID = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userID);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 取消订阅
    _redis.unsbsubscribe(userID);

    User user(userID, "", "", "offline");
    // 更新用户状态
    _usermodel.updateState(user);
}

// 一对一聊天
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "DO ONECHAT SERVICE!!!";
    int toid = js["to"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);

        if (it != _userConnMap.end())
        {
            // 用户在线，转发消息
            it->second->send(js.dump());
            return;
        }
    }

    // 用户登录到其它服务器
    User user = _usermodel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        LOG_INFO << "one chat msg published to redis, id: " << toid << ", msg: " << js.dump();
        return;
    }

    // 用户不在线，保存离线消息
    _offMsgModel.insert(toid, js.dump());
}

// 添加好友
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "DO ADDFRIEND SERVICE!!!";
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    _friendModel.insert(userid, friendid);
}

// 创建群组
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "DO CREATEGROUP SERVICE!!!";
    int userId = js["id"].get<int>();
    string groupName = js["groupname"];
    string groupDesc = js["groupdesc"];

    Group group(-1, groupName, groupDesc);
    bool state = _groupModel.createGroup(group);
    if (state)
    {
        // 创建成功，存储创建者信息
        _groupModel.addGroup(userId, group.getId(), "creator");

        json response;
        response["userid"] = userId;
        response["groupid"] = group.getId();
        response["errno"] = 0;
        conn->send(response.dump());
    }
    else
    {
        // 创建失败
        json response;
        response["userid"] = userId;
        response["errno"] = 2;
        response["errmsg"] = "群组创建失败";
        conn->send(response.dump());
    }
}

// 加入群组
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "DO ADDGROUP SERVICE!!!";
    int userId = js["id"].get<int>();
    int groupId = js["groupid"].get<int>();

    _groupModel.addGroup(userId, groupId, "normal");
}

// 群聊天
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "DO GROUPCHAT SERVICE!!!";
    int userId = js["id"].get<int>();
    int groupId = js["groupid"].get<int>();

    vector<int> idVec = _groupModel.queryGroupUser(userId, groupId);
    lock_guard<mutex> lock(_connMutex);
    for (int id : idVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 用户在线
            it->second->send(js.dump());
        }
        else
        {
            // 用户登录到其它服务器
            User user = _usermodel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
                LOG_INFO << "group chat msg published to redis, id: " << id << ", msg: " << js.dump();
                return;
            }
            else
            {
                // 用户离线
                _offMsgModel.insert(id, js.dump());
            }
        }
    }
}

// 提供给redis的回调
void ChatService::handlerRedisSubscribeMsg(int userID, string message)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userID);
    if (it != _userConnMap.end())
    {
        it->second->send(message);
        return;
    }

    _offMsgModel.insert(userID, message);
}
