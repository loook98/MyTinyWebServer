#include "config.h"

//服务器主程序，调用WebServer类实现Web服务器
int main(int argc, char *argv[])
{
    //需要修改的数据库信息,登录名,密码,库名
    string user = "root";
    string passwd = "123";
    string databasename = "webserverDb";

    //命令行解析
    Config config;
    config.parse_arg(argc, argv);

    WebServer server;

    //初始化
    server.init(config.PORT, user, passwd, databasename, config.LOGWrite,
                config.OPT_LINGER, config.TRIGMode,  config.sql_num,  config.thread_num,
                config.close_log, config.actor_model);


    //初始化日志系统
    server.log_write();

    //初始化数据库连接池
    server.sql_pool();

    //创建线程池
    server.thread_pool();

    //设置listenfd和connfd的epoll的触发模式
    server.trig_mode();

    //开始监听套接口
    server.eventListen();

    //运行服务器主线程
    server.eventLoop();

    return 0;
}

