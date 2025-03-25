#include "groupmodel.hpp"
#include "db.h"
#include "groupuser.hpp"
#include <iostream>

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql,
            "insert into allgroup (groupname, groupdesc) values ('%s', '%s');",
            group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;

    // 链接成功
    if (mysql.connect())
    {
        // 添加成功
        if (mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
}

// 加入群组
void GroupModel::addGroup(int userId, int groupId, string role)
{
    char sql[1024] = {0};
    sprintf(sql,
            "insert into groupuser (groupid, userid, grouprole) values (%d, %d,'%s');",
            groupId, userId, role.c_str());

    MySQL mysql;

    // 链接成功
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户所在群组信息
vector<Group> GroupModel::queryGroup(int userId)
{
    // 查询出所有的群组
    vector<Group> v;
    char sql[1024] = {0};
    sprintf(sql,
            "select a.id, a.groupname, a.groupdesc from allgroup a inner join \
            groupuser b on b.groupid=a.id where b.userid= %d;",
            userId);

    MySQL mysql;

    // 链接成功
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;

            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                v.push_back(group);
            }
            mysql_free_result(res);
        }
    }

    // 查询群组的用户信息
    for (Group &group : v)
    {

        sprintf(sql,
                "select a.id, a.name, a.state, b.grouprole from user a inner join \
            groupuser b on a.id=b.userid where b.groupid=%d;",
                group.getId());

        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser groupuser;
                groupuser.setId(atoi(row[0]));
                groupuser.setName(row[1]);
                groupuser.setState(row[2]);
                groupuser.setRole(row[3]);

                group.getUsers().push_back(groupuser);
            }
            mysql_free_result(res);
        }
    }

    return v;
}

// 根据GROUPID查询用户ID列表，用于群发消息
vector<int> GroupModel::queryGroupUser(int userId, int groupId)
{
    // 查询出所有的群组
    vector<int> v;
    char sql[1024] = {0};
    sprintf(sql,
            "select userid from groupuser where groupid=%d and userid!=%d;",
            groupId, userId);

    MySQL mysql;

    // 链接成功
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;

            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                v.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return v;
}
