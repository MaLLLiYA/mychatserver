#include "friendmodel.hpp"
#include "db.h"
// #include "user.hpp"
// 添加好友
void FriendModel::insert(int id, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql,
            "insert into friend values (%d, %d);", id, friendid);

    MySQL mysql;

    // 链接成功
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 获取好友列表
vector<User> FriendModel::query(int id)
{
    vector<User> friends;
    char sql[1024] = {0};
    sprintf(sql,
            "select a.id, a.name, a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d;", id);

    MySQL mysql;

    // 链接成功
    if (mysql.connect())
    {
        // 添加成功
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);

                friends.push_back(user);
            }
            mysql_free_result(res);
            return friends;
        }
    }

    // 未找到查询结果
    return friends;
}