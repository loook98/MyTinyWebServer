#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../lock/locker.h"
#include "../log/log.h"

using namespace std;

class connection_pool{
public:
    //获取一个数据库连接
    MYSQL* getConnection();
    //释放一个数据库连接
    bool releaseConnection(MYSQL* conn);
    //销毁所有连接
    void destroyPool();

    //获取空闲连接数
    int getFreeConn();

    //单例模式，获取数据库连接池
    static connection_pool* getInstance();
    //初始化
    void init(string url, string user, string password, string databaseName, int port, int maxConn, int close_log);

private:
    connection_pool();
    ~connection_pool();
    
    
    locker lock;   //保护连接池的互斥锁
    sem reserve;   //对应可用连接数的信号量
    list<MYSQL*> connList;   //连接池
    
    int m_maxConn;//最大连接数
    int m_curConn;//当前已使用的连接数
    int m_freeConn;//空闲连接数

public:
    string m_url;          //主机地址
    int m_port;         //数据库端口
    string m_user;         //登录用户名
    string m_password;     //登录密码
    string m_databaseName; //使用的数据库名称

    int m_close_log; //日志开关

};

class connectionRAII{
public:
    //从给定的connPool中取出一个连接（指针），赋值给给定的指针
    connectionRAII(MYSQL** conn, connection_pool *connPool);
    ~connectionRAII();

private:
    MYSQL* connRAII;
    connection_pool *poolRAII;
};

#endif