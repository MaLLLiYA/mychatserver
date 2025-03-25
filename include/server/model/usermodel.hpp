#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"

// User表的数据操作类
class UserModel
{
public:
    // 插入用户数据
    bool insert(User &user);

    // 根据ID查找用户
    User query(int id);

    // 更新用户状态
    bool updateState(User &user);

    // 服务器异常，重置业务
    void resetState();

private:
};

#endif