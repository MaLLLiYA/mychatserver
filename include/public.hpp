#ifndef PUBLIC_H
#define PUBLIC_H

enum EnMsgType
{
    LOGIN_MSG = 1,
    LOGOUT_MSG,     // 注销
    LOGIN_MSG_ACK,  // 登录响应消息
    REG_MSG,
    REG_MSG_ACK,     // 注册响应消息
    ONE_CHAT_MSG,   // 一对一聊天
    ADD_FRIEND_MSG, // 添加好友
    ADD_GROUP_MSG,  // 加入群组
    CREATE_GROUP_MSG,   // 创建群组
    GROUP_CHAT_MSG,     // 群聊消息
};

#endif