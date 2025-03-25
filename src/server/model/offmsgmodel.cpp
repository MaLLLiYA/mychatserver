#include "offmsgmodel.hpp"
#include "db.h"

// 保存离线消息
void OffMsgModel::insert(int userid, string msg)
{
    char sql[1024] = {0};
    sprintf(sql,
            "insert into offlinemessage values (%d, '%s');",
            userid, msg.c_str());

    MySQL mysql;

    // 链接成功
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 删除离线消息
void OffMsgModel::remove(int userid)
{
    char sql[1024] = {0};
    sprintf(sql,
            "delete from offlinemessage where userid = %d;", userid);

    MySQL mysql;

    // 链接成功
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询离线消息
vector<string> OffMsgModel::query(int userid)
{
    vector<string> msgs;
    char sql[1024] = {0};
    sprintf(sql,
            "select message from offlinemessage where userid = '%d';", userid);

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
                msgs.push_back(row[0]);
            }
            mysql_free_result(res);
            return msgs;
        }
    }

    // 未找到查询结果
    return msgs;
}