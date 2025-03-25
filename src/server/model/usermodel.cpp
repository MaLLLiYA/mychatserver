#include "usermodel.hpp"
#include "db.h"
#include <iostream>
using namespace std;

// 插入用户数据
bool UserModel::insert(User &user)
{
    char sql[1024] = {0};
    sprintf(sql,
            "insert into user (name, password, state) values ('%s', '%s', '%s');",
            user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());

    MySQL mysql;

    // 链接成功
    if (mysql.connect())
    {
        // 添加成功
        if (mysql.update(sql))
        {
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
}

// 根据ID查找用户
User UserModel::query(int id)
{
    char sql[1024] = {0};
    sprintf(sql,
            "select * from user where id = '%d';", id);

    MySQL mysql;

    // 链接成功
    if (mysql.connect())
    {
        // 添加成功

        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);

                mysql_free_result(res);
                return user;
            }
        }
    }
    // 未找到查询结果
    return User();
}

// 更新用户状态
bool UserModel::updateState(User &user)
{
    char sql[1024] = {0};
    sprintf(sql,
            "update user set state = '%s' where id = '%d';",
            user.getState().c_str(), user.getId());

    MySQL mysql;

    // 链接成功
    if (mysql.connect())
    {
        // 添加成功
        if (mysql.update(sql))
        {
            return true;
        }
    }

    return false;
}

// 服务器异常，重置业务
void UserModel::resetState()
{
    char sql[1024] = "update user set state = 'offline' where state = 'online';";
    MySQL mysql;

    // 链接成功
    if (mysql.connect())
    {
        // 添加成功
        mysql.update(sql);
    }
}